4850 #include "types.h"
4851 #include "defs.h"
4852 #include "param.h"
4853 #include "spinlock.h"
4854 #include "sleeplock.h"
4855 #include "fs.h"
4856 #include "buf.h"
4857 
4858 // Simple logging that allows concurrent FS system calls.
4859 //
4860 // A log transaction contains the updates of multiple FS system
4861 // calls. The logging system only commits when there are
4862 // no FS system calls active. Thus there is never
4863 // any reasoning required about whether a commit might
4864 // write an uncommitted system call's updates to disk.
4865 //
4866 // A system call should call begin_op()/end_op() to mark
4867 // its start and end. Usually begin_op() just increments
4868 // the count of in-progress FS system calls and returns.
4869 // But if it thinks the log is close to running out, it
4870 // sleeps until the last outstanding end_op() commits.
4871 //
4872 // The log is a physical re-do log containing disk blocks.
4873 // The on-disk log format:
4874 //   header block, containing block #s for block A, B, C, ...
4875 //   block A
4876 //   block B
4877 //   block C
4878 //   ...
4879 // Log appends are synchronous.
4880 
4881 // Contents of the header block, used for both the on-disk header block
4882 // and to keep track in memory of logged block# before commit.
4883 struct logheader {
4884   int n;
4885   int block[LOGSIZE];
4886 };
4887 
4888 struct log {
4889   struct spinlock lock;
4890   int start;
4891   int size;
4892   int outstanding; // how many FS sys calls are executing.
4893   int committing;  // in commit(), please wait.
4894   int dev;
4895   struct logheader lh;
4896 };
4897 
4898 
4899 
4900 struct log log;
4901 
4902 static void recover_from_log(void);
4903 static void commit();
4904 
4905 void
4906 initlog(int dev)
4907 {
4908   if (sizeof(struct logheader) >= BSIZE)
4909     panic("initlog: too big logheader");
4910 
4911   struct superblock sb;
4912   initlock(&log.lock, "log");
4913   readsb(dev, &sb);
4914   log.start = sb.logstart;
4915   log.size = sb.nlog;
4916   log.dev = dev;
4917   recover_from_log();
4918 }
4919 
4920 // Copy committed blocks from log to their home location
4921 static void
4922 install_trans(void)
4923 {
4924   int tail;
4925 
4926   for (tail = 0; tail < log.lh.n; tail++) {
4927     struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
4928     struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
4929     memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
4930     bwrite(dbuf);  // write dst to disk
4931     brelse(lbuf);
4932     brelse(dbuf);
4933   }
4934 }
4935 
4936 // Read the log header from disk into the in-memory log header
4937 static void
4938 read_head(void)
4939 {
4940   struct buf *buf = bread(log.dev, log.start);
4941   struct logheader *lh = (struct logheader *) (buf->data);
4942   int i;
4943   log.lh.n = lh->n;
4944   for (i = 0; i < log.lh.n; i++) {
4945     log.lh.block[i] = lh->block[i];
4946   }
4947   brelse(buf);
4948 }
4949 
4950 // Write in-memory log header to disk.
4951 // This is the true point at which the
4952 // current transaction commits.
4953 static void
4954 write_head(void)
4955 {
4956   struct buf *buf = bread(log.dev, log.start);
4957   struct logheader *hb = (struct logheader *) (buf->data);
4958   int i;
4959   hb->n = log.lh.n;
4960   for (i = 0; i < log.lh.n; i++) {
4961     hb->block[i] = log.lh.block[i];
4962   }
4963   bwrite(buf);
4964   brelse(buf);
4965 }
4966 
4967 static void
4968 recover_from_log(void)
4969 {
4970   read_head();
4971   install_trans(); // if committed, copy from log to disk
4972   log.lh.n = 0;
4973   write_head(); // clear the log
4974 }
4975 
4976 // called at the start of each FS system call.
4977 void
4978 begin_op(void)
4979 {
4980   acquire(&log.lock);
4981   while(1){
4982     if(log.committing){
4983       sleep(&log, &log.lock);
4984     } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
4985       // this op might exhaust log space; wait for commit.
4986       sleep(&log, &log.lock);
4987     } else {
4988       log.outstanding += 1;
4989       release(&log.lock);
4990       break;
4991     }
4992   }
4993 }
4994 
4995 
4996 
4997 
4998 
4999 
5000 // called at the end of each FS system call.
5001 // commits if this was the last outstanding operation.
5002 void
5003 end_op(void)
5004 {
5005   int do_commit = 0;
5006 
5007   acquire(&log.lock);
5008   log.outstanding -= 1;
5009   if(log.committing)
5010     panic("log.committing");
5011   if(log.outstanding == 0){
5012     do_commit = 1;
5013     log.committing = 1;
5014   } else {
5015     // begin_op() may be waiting for log space,
5016     // and decrementing log.outstanding has decreased
5017     // the amount of reserved space.
5018     wakeup(&log);
5019   }
5020   release(&log.lock);
5021 
5022   if(do_commit){
5023     // call commit w/o holding locks, since not allowed
5024     // to sleep with locks.
5025     commit();
5026     acquire(&log.lock);
5027     log.committing = 0;
5028     wakeup(&log);
5029     release(&log.lock);
5030   }
5031 }
5032 
5033 // Copy modified blocks from cache to log.
5034 static void
5035 write_log(void)
5036 {
5037   int tail;
5038 
5039   for (tail = 0; tail < log.lh.n; tail++) {
5040     struct buf *to = bread(log.dev, log.start+tail+1); // log block
5041     struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
5042     memmove(to->data, from->data, BSIZE);
5043     bwrite(to);  // write the log
5044     brelse(from);
5045     brelse(to);
5046   }
5047 }
5048 
5049 
5050 static void
5051 commit()
5052 {
5053   if (log.lh.n > 0) {
5054     write_log();     // Write modified blocks from cache to log
5055     write_head();    // Write header to disk -- the real commit
5056     install_trans(); // Now install writes to home locations
5057     log.lh.n = 0;
5058     write_head();    // Erase the transaction from the log
5059   }
5060 }
5061 
5062 // Caller has modified b->data and is done with the buffer.
5063 // Record the block number and pin in the cache with B_DIRTY.
5064 // commit()/write_log() will do the disk write.
5065 //
5066 // log_write() replaces bwrite(); a typical use is:
5067 //   bp = bread(...)
5068 //   modify bp->data[]
5069 //   log_write(bp)
5070 //   brelse(bp)
5071 void
5072 log_write(struct buf *b)
5073 {
5074   int i;
5075 
5076   if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
5077     panic("too big a transaction");
5078   if (log.outstanding < 1)
5079     panic("log_write outside of trans");
5080 
5081   acquire(&log.lock);
5082   for (i = 0; i < log.lh.n; i++) {
5083     if (log.lh.block[i] == b->blockno)   // log absorbtion
5084       break;
5085   }
5086   log.lh.block[i] = b->blockno;
5087   if (i == log.lh.n)
5088     log.lh.n++;
5089   b->flags |= B_DIRTY; // prevent eviction
5090   release(&log.lock);
5091 }
5092 
5093 
5094 
5095 
5096 
5097 
5098 
5099 
