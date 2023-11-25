4550 // Buffer cache.
4551 //
4552 // The buffer cache is a linked list of buf structures holding
4553 // cached copies of disk block contents.  Caching disk blocks
4554 // in memory reduces the number of disk reads and also provides
4555 // a synchronization point for disk blocks used by multiple processes.
4556 //
4557 // Interface:
4558 // * To get a buffer for a particular disk block, call bread.
4559 // * After changing buffer data, call bwrite to write it to disk.
4560 // * When done with the buffer, call brelse.
4561 // * Do not use the buffer after calling brelse.
4562 // * Only one process at a time can use a buffer,
4563 //     so do not keep them longer than necessary.
4564 //
4565 // The implementation uses two state flags internally:
4566 // * B_VALID: the buffer data has been read from the disk.
4567 // * B_DIRTY: the buffer data has been modified
4568 //     and needs to be written to disk.
4569 
4570 #include "types.h"
4571 #include "defs.h"
4572 #include "param.h"
4573 #include "spinlock.h"
4574 #include "sleeplock.h"
4575 #include "fs.h"
4576 #include "buf.h"
4577 
4578 struct {
4579   struct spinlock lock;
4580   struct buf buf[NBUF];
4581 
4582   // Linked list of all buffers, through prev/next.
4583   // head.next is most recently used.
4584   struct buf head;
4585 } bcache;
4586 
4587 void
4588 binit(void)
4589 {
4590   struct buf *b;
4591 
4592   initlock(&bcache.lock, "bcache");
4593 
4594 
4595 
4596 
4597 
4598 
4599 
4600   // Create linked list of buffers
4601   bcache.head.prev = &bcache.head;
4602   bcache.head.next = &bcache.head;
4603   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
4604     b->next = bcache.head.next;
4605     b->prev = &bcache.head;
4606     initsleeplock(&b->lock, "buffer");
4607     bcache.head.next->prev = b;
4608     bcache.head.next = b;
4609   }
4610 }
4611 
4612 // Look through buffer cache for block on device dev.
4613 // If not found, allocate a buffer.
4614 // In either case, return locked buffer.
4615 static struct buf*
4616 bget(uint dev, uint blockno)
4617 {
4618   struct buf *b;
4619 
4620   acquire(&bcache.lock);
4621 
4622   // Is the block already cached?
4623   for(b = bcache.head.next; b != &bcache.head; b = b->next){
4624     if(b->dev == dev && b->blockno == blockno){
4625       b->refcnt++;
4626       release(&bcache.lock);
4627       acquiresleep(&b->lock);
4628       return b;
4629     }
4630   }
4631 
4632   // Not cached; recycle an unused buffer.
4633   // Even if refcnt==0, B_DIRTY indicates a buffer is in use
4634   // because log.c has modified it but not yet committed it.
4635   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
4636     if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
4637       b->dev = dev;
4638       b->blockno = blockno;
4639       b->flags = 0;
4640       b->refcnt = 1;
4641       release(&bcache.lock);
4642       acquiresleep(&b->lock);
4643       return b;
4644     }
4645   }
4646   panic("bget: no buffers");
4647 }
4648 
4649 
4650 // Return a locked buf with the contents of the indicated block.
4651 struct buf*
4652 bread(uint dev, uint blockno)
4653 {
4654   struct buf *b;
4655 
4656   b = bget(dev, blockno);
4657   if((b->flags & B_VALID) == 0) {
4658     iderw(b);
4659   }
4660   return b;
4661 }
4662 
4663 // Write b's contents to disk.  Must be locked.
4664 void
4665 bwrite(struct buf *b)
4666 {
4667   if(!holdingsleep(&b->lock))
4668     panic("bwrite");
4669   b->flags |= B_DIRTY;
4670   iderw(b);
4671 }
4672 
4673 // Release a locked buffer.
4674 // Move to the head of the MRU list.
4675 void
4676 brelse(struct buf *b)
4677 {
4678   if(!holdingsleep(&b->lock))
4679     panic("brelse");
4680 
4681   releasesleep(&b->lock);
4682 
4683   acquire(&bcache.lock);
4684   b->refcnt--;
4685   if (b->refcnt == 0) {
4686     // no one is waiting for it.
4687     b->next->prev = b->prev;
4688     b->prev->next = b->next;
4689     b->next = bcache.head.next;
4690     b->prev = &bcache.head;
4691     bcache.head.next->prev = b;
4692     bcache.head.next = b;
4693   }
4694 
4695   release(&bcache.lock);
4696 }
4697 
4698 
4699 
4700 // Blank page.
4701 
4702 
4703 
4704 
4705 
4706 
4707 
4708 
4709 
4710 
4711 
4712 
4713 
4714 
4715 
4716 
4717 
4718 
4719 
4720 
4721 
4722 
4723 
4724 
4725 
4726 
4727 
4728 
4729 
4730 
4731 
4732 
4733 
4734 
4735 
4736 
4737 
4738 
4739 
4740 
4741 
4742 
4743 
4744 
4745 
4746 
4747 
4748 
4749 
