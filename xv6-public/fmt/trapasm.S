3400 #include "mmu.h"
3401 
3402   # vectors.S sends all traps here.
3403 .globl alltraps
3404 alltraps:
3405   # Build trap frame.
3406   pushl %ds
3407   pushl %es
3408   pushl %fs
3409   pushl %gs
3410   pushal
3411 
3412   # Set up data segments.
3413   movw $(SEG_KDATA<<3), %ax
3414   movw %ax, %ds
3415   movw %ax, %es
3416 
3417   # Call trap(tf), where tf=%esp
3418   pushl %esp
3419   call trap
3420   addl $4, %esp
3421 
3422   # Return falls through to trapret...
3423 .globl trapret
3424 trapret:
3425   popal
3426   popl %gs
3427   popl %fs
3428   popl %es
3429   popl %ds
3430   addl $0x8, %esp  # trapno and errcode
3431   iret
3432 
3433 
3434 
3435 
3436 
3437 
3438 
3439 
3440 
3441 
3442 
3443 
3444 
3445 
3446 
3447 
3448 
3449 
