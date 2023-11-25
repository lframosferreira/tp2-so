8550 # Initial process execs /init.
8551 # This code runs in user space.
8552 
8553 #include "syscall.h"
8554 #include "traps.h"
8555 
8556 
8557 # exec(init, argv)
8558 .globl start
8559 start:
8560   pushl $argv
8561   pushl $init
8562   pushl $0  // where caller pc would be
8563   movl $SYS_exec, %eax
8564   int $T_SYSCALL
8565 
8566 # for(;;) exit();
8567 exit:
8568   movl $SYS_exit, %eax
8569   int $T_SYSCALL
8570   jmp exit
8571 
8572 # char init[] = "/init\0";
8573 init:
8574   .string "/init\0"
8575 
8576 # char *argv[] = { init, 0 };
8577 .p2align 2
8578 argv:
8579   .long init
8580   .long 0
8581 
8582 
8583 
8584 
8585 
8586 
8587 
8588 
8589 
8590 
8591 
8592 
8593 
8594 
8595 
8596 
8597 
8598 
8599 
