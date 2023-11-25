9350 // Boot loader.
9351 //
9352 // Part of the boot block, along with bootasm.S, which calls bootmain().
9353 // bootasm.S has put the processor into protected 32-bit mode.
9354 // bootmain() loads an ELF kernel image from the disk starting at
9355 // sector 1 and then jumps to the kernel entry routine.
9356 
9357 #include "types.h"
9358 #include "elf.h"
9359 #include "x86.h"
9360 #include "memlayout.h"
9361 
9362 #define SECTSIZE  512
9363 
9364 void readseg(uchar*, uint, uint);
9365 
9366 void
9367 bootmain(void)
9368 {
9369   struct elfhdr *elf;
9370   struct proghdr *ph, *eph;
9371   void (*entry)(void);
9372   uchar* pa;
9373 
9374   elf = (struct elfhdr*)0x10000;  // scratch space
9375 
9376   // Read 1st page off disk
9377   readseg((uchar*)elf, 4096, 0);
9378 
9379   // Is this an ELF executable?
9380   if(elf->magic != ELF_MAGIC)
9381     return;  // let bootasm.S handle error
9382 
9383   // Load each program segment (ignores ph flags).
9384   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
9385   eph = ph + elf->phnum;
9386   for(; ph < eph; ph++){
9387     pa = (uchar*)ph->paddr;
9388     readseg(pa, ph->filesz, ph->off);
9389     if(ph->memsz > ph->filesz)
9390       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
9391   }
9392 
9393   // Call the entry point from the ELF header.
9394   // Does not return!
9395   entry = (void(*)(void))(elf->entry);
9396   entry();
9397 }
9398 
9399 
9400 void
9401 waitdisk(void)
9402 {
9403   // Wait for disk ready.
9404   while((inb(0x1F7) & 0xC0) != 0x40)
9405     ;
9406 }
9407 
9408 // Read a single sector at offset into dst.
9409 void
9410 readsect(void *dst, uint offset)
9411 {
9412   // Issue command.
9413   waitdisk();
9414   outb(0x1F2, 1);   // count = 1
9415   outb(0x1F3, offset);
9416   outb(0x1F4, offset >> 8);
9417   outb(0x1F5, offset >> 16);
9418   outb(0x1F6, (offset >> 24) | 0xE0);
9419   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
9420 
9421   // Read data.
9422   waitdisk();
9423   insl(0x1F0, dst, SECTSIZE/4);
9424 }
9425 
9426 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
9427 // Might copy more than asked.
9428 void
9429 readseg(uchar* pa, uint count, uint offset)
9430 {
9431   uchar* epa;
9432 
9433   epa = pa + count;
9434 
9435   // Round down to sector boundary.
9436   pa -= offset % SECTSIZE;
9437 
9438   // Translate from bytes to sectors; kernel starts at sector 1.
9439   offset = (offset / SECTSIZE) + 1;
9440 
9441   // If this is too slow, we could read lots of sectors at a time.
9442   // We'd write more to memory than asked, but it doesn't matter --
9443   // we load in increasing order.
9444   for(; pa < epa; pa += SECTSIZE, offset++)
9445     readsect(pa, offset);
9446 }
9447 
9448 
9449 
