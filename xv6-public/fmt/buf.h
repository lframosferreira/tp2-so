4000 struct buf {
4001   int flags;
4002   uint dev;
4003   uint blockno;
4004   struct sleeplock lock;
4005   uint refcnt;
4006   struct buf *prev; // LRU cache list
4007   struct buf *next;
4008   struct buf *qnext; // disk queue
4009   uchar data[BSIZE];
4010 };
4011 #define B_VALID 0x2  // buffer has been read from disk
4012 #define B_DIRTY 0x4  // buffer needs to be written to disk
4013 
4014 
4015 
4016 
4017 
4018 
4019 
4020 
4021 
4022 
4023 
4024 
4025 
4026 
4027 
4028 
4029 
4030 
4031 
4032 
4033 
4034 
4035 
4036 
4037 
4038 
4039 
4040 
4041 
4042 
4043 
4044 
4045 
4046 
4047 
4048 
4049 
