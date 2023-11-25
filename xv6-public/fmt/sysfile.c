6200 //
6201 // File-system system calls.
6202 // Mostly argument checking, since we don't trust
6203 // user code, and calls into file.c and fs.c.
6204 //
6205 
6206 #include "types.h"
6207 #include "defs.h"
6208 #include "param.h"
6209 #include "stat.h"
6210 #include "mmu.h"
6211 #include "proc.h"
6212 #include "fs.h"
6213 #include "spinlock.h"
6214 #include "sleeplock.h"
6215 #include "file.h"
6216 #include "fcntl.h"
6217 
6218 // Fetch the nth word-sized system call argument as a file descriptor
6219 // and return both the descriptor and the corresponding struct file.
6220 static int
6221 argfd(int n, int *pfd, struct file **pf)
6222 {
6223   int fd;
6224   struct file *f;
6225 
6226   if(argint(n, &fd) < 0)
6227     return -1;
6228   if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
6229     return -1;
6230   if(pfd)
6231     *pfd = fd;
6232   if(pf)
6233     *pf = f;
6234   return 0;
6235 }
6236 
6237 
6238 
6239 
6240 
6241 
6242 
6243 
6244 
6245 
6246 
6247 
6248 
6249 
6250 // Allocate a file descriptor for the given file.
6251 // Takes over file reference from caller on success.
6252 static int
6253 fdalloc(struct file *f)
6254 {
6255   int fd;
6256   struct proc *curproc = myproc();
6257 
6258   for(fd = 0; fd < NOFILE; fd++){
6259     if(curproc->ofile[fd] == 0){
6260       curproc->ofile[fd] = f;
6261       return fd;
6262     }
6263   }
6264   return -1;
6265 }
6266 
6267 int
6268 sys_dup(void)
6269 {
6270   struct file *f;
6271   int fd;
6272 
6273   if(argfd(0, 0, &f) < 0)
6274     return -1;
6275   if((fd=fdalloc(f)) < 0)
6276     return -1;
6277   filedup(f);
6278   return fd;
6279 }
6280 
6281 int
6282 sys_read(void)
6283 {
6284   struct file *f;
6285   int n;
6286   char *p;
6287 
6288   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6289     return -1;
6290   return fileread(f, p, n);
6291 }
6292 
6293 
6294 
6295 
6296 
6297 
6298 
6299 
6300 int
6301 sys_write(void)
6302 {
6303   struct file *f;
6304   int n;
6305   char *p;
6306 
6307   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6308     return -1;
6309   return filewrite(f, p, n);
6310 }
6311 
6312 int
6313 sys_close(void)
6314 {
6315   int fd;
6316   struct file *f;
6317 
6318   if(argfd(0, &fd, &f) < 0)
6319     return -1;
6320   myproc()->ofile[fd] = 0;
6321   fileclose(f);
6322   return 0;
6323 }
6324 
6325 int
6326 sys_fstat(void)
6327 {
6328   struct file *f;
6329   struct stat *st;
6330 
6331   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
6332     return -1;
6333   return filestat(f, st);
6334 }
6335 
6336 
6337 
6338 
6339 
6340 
6341 
6342 
6343 
6344 
6345 
6346 
6347 
6348 
6349 
6350 // Create the path new as a link to the same inode as old.
6351 int
6352 sys_link(void)
6353 {
6354   char name[DIRSIZ], *new, *old;
6355   struct inode *dp, *ip;
6356 
6357   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
6358     return -1;
6359 
6360   begin_op();
6361   if((ip = namei(old)) == 0){
6362     end_op();
6363     return -1;
6364   }
6365 
6366   ilock(ip);
6367   if(ip->type == T_DIR){
6368     iunlockput(ip);
6369     end_op();
6370     return -1;
6371   }
6372 
6373   ip->nlink++;
6374   iupdate(ip);
6375   iunlock(ip);
6376 
6377   if((dp = nameiparent(new, name)) == 0)
6378     goto bad;
6379   ilock(dp);
6380   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
6381     iunlockput(dp);
6382     goto bad;
6383   }
6384   iunlockput(dp);
6385   iput(ip);
6386 
6387   end_op();
6388 
6389   return 0;
6390 
6391 bad:
6392   ilock(ip);
6393   ip->nlink--;
6394   iupdate(ip);
6395   iunlockput(ip);
6396   end_op();
6397   return -1;
6398 }
6399 
6400 // Is the directory dp empty except for "." and ".." ?
6401 static int
6402 isdirempty(struct inode *dp)
6403 {
6404   int off;
6405   struct dirent de;
6406 
6407   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
6408     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6409       panic("isdirempty: readi");
6410     if(de.inum != 0)
6411       return 0;
6412   }
6413   return 1;
6414 }
6415 
6416 
6417 
6418 
6419 
6420 
6421 
6422 
6423 
6424 
6425 
6426 
6427 
6428 
6429 
6430 
6431 
6432 
6433 
6434 
6435 
6436 
6437 
6438 
6439 
6440 
6441 
6442 
6443 
6444 
6445 
6446 
6447 
6448 
6449 
6450 int
6451 sys_unlink(void)
6452 {
6453   struct inode *ip, *dp;
6454   struct dirent de;
6455   char name[DIRSIZ], *path;
6456   uint off;
6457 
6458   if(argstr(0, &path) < 0)
6459     return -1;
6460 
6461   begin_op();
6462   if((dp = nameiparent(path, name)) == 0){
6463     end_op();
6464     return -1;
6465   }
6466 
6467   ilock(dp);
6468 
6469   // Cannot unlink "." or "..".
6470   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
6471     goto bad;
6472 
6473   if((ip = dirlookup(dp, name, &off)) == 0)
6474     goto bad;
6475   ilock(ip);
6476 
6477   if(ip->nlink < 1)
6478     panic("unlink: nlink < 1");
6479   if(ip->type == T_DIR && !isdirempty(ip)){
6480     iunlockput(ip);
6481     goto bad;
6482   }
6483 
6484   memset(&de, 0, sizeof(de));
6485   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6486     panic("unlink: writei");
6487   if(ip->type == T_DIR){
6488     dp->nlink--;
6489     iupdate(dp);
6490   }
6491   iunlockput(dp);
6492 
6493   ip->nlink--;
6494   iupdate(ip);
6495   iunlockput(ip);
6496 
6497   end_op();
6498 
6499   return 0;
6500 bad:
6501   iunlockput(dp);
6502   end_op();
6503   return -1;
6504 }
6505 
6506 static struct inode*
6507 create(char *path, short type, short major, short minor)
6508 {
6509   struct inode *ip, *dp;
6510   char name[DIRSIZ];
6511 
6512   if((dp = nameiparent(path, name)) == 0)
6513     return 0;
6514   ilock(dp);
6515 
6516   if((ip = dirlookup(dp, name, 0)) != 0){
6517     iunlockput(dp);
6518     ilock(ip);
6519     if(type == T_FILE && ip->type == T_FILE)
6520       return ip;
6521     iunlockput(ip);
6522     return 0;
6523   }
6524 
6525   if((ip = ialloc(dp->dev, type)) == 0)
6526     panic("create: ialloc");
6527 
6528   ilock(ip);
6529   ip->major = major;
6530   ip->minor = minor;
6531   ip->nlink = 1;
6532   iupdate(ip);
6533 
6534   if(type == T_DIR){  // Create . and .. entries.
6535     dp->nlink++;  // for ".."
6536     iupdate(dp);
6537     // No ip->nlink++ for ".": avoid cyclic ref count.
6538     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
6539       panic("create dots");
6540   }
6541 
6542   if(dirlink(dp, name, ip->inum) < 0)
6543     panic("create: dirlink");
6544 
6545   iunlockput(dp);
6546 
6547   return ip;
6548 }
6549 
6550 int
6551 sys_open(void)
6552 {
6553   char *path;
6554   int fd, omode;
6555   struct file *f;
6556   struct inode *ip;
6557 
6558   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
6559     return -1;
6560 
6561   begin_op();
6562 
6563   if(omode & O_CREATE){
6564     ip = create(path, T_FILE, 0, 0);
6565     if(ip == 0){
6566       end_op();
6567       return -1;
6568     }
6569   } else {
6570     if((ip = namei(path)) == 0){
6571       end_op();
6572       return -1;
6573     }
6574     ilock(ip);
6575     if(ip->type == T_DIR && omode != O_RDONLY){
6576       iunlockput(ip);
6577       end_op();
6578       return -1;
6579     }
6580   }
6581 
6582   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
6583     if(f)
6584       fileclose(f);
6585     iunlockput(ip);
6586     end_op();
6587     return -1;
6588   }
6589   iunlock(ip);
6590   end_op();
6591 
6592   f->type = FD_INODE;
6593   f->ip = ip;
6594   f->off = 0;
6595   f->readable = !(omode & O_WRONLY);
6596   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
6597   return fd;
6598 }
6599 
6600 int
6601 sys_mkdir(void)
6602 {
6603   char *path;
6604   struct inode *ip;
6605 
6606   begin_op();
6607   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
6608     end_op();
6609     return -1;
6610   }
6611   iunlockput(ip);
6612   end_op();
6613   return 0;
6614 }
6615 
6616 int
6617 sys_mknod(void)
6618 {
6619   struct inode *ip;
6620   char *path;
6621   int major, minor;
6622 
6623   begin_op();
6624   if((argstr(0, &path)) < 0 ||
6625      argint(1, &major) < 0 ||
6626      argint(2, &minor) < 0 ||
6627      (ip = create(path, T_DEV, major, minor)) == 0){
6628     end_op();
6629     return -1;
6630   }
6631   iunlockput(ip);
6632   end_op();
6633   return 0;
6634 }
6635 
6636 
6637 
6638 
6639 
6640 
6641 
6642 
6643 
6644 
6645 
6646 
6647 
6648 
6649 
6650 int
6651 sys_chdir(void)
6652 {
6653   char *path;
6654   struct inode *ip;
6655   struct proc *curproc = myproc();
6656 
6657   begin_op();
6658   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
6659     end_op();
6660     return -1;
6661   }
6662   ilock(ip);
6663   if(ip->type != T_DIR){
6664     iunlockput(ip);
6665     end_op();
6666     return -1;
6667   }
6668   iunlock(ip);
6669   iput(curproc->cwd);
6670   end_op();
6671   curproc->cwd = ip;
6672   return 0;
6673 }
6674 
6675 int
6676 sys_exec(void)
6677 {
6678   char *path, *argv[MAXARG];
6679   int i;
6680   uint uargv, uarg;
6681 
6682   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
6683     return -1;
6684   }
6685   memset(argv, 0, sizeof(argv));
6686   for(i=0;; i++){
6687     if(i >= NELEM(argv))
6688       return -1;
6689     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
6690       return -1;
6691     if(uarg == 0){
6692       argv[i] = 0;
6693       break;
6694     }
6695     if(fetchstr(uarg, &argv[i]) < 0)
6696       return -1;
6697   }
6698   return exec(path, argv);
6699 }
6700 int
6701 sys_pipe(void)
6702 {
6703   int *fd;
6704   struct file *rf, *wf;
6705   int fd0, fd1;
6706 
6707   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
6708     return -1;
6709   if(pipealloc(&rf, &wf) < 0)
6710     return -1;
6711   fd0 = -1;
6712   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
6713     if(fd0 >= 0)
6714       myproc()->ofile[fd0] = 0;
6715     fileclose(rf);
6716     fileclose(wf);
6717     return -1;
6718   }
6719   fd[0] = fd0;
6720   fd[1] = fd1;
6721   return 0;
6722 }
6723 
6724 
6725 
6726 
6727 
6728 
6729 
6730 
6731 
6732 
6733 
6734 
6735 
6736 
6737 
6738 
6739 
6740 
6741 
6742 
6743 
6744 
6745 
6746 
6747 
6748 
6749 
