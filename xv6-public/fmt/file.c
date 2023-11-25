6000 //
6001 // File descriptors
6002 //
6003 
6004 #include "types.h"
6005 #include "defs.h"
6006 #include "param.h"
6007 #include "fs.h"
6008 #include "spinlock.h"
6009 #include "sleeplock.h"
6010 #include "file.h"
6011 
6012 struct devsw devsw[NDEV];
6013 struct {
6014   struct spinlock lock;
6015   struct file file[NFILE];
6016 } ftable;
6017 
6018 void
6019 fileinit(void)
6020 {
6021   initlock(&ftable.lock, "ftable");
6022 }
6023 
6024 // Allocate a file structure.
6025 struct file*
6026 filealloc(void)
6027 {
6028   struct file *f;
6029 
6030   acquire(&ftable.lock);
6031   for(f = ftable.file; f < ftable.file + NFILE; f++){
6032     if(f->ref == 0){
6033       f->ref = 1;
6034       release(&ftable.lock);
6035       return f;
6036     }
6037   }
6038   release(&ftable.lock);
6039   return 0;
6040 }
6041 
6042 
6043 
6044 
6045 
6046 
6047 
6048 
6049 
6050 // Increment ref count for file f.
6051 struct file*
6052 filedup(struct file *f)
6053 {
6054   acquire(&ftable.lock);
6055   if(f->ref < 1)
6056     panic("filedup");
6057   f->ref++;
6058   release(&ftable.lock);
6059   return f;
6060 }
6061 
6062 // Close file f.  (Decrement ref count, close when reaches 0.)
6063 void
6064 fileclose(struct file *f)
6065 {
6066   struct file ff;
6067 
6068   acquire(&ftable.lock);
6069   if(f->ref < 1)
6070     panic("fileclose");
6071   if(--f->ref > 0){
6072     release(&ftable.lock);
6073     return;
6074   }
6075   ff = *f;
6076   f->ref = 0;
6077   f->type = FD_NONE;
6078   release(&ftable.lock);
6079 
6080   if(ff.type == FD_PIPE)
6081     pipeclose(ff.pipe, ff.writable);
6082   else if(ff.type == FD_INODE){
6083     begin_op();
6084     iput(ff.ip);
6085     end_op();
6086   }
6087 }
6088 
6089 
6090 
6091 
6092 
6093 
6094 
6095 
6096 
6097 
6098 
6099 
6100 // Get metadata about file f.
6101 int
6102 filestat(struct file *f, struct stat *st)
6103 {
6104   if(f->type == FD_INODE){
6105     ilock(f->ip);
6106     stati(f->ip, st);
6107     iunlock(f->ip);
6108     return 0;
6109   }
6110   return -1;
6111 }
6112 
6113 // Read from file f.
6114 int
6115 fileread(struct file *f, char *addr, int n)
6116 {
6117   int r;
6118 
6119   if(f->readable == 0)
6120     return -1;
6121   if(f->type == FD_PIPE)
6122     return piperead(f->pipe, addr, n);
6123   if(f->type == FD_INODE){
6124     ilock(f->ip);
6125     if((r = readi(f->ip, addr, f->off, n)) > 0)
6126       f->off += r;
6127     iunlock(f->ip);
6128     return r;
6129   }
6130   panic("fileread");
6131 }
6132 
6133 
6134 
6135 
6136 
6137 
6138 
6139 
6140 
6141 
6142 
6143 
6144 
6145 
6146 
6147 
6148 
6149 
6150 // Write to file f.
6151 int
6152 filewrite(struct file *f, char *addr, int n)
6153 {
6154   int r;
6155 
6156   if(f->writable == 0)
6157     return -1;
6158   if(f->type == FD_PIPE)
6159     return pipewrite(f->pipe, addr, n);
6160   if(f->type == FD_INODE){
6161     // write a few blocks at a time to avoid exceeding
6162     // the maximum log transaction size, including
6163     // i-node, indirect block, allocation blocks,
6164     // and 2 blocks of slop for non-aligned writes.
6165     // this really belongs lower down, since writei()
6166     // might be writing a device like the console.
6167     int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
6168     int i = 0;
6169     while(i < n){
6170       int n1 = n - i;
6171       if(n1 > max)
6172         n1 = max;
6173 
6174       begin_op();
6175       ilock(f->ip);
6176       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
6177         f->off += r;
6178       iunlock(f->ip);
6179       end_op();
6180 
6181       if(r < 0)
6182         break;
6183       if(r != n1)
6184         panic("short filewrite");
6185       i += r;
6186     }
6187     return i == n ? n : -1;
6188   }
6189   panic("filewrite");
6190 }
6191 
6192 
6193 
6194 
6195 
6196 
6197 
6198 
6199 
