6900 #include "types.h"
6901 #include "defs.h"
6902 #include "param.h"
6903 #include "mmu.h"
6904 #include "proc.h"
6905 #include "fs.h"
6906 #include "spinlock.h"
6907 #include "sleeplock.h"
6908 #include "file.h"
6909 
6910 #define PIPESIZE 512
6911 
6912 struct pipe {
6913   struct spinlock lock;
6914   char data[PIPESIZE];
6915   uint nread;     // number of bytes read
6916   uint nwrite;    // number of bytes written
6917   int readopen;   // read fd is still open
6918   int writeopen;  // write fd is still open
6919 };
6920 
6921 int
6922 pipealloc(struct file **f0, struct file **f1)
6923 {
6924   struct pipe *p;
6925 
6926   p = 0;
6927   *f0 = *f1 = 0;
6928   if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
6929     goto bad;
6930   if((p = (struct pipe*)kalloc()) == 0)
6931     goto bad;
6932   p->readopen = 1;
6933   p->writeopen = 1;
6934   p->nwrite = 0;
6935   p->nread = 0;
6936   initlock(&p->lock, "pipe");
6937   (*f0)->type = FD_PIPE;
6938   (*f0)->readable = 1;
6939   (*f0)->writable = 0;
6940   (*f0)->pipe = p;
6941   (*f1)->type = FD_PIPE;
6942   (*f1)->readable = 0;
6943   (*f1)->writable = 1;
6944   (*f1)->pipe = p;
6945   return 0;
6946 
6947 
6948 
6949 
6950  bad:
6951   if(p)
6952     kfree((char*)p);
6953   if(*f0)
6954     fileclose(*f0);
6955   if(*f1)
6956     fileclose(*f1);
6957   return -1;
6958 }
6959 
6960 void
6961 pipeclose(struct pipe *p, int writable)
6962 {
6963   acquire(&p->lock);
6964   if(writable){
6965     p->writeopen = 0;
6966     wakeup(&p->nread);
6967   } else {
6968     p->readopen = 0;
6969     wakeup(&p->nwrite);
6970   }
6971   if(p->readopen == 0 && p->writeopen == 0){
6972     release(&p->lock);
6973     kfree((char*)p);
6974   } else
6975     release(&p->lock);
6976 }
6977 
6978 
6979 int
6980 pipewrite(struct pipe *p, char *addr, int n)
6981 {
6982   int i;
6983 
6984   acquire(&p->lock);
6985   for(i = 0; i < n; i++){
6986     while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
6987       if(p->readopen == 0 || myproc()->killed){
6988         release(&p->lock);
6989         return -1;
6990       }
6991       wakeup(&p->nread);
6992       sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
6993     }
6994     p->data[p->nwrite++ % PIPESIZE] = addr[i];
6995   }
6996   wakeup(&p->nread);  //DOC: pipewrite-wakeup1
6997   release(&p->lock);
6998   return n;
6999 }
7000 int
7001 piperead(struct pipe *p, char *addr, int n)
7002 {
7003   int i;
7004 
7005   acquire(&p->lock);
7006   while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
7007     if(myproc()->killed){
7008       release(&p->lock);
7009       return -1;
7010     }
7011     sleep(&p->nread, &p->lock); //DOC: piperead-sleep
7012   }
7013   for(i = 0; i < n; i++){  //DOC: piperead-copy
7014     if(p->nread == p->nwrite)
7015       break;
7016     addr[i] = p->data[p->nread++ % PIPESIZE];
7017   }
7018   wakeup(&p->nwrite);  //DOC: piperead-wakeup
7019   release(&p->lock);
7020   return i;
7021 }
7022 
7023 
7024 
7025 
7026 
7027 
7028 
7029 
7030 
7031 
7032 
7033 
7034 
7035 
7036 
7037 
7038 
7039 
7040 
7041 
7042 
7043 
7044 
7045 
7046 
7047 
7048 
7049 
