3450 #include "types.h"
3451 #include "defs.h"
3452 #include "param.h"
3453 #include "memlayout.h"
3454 #include "mmu.h"
3455 #include "proc.h"
3456 #include "x86.h"
3457 #include "traps.h"
3458 #include "spinlock.h"
3459 
3460 // Interrupt descriptor table (shared by all CPUs).
3461 struct gatedesc idt[256];
3462 extern uint vectors[];  // in vectors.S: array of 256 entry pointers
3463 struct spinlock tickslock;
3464 uint ticks;
3465 
3466 void
3467 tvinit(void)
3468 {
3469   int i;
3470 
3471   for(i = 0; i < 256; i++)
3472     SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
3473   SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
3474 
3475   initlock(&tickslock, "time");
3476 }
3477 
3478 void
3479 idtinit(void)
3480 {
3481   lidt(idt, sizeof(idt));
3482 }
3483 
3484 
3485 
3486 
3487 
3488 
3489 
3490 
3491 
3492 
3493 
3494 
3495 
3496 
3497 
3498 
3499 
3500 void
3501 trap(struct trapframe *tf)
3502 {
3503   if(tf->trapno == T_SYSCALL){
3504     if(myproc()->killed)
3505       exit();
3506     myproc()->tf = tf;
3507     syscall();
3508     if(myproc()->killed)
3509       exit();
3510     return;
3511   }
3512 
3513   switch(tf->trapno){
3514   case T_IRQ0 + IRQ_TIMER:
3515     if(cpuid() == 0){
3516       acquire(&tickslock);
3517       ticks++;
3518       wakeup(&ticks);
3519       release(&tickslock);
3520     }
3521     lapiceoi();
3522     break;
3523   case T_IRQ0 + IRQ_IDE:
3524     ideintr();
3525     lapiceoi();
3526     break;
3527   case T_IRQ0 + IRQ_IDE+1:
3528     // Bochs generates spurious IDE1 interrupts.
3529     break;
3530   case T_IRQ0 + IRQ_KBD:
3531     kbdintr();
3532     lapiceoi();
3533     break;
3534   case T_IRQ0 + IRQ_COM1:
3535     uartintr();
3536     lapiceoi();
3537     break;
3538   case T_IRQ0 + 7:
3539   case T_IRQ0 + IRQ_SPURIOUS:
3540     cprintf("cpu%d: spurious interrupt at %x:%x\n",
3541             cpuid(), tf->cs, tf->eip);
3542     lapiceoi();
3543     break;
3544 
3545 
3546 
3547 
3548 
3549 
3550   default:
3551     if(myproc() == 0 || (tf->cs&3) == 0){
3552       // In kernel, it must be our mistake.
3553       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
3554               tf->trapno, cpuid(), tf->eip, rcr2());
3555       panic("trap");
3556     }
3557     // In user space, assume process misbehaved.
3558     cprintf("pid %d %s: trap %d err %d on cpu %d "
3559             "eip 0x%x addr 0x%x--kill proc\n",
3560             myproc()->pid, myproc()->name, tf->trapno,
3561             tf->err, cpuid(), tf->eip, rcr2());
3562     myproc()->killed = 1;
3563   }
3564 
3565   // Force process exit if it has been killed and is in user space.
3566   // (If it is still executing in the kernel, let it keep running
3567   // until it gets to the regular system call return.)
3568   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3569     exit();
3570 
3571   // Force process to give up CPU on clock tick.
3572   // If interrupts were on while locks held, would need to check nlock.
3573   if(myproc() && myproc()->state == RUNNING &&
3574      tf->trapno == T_IRQ0+IRQ_TIMER)
3575     {
3576       if (myproc()->time_slice != 0){
3577         myproc()->time_slice--;
3578       }else {
3579         yield();
3580       }
3581     }
3582 
3583   // Check if the process has been killed since we yielded
3584   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3585     exit();
3586 }
3587 
3588 
3589 
3590 
3591 
3592 
3593 
3594 
3595 
3596 
3597 
3598 
3599 
