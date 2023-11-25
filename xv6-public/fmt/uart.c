8450 // Intel 8250 serial port (UART).
8451 
8452 #include "types.h"
8453 #include "defs.h"
8454 #include "param.h"
8455 #include "traps.h"
8456 #include "spinlock.h"
8457 #include "sleeplock.h"
8458 #include "fs.h"
8459 #include "file.h"
8460 #include "mmu.h"
8461 #include "proc.h"
8462 #include "x86.h"
8463 
8464 #define COM1    0x3f8
8465 
8466 static int uart;    // is there a uart?
8467 
8468 void
8469 uartinit(void)
8470 {
8471   char *p;
8472 
8473   // Turn off the FIFO
8474   outb(COM1+2, 0);
8475 
8476   // 9600 baud, 8 data bits, 1 stop bit, parity off.
8477   outb(COM1+3, 0x80);    // Unlock divisor
8478   outb(COM1+0, 115200/9600);
8479   outb(COM1+1, 0);
8480   outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
8481   outb(COM1+4, 0);
8482   outb(COM1+1, 0x01);    // Enable receive interrupts.
8483 
8484   // If status is 0xFF, no serial port.
8485   if(inb(COM1+5) == 0xFF)
8486     return;
8487   uart = 1;
8488 
8489   // Acknowledge pre-existing interrupt conditions;
8490   // enable interrupts.
8491   inb(COM1+2);
8492   inb(COM1+0);
8493   ioapicenable(IRQ_COM1, 0);
8494 
8495   // Announce that we're here.
8496   for(p="xv6...\n"; *p; p++)
8497     uartputc(*p);
8498 }
8499 
8500 void
8501 uartputc(int c)
8502 {
8503   int i;
8504 
8505   if(!uart)
8506     return;
8507   for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
8508     microdelay(10);
8509   outb(COM1+0, c);
8510 }
8511 
8512 static int
8513 uartgetc(void)
8514 {
8515   if(!uart)
8516     return -1;
8517   if(!(inb(COM1+5) & 0x01))
8518     return -1;
8519   return inb(COM1+0);
8520 }
8521 
8522 void
8523 uartintr(void)
8524 {
8525   consoleintr(uartgetc);
8526 }
8527 
8528 
8529 
8530 
8531 
8532 
8533 
8534 
8535 
8536 
8537 
8538 
8539 
8540 
8541 
8542 
8543 
8544 
8545 
8546 
8547 
8548 
8549 
