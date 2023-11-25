5100 // File system implementation.  Five layers:
5101 //   + Blocks: allocator for raw disk blocks.
5102 //   + Log: crash recovery for multi-step updates.
5103 //   + Files: inode allocator, reading, writing, metadata.
5104 //   + Directories: inode with special contents (list of other inodes!)
5105 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
5106 //
5107 // This file contains the low-level file system manipulation
5108 // routines.  The (higher-level) system call implementations
5109 // are in sysfile.c.
5110 
5111 #include "types.h"
5112 #include "defs.h"
5113 #include "param.h"
5114 #include "stat.h"
5115 #include "mmu.h"
5116 #include "proc.h"
5117 #include "spinlock.h"
5118 #include "sleeplock.h"
5119 #include "fs.h"
5120 #include "buf.h"
5121 #include "file.h"
5122 
5123 #define min(a, b) ((a) < (b) ? (a) : (b))
5124 static void itrunc(struct inode*);
5125 // there should be one superblock per disk device, but we run with
5126 // only one device
5127 struct superblock sb;
5128 
5129 // Read the super block.
5130 void
5131 readsb(int dev, struct superblock *sb)
5132 {
5133   struct buf *bp;
5134 
5135   bp = bread(dev, 1);
5136   memmove(sb, bp->data, sizeof(*sb));
5137   brelse(bp);
5138 }
5139 
5140 
5141 
5142 
5143 
5144 
5145 
5146 
5147 
5148 
5149 
5150 // Zero a block.
5151 static void
5152 bzero(int dev, int bno)
5153 {
5154   struct buf *bp;
5155 
5156   bp = bread(dev, bno);
5157   memset(bp->data, 0, BSIZE);
5158   log_write(bp);
5159   brelse(bp);
5160 }
5161 
5162 // Blocks.
5163 
5164 // Allocate a zeroed disk block.
5165 static uint
5166 balloc(uint dev)
5167 {
5168   int b, bi, m;
5169   struct buf *bp;
5170 
5171   bp = 0;
5172   for(b = 0; b < sb.size; b += BPB){
5173     bp = bread(dev, BBLOCK(b, sb));
5174     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
5175       m = 1 << (bi % 8);
5176       if((bp->data[bi/8] & m) == 0){  // Is block free?
5177         bp->data[bi/8] |= m;  // Mark block in use.
5178         log_write(bp);
5179         brelse(bp);
5180         bzero(dev, b + bi);
5181         return b + bi;
5182       }
5183     }
5184     brelse(bp);
5185   }
5186   panic("balloc: out of blocks");
5187 }
5188 
5189 
5190 
5191 
5192 
5193 
5194 
5195 
5196 
5197 
5198 
5199 
5200 // Free a disk block.
5201 static void
5202 bfree(int dev, uint b)
5203 {
5204   struct buf *bp;
5205   int bi, m;
5206 
5207   bp = bread(dev, BBLOCK(b, sb));
5208   bi = b % BPB;
5209   m = 1 << (bi % 8);
5210   if((bp->data[bi/8] & m) == 0)
5211     panic("freeing free block");
5212   bp->data[bi/8] &= ~m;
5213   log_write(bp);
5214   brelse(bp);
5215 }
5216 
5217 // Inodes.
5218 //
5219 // An inode describes a single unnamed file.
5220 // The inode disk structure holds metadata: the file's type,
5221 // its size, the number of links referring to it, and the
5222 // list of blocks holding the file's content.
5223 //
5224 // The inodes are laid out sequentially on disk at
5225 // sb.startinode. Each inode has a number, indicating its
5226 // position on the disk.
5227 //
5228 // The kernel keeps a cache of in-use inodes in memory
5229 // to provide a place for synchronizing access
5230 // to inodes used by multiple processes. The cached
5231 // inodes include book-keeping information that is
5232 // not stored on disk: ip->ref and ip->valid.
5233 //
5234 // An inode and its in-memory representation go through a
5235 // sequence of states before they can be used by the
5236 // rest of the file system code.
5237 //
5238 // * Allocation: an inode is allocated if its type (on disk)
5239 //   is non-zero. ialloc() allocates, and iput() frees if
5240 //   the reference and link counts have fallen to zero.
5241 //
5242 // * Referencing in cache: an entry in the inode cache
5243 //   is free if ip->ref is zero. Otherwise ip->ref tracks
5244 //   the number of in-memory pointers to the entry (open
5245 //   files and current directories). iget() finds or
5246 //   creates a cache entry and increments its ref; iput()
5247 //   decrements ref.
5248 //
5249 // * Valid: the information (type, size, &c) in an inode
5250 //   cache entry is only correct when ip->valid is 1.
5251 //   ilock() reads the inode from
5252 //   the disk and sets ip->valid, while iput() clears
5253 //   ip->valid if ip->ref has fallen to zero.
5254 //
5255 // * Locked: file system code may only examine and modify
5256 //   the information in an inode and its content if it
5257 //   has first locked the inode.
5258 //
5259 // Thus a typical sequence is:
5260 //   ip = iget(dev, inum)
5261 //   ilock(ip)
5262 //   ... examine and modify ip->xxx ...
5263 //   iunlock(ip)
5264 //   iput(ip)
5265 //
5266 // ilock() is separate from iget() so that system calls can
5267 // get a long-term reference to an inode (as for an open file)
5268 // and only lock it for short periods (e.g., in read()).
5269 // The separation also helps avoid deadlock and races during
5270 // pathname lookup. iget() increments ip->ref so that the inode
5271 // stays cached and pointers to it remain valid.
5272 //
5273 // Many internal file system functions expect the caller to
5274 // have locked the inodes involved; this lets callers create
5275 // multi-step atomic operations.
5276 //
5277 // The icache.lock spin-lock protects the allocation of icache
5278 // entries. Since ip->ref indicates whether an entry is free,
5279 // and ip->dev and ip->inum indicate which i-node an entry
5280 // holds, one must hold icache.lock while using any of those fields.
5281 //
5282 // An ip->lock sleep-lock protects all ip-> fields other than ref,
5283 // dev, and inum.  One must hold ip->lock in order to
5284 // read or write that inode's ip->valid, ip->size, ip->type, &c.
5285 
5286 struct {
5287   struct spinlock lock;
5288   struct inode inode[NINODE];
5289 } icache;
5290 
5291 void
5292 iinit(int dev)
5293 {
5294   int i = 0;
5295 
5296   initlock(&icache.lock, "icache");
5297   for(i = 0; i < NINODE; i++) {
5298     initsleeplock(&icache.inode[i].lock, "inode");
5299   }
5300   readsb(dev, &sb);
5301   cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d\
5302  inodestart %d bmap start %d\n", sb.size, sb.nblocks,
5303           sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,
5304           sb.bmapstart);
5305 }
5306 
5307 static struct inode* iget(uint dev, uint inum);
5308 
5309 
5310 
5311 
5312 
5313 
5314 
5315 
5316 
5317 
5318 
5319 
5320 
5321 
5322 
5323 
5324 
5325 
5326 
5327 
5328 
5329 
5330 
5331 
5332 
5333 
5334 
5335 
5336 
5337 
5338 
5339 
5340 
5341 
5342 
5343 
5344 
5345 
5346 
5347 
5348 
5349 
5350 // Allocate an inode on device dev.
5351 // Mark it as allocated by  giving it type type.
5352 // Returns an unlocked but allocated and referenced inode.
5353 struct inode*
5354 ialloc(uint dev, short type)
5355 {
5356   int inum;
5357   struct buf *bp;
5358   struct dinode *dip;
5359 
5360   for(inum = 1; inum < sb.ninodes; inum++){
5361     bp = bread(dev, IBLOCK(inum, sb));
5362     dip = (struct dinode*)bp->data + inum%IPB;
5363     if(dip->type == 0){  // a free inode
5364       memset(dip, 0, sizeof(*dip));
5365       dip->type = type;
5366       log_write(bp);   // mark it allocated on the disk
5367       brelse(bp);
5368       return iget(dev, inum);
5369     }
5370     brelse(bp);
5371   }
5372   panic("ialloc: no inodes");
5373 }
5374 
5375 // Copy a modified in-memory inode to disk.
5376 // Must be called after every change to an ip->xxx field
5377 // that lives on disk, since i-node cache is write-through.
5378 // Caller must hold ip->lock.
5379 void
5380 iupdate(struct inode *ip)
5381 {
5382   struct buf *bp;
5383   struct dinode *dip;
5384 
5385   bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5386   dip = (struct dinode*)bp->data + ip->inum%IPB;
5387   dip->type = ip->type;
5388   dip->major = ip->major;
5389   dip->minor = ip->minor;
5390   dip->nlink = ip->nlink;
5391   dip->size = ip->size;
5392   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
5393   log_write(bp);
5394   brelse(bp);
5395 }
5396 
5397 
5398 
5399 
5400 // Find the inode with number inum on device dev
5401 // and return the in-memory copy. Does not lock
5402 // the inode and does not read it from disk.
5403 static struct inode*
5404 iget(uint dev, uint inum)
5405 {
5406   struct inode *ip, *empty;
5407 
5408   acquire(&icache.lock);
5409 
5410   // Is the inode already cached?
5411   empty = 0;
5412   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
5413     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
5414       ip->ref++;
5415       release(&icache.lock);
5416       return ip;
5417     }
5418     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
5419       empty = ip;
5420   }
5421 
5422   // Recycle an inode cache entry.
5423   if(empty == 0)
5424     panic("iget: no inodes");
5425 
5426   ip = empty;
5427   ip->dev = dev;
5428   ip->inum = inum;
5429   ip->ref = 1;
5430   ip->valid = 0;
5431   release(&icache.lock);
5432 
5433   return ip;
5434 }
5435 
5436 // Increment reference count for ip.
5437 // Returns ip to enable ip = idup(ip1) idiom.
5438 struct inode*
5439 idup(struct inode *ip)
5440 {
5441   acquire(&icache.lock);
5442   ip->ref++;
5443   release(&icache.lock);
5444   return ip;
5445 }
5446 
5447 
5448 
5449 
5450 // Lock the given inode.
5451 // Reads the inode from disk if necessary.
5452 void
5453 ilock(struct inode *ip)
5454 {
5455   struct buf *bp;
5456   struct dinode *dip;
5457 
5458   if(ip == 0 || ip->ref < 1)
5459     panic("ilock");
5460 
5461   acquiresleep(&ip->lock);
5462 
5463   if(ip->valid == 0){
5464     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5465     dip = (struct dinode*)bp->data + ip->inum%IPB;
5466     ip->type = dip->type;
5467     ip->major = dip->major;
5468     ip->minor = dip->minor;
5469     ip->nlink = dip->nlink;
5470     ip->size = dip->size;
5471     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
5472     brelse(bp);
5473     ip->valid = 1;
5474     if(ip->type == 0)
5475       panic("ilock: no type");
5476   }
5477 }
5478 
5479 // Unlock the given inode.
5480 void
5481 iunlock(struct inode *ip)
5482 {
5483   if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
5484     panic("iunlock");
5485 
5486   releasesleep(&ip->lock);
5487 }
5488 
5489 
5490 
5491 
5492 
5493 
5494 
5495 
5496 
5497 
5498 
5499 
5500 // Drop a reference to an in-memory inode.
5501 // If that was the last reference, the inode cache entry can
5502 // be recycled.
5503 // If that was the last reference and the inode has no links
5504 // to it, free the inode (and its content) on disk.
5505 // All calls to iput() must be inside a transaction in
5506 // case it has to free the inode.
5507 void
5508 iput(struct inode *ip)
5509 {
5510   acquiresleep(&ip->lock);
5511   if(ip->valid && ip->nlink == 0){
5512     acquire(&icache.lock);
5513     int r = ip->ref;
5514     release(&icache.lock);
5515     if(r == 1){
5516       // inode has no links and no other references: truncate and free.
5517       itrunc(ip);
5518       ip->type = 0;
5519       iupdate(ip);
5520       ip->valid = 0;
5521     }
5522   }
5523   releasesleep(&ip->lock);
5524 
5525   acquire(&icache.lock);
5526   ip->ref--;
5527   release(&icache.lock);
5528 }
5529 
5530 // Common idiom: unlock, then put.
5531 void
5532 iunlockput(struct inode *ip)
5533 {
5534   iunlock(ip);
5535   iput(ip);
5536 }
5537 
5538 
5539 
5540 
5541 
5542 
5543 
5544 
5545 
5546 
5547 
5548 
5549 
5550 // Inode content
5551 //
5552 // The content (data) associated with each inode is stored
5553 // in blocks on the disk. The first NDIRECT block numbers
5554 // are listed in ip->addrs[].  The next NINDIRECT blocks are
5555 // listed in block ip->addrs[NDIRECT].
5556 
5557 // Return the disk block address of the nth block in inode ip.
5558 // If there is no such block, bmap allocates one.
5559 static uint
5560 bmap(struct inode *ip, uint bn)
5561 {
5562   uint addr, *a;
5563   struct buf *bp;
5564 
5565   if(bn < NDIRECT){
5566     if((addr = ip->addrs[bn]) == 0)
5567       ip->addrs[bn] = addr = balloc(ip->dev);
5568     return addr;
5569   }
5570   bn -= NDIRECT;
5571 
5572   if(bn < NINDIRECT){
5573     // Load indirect block, allocating if necessary.
5574     if((addr = ip->addrs[NDIRECT]) == 0)
5575       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
5576     bp = bread(ip->dev, addr);
5577     a = (uint*)bp->data;
5578     if((addr = a[bn]) == 0){
5579       a[bn] = addr = balloc(ip->dev);
5580       log_write(bp);
5581     }
5582     brelse(bp);
5583     return addr;
5584   }
5585 
5586   panic("bmap: out of range");
5587 }
5588 
5589 
5590 
5591 
5592 
5593 
5594 
5595 
5596 
5597 
5598 
5599 
5600 // Truncate inode (discard contents).
5601 // Only called when the inode has no links
5602 // to it (no directory entries referring to it)
5603 // and has no in-memory reference to it (is
5604 // not an open file or current directory).
5605 static void
5606 itrunc(struct inode *ip)
5607 {
5608   int i, j;
5609   struct buf *bp;
5610   uint *a;
5611 
5612   for(i = 0; i < NDIRECT; i++){
5613     if(ip->addrs[i]){
5614       bfree(ip->dev, ip->addrs[i]);
5615       ip->addrs[i] = 0;
5616     }
5617   }
5618 
5619   if(ip->addrs[NDIRECT]){
5620     bp = bread(ip->dev, ip->addrs[NDIRECT]);
5621     a = (uint*)bp->data;
5622     for(j = 0; j < NINDIRECT; j++){
5623       if(a[j])
5624         bfree(ip->dev, a[j]);
5625     }
5626     brelse(bp);
5627     bfree(ip->dev, ip->addrs[NDIRECT]);
5628     ip->addrs[NDIRECT] = 0;
5629   }
5630 
5631   ip->size = 0;
5632   iupdate(ip);
5633 }
5634 
5635 // Copy stat information from inode.
5636 // Caller must hold ip->lock.
5637 void
5638 stati(struct inode *ip, struct stat *st)
5639 {
5640   st->dev = ip->dev;
5641   st->ino = ip->inum;
5642   st->type = ip->type;
5643   st->nlink = ip->nlink;
5644   st->size = ip->size;
5645 }
5646 
5647 
5648 
5649 
5650 // Read data from inode.
5651 // Caller must hold ip->lock.
5652 int
5653 readi(struct inode *ip, char *dst, uint off, uint n)
5654 {
5655   uint tot, m;
5656   struct buf *bp;
5657 
5658   if(ip->type == T_DEV){
5659     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
5660       return -1;
5661     return devsw[ip->major].read(ip, dst, n);
5662   }
5663 
5664   if(off > ip->size || off + n < off)
5665     return -1;
5666   if(off + n > ip->size)
5667     n = ip->size - off;
5668 
5669   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
5670     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5671     m = min(n - tot, BSIZE - off%BSIZE);
5672     memmove(dst, bp->data + off%BSIZE, m);
5673     brelse(bp);
5674   }
5675   return n;
5676 }
5677 
5678 
5679 
5680 
5681 
5682 
5683 
5684 
5685 
5686 
5687 
5688 
5689 
5690 
5691 
5692 
5693 
5694 
5695 
5696 
5697 
5698 
5699 
5700 // Write data to inode.
5701 // Caller must hold ip->lock.
5702 int
5703 writei(struct inode *ip, char *src, uint off, uint n)
5704 {
5705   uint tot, m;
5706   struct buf *bp;
5707 
5708   if(ip->type == T_DEV){
5709     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
5710       return -1;
5711     return devsw[ip->major].write(ip, src, n);
5712   }
5713 
5714   if(off > ip->size || off + n < off)
5715     return -1;
5716   if(off + n > MAXFILE*BSIZE)
5717     return -1;
5718 
5719   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
5720     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5721     m = min(n - tot, BSIZE - off%BSIZE);
5722     memmove(bp->data + off%BSIZE, src, m);
5723     log_write(bp);
5724     brelse(bp);
5725   }
5726 
5727   if(n > 0 && off > ip->size){
5728     ip->size = off;
5729     iupdate(ip);
5730   }
5731   return n;
5732 }
5733 
5734 
5735 
5736 
5737 
5738 
5739 
5740 
5741 
5742 
5743 
5744 
5745 
5746 
5747 
5748 
5749 
5750 // Directories
5751 
5752 int
5753 namecmp(const char *s, const char *t)
5754 {
5755   return strncmp(s, t, DIRSIZ);
5756 }
5757 
5758 // Look for a directory entry in a directory.
5759 // If found, set *poff to byte offset of entry.
5760 struct inode*
5761 dirlookup(struct inode *dp, char *name, uint *poff)
5762 {
5763   uint off, inum;
5764   struct dirent de;
5765 
5766   if(dp->type != T_DIR)
5767     panic("dirlookup not DIR");
5768 
5769   for(off = 0; off < dp->size; off += sizeof(de)){
5770     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5771       panic("dirlookup read");
5772     if(de.inum == 0)
5773       continue;
5774     if(namecmp(name, de.name) == 0){
5775       // entry matches path element
5776       if(poff)
5777         *poff = off;
5778       inum = de.inum;
5779       return iget(dp->dev, inum);
5780     }
5781   }
5782 
5783   return 0;
5784 }
5785 
5786 
5787 
5788 
5789 
5790 
5791 
5792 
5793 
5794 
5795 
5796 
5797 
5798 
5799 
5800 // Write a new directory entry (name, inum) into the directory dp.
5801 int
5802 dirlink(struct inode *dp, char *name, uint inum)
5803 {
5804   int off;
5805   struct dirent de;
5806   struct inode *ip;
5807 
5808   // Check that name is not present.
5809   if((ip = dirlookup(dp, name, 0)) != 0){
5810     iput(ip);
5811     return -1;
5812   }
5813 
5814   // Look for an empty dirent.
5815   for(off = 0; off < dp->size; off += sizeof(de)){
5816     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5817       panic("dirlink read");
5818     if(de.inum == 0)
5819       break;
5820   }
5821 
5822   strncpy(de.name, name, DIRSIZ);
5823   de.inum = inum;
5824   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5825     panic("dirlink");
5826 
5827   return 0;
5828 }
5829 
5830 
5831 
5832 
5833 
5834 
5835 
5836 
5837 
5838 
5839 
5840 
5841 
5842 
5843 
5844 
5845 
5846 
5847 
5848 
5849 
5850 // Paths
5851 
5852 // Copy the next path element from path into name.
5853 // Return a pointer to the element following the copied one.
5854 // The returned path has no leading slashes,
5855 // so the caller can check *path=='\0' to see if the name is the last one.
5856 // If no name to remove, return 0.
5857 //
5858 // Examples:
5859 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
5860 //   skipelem("///a//bb", name) = "bb", setting name = "a"
5861 //   skipelem("a", name) = "", setting name = "a"
5862 //   skipelem("", name) = skipelem("////", name) = 0
5863 //
5864 static char*
5865 skipelem(char *path, char *name)
5866 {
5867   char *s;
5868   int len;
5869 
5870   while(*path == '/')
5871     path++;
5872   if(*path == 0)
5873     return 0;
5874   s = path;
5875   while(*path != '/' && *path != 0)
5876     path++;
5877   len = path - s;
5878   if(len >= DIRSIZ)
5879     memmove(name, s, DIRSIZ);
5880   else {
5881     memmove(name, s, len);
5882     name[len] = 0;
5883   }
5884   while(*path == '/')
5885     path++;
5886   return path;
5887 }
5888 
5889 
5890 
5891 
5892 
5893 
5894 
5895 
5896 
5897 
5898 
5899 
5900 // Look up and return the inode for a path name.
5901 // If parent != 0, return the inode for the parent and copy the final
5902 // path element into name, which must have room for DIRSIZ bytes.
5903 // Must be called inside a transaction since it calls iput().
5904 static struct inode*
5905 namex(char *path, int nameiparent, char *name)
5906 {
5907   struct inode *ip, *next;
5908 
5909   if(*path == '/')
5910     ip = iget(ROOTDEV, ROOTINO);
5911   else
5912     ip = idup(myproc()->cwd);
5913 
5914   while((path = skipelem(path, name)) != 0){
5915     ilock(ip);
5916     if(ip->type != T_DIR){
5917       iunlockput(ip);
5918       return 0;
5919     }
5920     if(nameiparent && *path == '\0'){
5921       // Stop one level early.
5922       iunlock(ip);
5923       return ip;
5924     }
5925     if((next = dirlookup(ip, name, 0)) == 0){
5926       iunlockput(ip);
5927       return 0;
5928     }
5929     iunlockput(ip);
5930     ip = next;
5931   }
5932   if(nameiparent){
5933     iput(ip);
5934     return 0;
5935   }
5936   return ip;
5937 }
5938 
5939 struct inode*
5940 namei(char *path)
5941 {
5942   char name[DIRSIZ];
5943   return namex(path, 0, name);
5944 }
5945 
5946 
5947 
5948 
5949 
5950 struct inode*
5951 nameiparent(char *path, char *name)
5952 {
5953   return namex(path, 1, name);
5954 }
5955 
5956 
5957 
5958 
5959 
5960 
5961 
5962 
5963 
5964 
5965 
5966 
5967 
5968 
5969 
5970 
5971 
5972 
5973 
5974 
5975 
5976 
5977 
5978 
5979 
5980 
5981 
5982 
5983 
5984 
5985 
5986 
5987 
5988 
5989 
5990 
5991 
5992 
5993 
5994 
5995 
5996 
5997 
5998 
5999 
