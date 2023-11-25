4300 struct file {
4301   enum { FD_NONE, FD_PIPE, FD_INODE } type;
4302   int ref; // reference count
4303   char readable;
4304   char writable;
4305   struct pipe *pipe;
4306   struct inode *ip;
4307   uint off;
4308 };
4309 
4310 
4311 // in-memory copy of an inode
4312 struct inode {
4313   uint dev;           // Device number
4314   uint inum;          // Inode number
4315   int ref;            // Reference count
4316   struct sleeplock lock; // protects everything below here
4317   int valid;          // inode has been read from disk?
4318 
4319   short type;         // copy of disk inode
4320   short major;
4321   short minor;
4322   short nlink;
4323   uint size;
4324   uint addrs[NDIRECT+1];
4325 };
4326 
4327 // table mapping major device number to
4328 // device functions
4329 struct devsw {
4330   int (*read)(struct inode*, char*, int);
4331   int (*write)(struct inode*, char*, int);
4332 };
4333 
4334 extern struct devsw devsw[];
4335 
4336 #define CONSOLE 1
4337 
4338 
4339 
4340 
4341 
4342 
4343 
4344 
4345 
4346 
4347 
4348 
4349 
