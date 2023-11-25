4350 // Simple PIO-based (non-DMA) IDE driver code.
4351 
4352 #include "types.h"
4353 #include "defs.h"
4354 #include "param.h"
4355 #include "memlayout.h"
4356 #include "mmu.h"
4357 #include "proc.h"
4358 #include "x86.h"
4359 #include "traps.h"
4360 #include "spinlock.h"
4361 #include "sleeplock.h"
4362 #include "fs.h"
4363 #include "buf.h"
4364 
4365 #define SECTOR_SIZE   512
4366 #define IDE_BSY       0x80
4367 #define IDE_DRDY      0x40
4368 #define IDE_DF        0x20
4369 #define IDE_ERR       0x01
4370 
4371 #define IDE_CMD_READ  0x20
4372 #define IDE_CMD_WRITE 0x30
4373 #define IDE_CMD_RDMUL 0xc4
4374 #define IDE_CMD_WRMUL 0xc5
4375 
4376 // idequeue points to the buf now being read/written to the disk.
4377 // idequeue->qnext points to the next buf to be processed.
4378 // You must hold idelock while manipulating queue.
4379 
4380 static struct spinlock idelock;
4381 static struct buf *idequeue;
4382 
4383 static int havedisk1;
4384 static void idestart(struct buf*);
4385 
4386 // Wait for IDE disk to become ready.
4387 static int
4388 idewait(int checkerr)
4389 {
4390   int r;
4391 
4392   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
4393     ;
4394   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
4395     return -1;
4396   return 0;
4397 }
4398 
4399 
4400 void
4401 ideinit(void)
4402 {
4403   int i;
4404 
4405   initlock(&idelock, "ide");
4406   ioapicenable(IRQ_IDE, ncpu - 1);
4407   idewait(0);
4408 
4409   // Check if disk 1 is present
4410   outb(0x1f6, 0xe0 | (1<<4));
4411   for(i=0; i<1000; i++){
4412     if(inb(0x1f7) != 0){
4413       havedisk1 = 1;
4414       break;
4415     }
4416   }
4417 
4418   // Switch back to disk 0.
4419   outb(0x1f6, 0xe0 | (0<<4));
4420 }
4421 
4422 // Start the request for b.  Caller must hold idelock.
4423 static void
4424 idestart(struct buf *b)
4425 {
4426   if(b == 0)
4427     panic("idestart");
4428   if(b->blockno >= FSSIZE)
4429     panic("incorrect blockno");
4430   int sector_per_block =  BSIZE/SECTOR_SIZE;
4431   int sector = b->blockno * sector_per_block;
4432   int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
4433   int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;
4434 
4435   if (sector_per_block > 7) panic("idestart");
4436 
4437   idewait(0);
4438   outb(0x3f6, 0);  // generate interrupt
4439   outb(0x1f2, sector_per_block);  // number of sectors
4440   outb(0x1f3, sector & 0xff);
4441   outb(0x1f4, (sector >> 8) & 0xff);
4442   outb(0x1f5, (sector >> 16) & 0xff);
4443   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
4444   if(b->flags & B_DIRTY){
4445     outb(0x1f7, write_cmd);
4446     outsl(0x1f0, b->data, BSIZE/4);
4447   } else {
4448     outb(0x1f7, read_cmd);
4449   }
4450 }
4451 
4452 // Interrupt handler.
4453 void
4454 ideintr(void)
4455 {
4456   struct buf *b;
4457 
4458   // First queued buffer is the active request.
4459   acquire(&idelock);
4460 
4461   if((b = idequeue) == 0){
4462     release(&idelock);
4463     return;
4464   }
4465   idequeue = b->qnext;
4466 
4467   // Read data if needed.
4468   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
4469     insl(0x1f0, b->data, BSIZE/4);
4470 
4471   // Wake process waiting for this buf.
4472   b->flags |= B_VALID;
4473   b->flags &= ~B_DIRTY;
4474   wakeup(b);
4475 
4476   // Start disk on next buf in queue.
4477   if(idequeue != 0)
4478     idestart(idequeue);
4479 
4480   release(&idelock);
4481 }
4482 
4483 
4484 
4485 
4486 
4487 
4488 
4489 
4490 
4491 
4492 
4493 
4494 
4495 
4496 
4497 
4498 
4499 
4500 // Sync buf with disk.
4501 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
4502 // Else if B_VALID is not set, read buf from disk, set B_VALID.
4503 void
4504 iderw(struct buf *b)
4505 {
4506   struct buf **pp;
4507 
4508   if(!holdingsleep(&b->lock))
4509     panic("iderw: buf not locked");
4510   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
4511     panic("iderw: nothing to do");
4512   if(b->dev != 0 && !havedisk1)
4513     panic("iderw: ide disk 1 not present");
4514 
4515   acquire(&idelock);  //DOC:acquire-lock
4516 
4517   // Append b to idequeue.
4518   b->qnext = 0;
4519   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
4520     ;
4521   *pp = b;
4522 
4523   // Start disk if necessary.
4524   if(idequeue == b)
4525     idestart(b);
4526 
4527   // Wait for request to finish.
4528   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
4529     sleep(b, &idelock);
4530   }
4531 
4532 
4533   release(&idelock);
4534 }
4535 
4536 
4537 
4538 
4539 
4540 
4541 
4542 
4543 
4544 
4545 
4546 
4547 
4548 
4549 
