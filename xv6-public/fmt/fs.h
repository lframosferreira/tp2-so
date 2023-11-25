4200 // On-disk file system format.
4201 // Both the kernel and user programs use this header file.
4202 
4203 
4204 #define ROOTINO 1  // root i-number
4205 #define BSIZE 512  // block size
4206 
4207 // Disk layout:
4208 // [ boot block | super block | log | inode blocks |
4209 //                                          free bit map | data blocks]
4210 //
4211 // mkfs computes the super block and builds an initial file system. The
4212 // super block describes the disk layout:
4213 struct superblock {
4214   uint size;         // Size of file system image (blocks)
4215   uint nblocks;      // Number of data blocks
4216   uint ninodes;      // Number of inodes.
4217   uint nlog;         // Number of log blocks
4218   uint logstart;     // Block number of first log block
4219   uint inodestart;   // Block number of first inode block
4220   uint bmapstart;    // Block number of first free map block
4221 };
4222 
4223 #define NDIRECT 12
4224 #define NINDIRECT (BSIZE / sizeof(uint))
4225 #define MAXFILE (NDIRECT + NINDIRECT)
4226 
4227 // On-disk inode structure
4228 struct dinode {
4229   short type;           // File type
4230   short major;          // Major device number (T_DEV only)
4231   short minor;          // Minor device number (T_DEV only)
4232   short nlink;          // Number of links to inode in file system
4233   uint size;            // Size of file (bytes)
4234   uint addrs[NDIRECT+1];   // Data block addresses
4235 };
4236 
4237 
4238 
4239 
4240 
4241 
4242 
4243 
4244 
4245 
4246 
4247 
4248 
4249 
4250 // Inodes per block.
4251 #define IPB           (BSIZE / sizeof(struct dinode))
4252 
4253 // Block containing inode i
4254 #define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)
4255 
4256 // Bitmap bits per block
4257 #define BPB           (BSIZE*8)
4258 
4259 // Block of free map containing bit for block b
4260 #define BBLOCK(b, sb) (b/BPB + sb.bmapstart)
4261 
4262 // Directory is a file containing a sequence of dirent structures.
4263 #define DIRSIZ 14
4264 
4265 struct dirent {
4266   ushort inum;
4267   char name[DIRSIZ];
4268 };
4269 
4270 
4271 
4272 
4273 
4274 
4275 
4276 
4277 
4278 
4279 
4280 
4281 
4282 
4283 
4284 
4285 
4286 
4287 
4288 
4289 
4290 
4291 
4292 
4293 
4294 
4295 
4296 
4297 
4298 
4299 
