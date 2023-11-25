8600 #include "syscall.h"
8601 #include "traps.h"
8602 
8603 #define SYSCALL(name) \
8604   .globl name; \
8605   name: \
8606     movl $SYS_ ## name, %eax; \
8607     int $T_SYSCALL; \
8608     ret
8609 
8610 SYSCALL(fork)
8611 SYSCALL(exit)
8612 SYSCALL(wait)
8613 SYSCALL(pipe)
8614 SYSCALL(read)
8615 SYSCALL(write)
8616 SYSCALL(close)
8617 SYSCALL(kill)
8618 SYSCALL(exec)
8619 SYSCALL(open)
8620 SYSCALL(mknod)
8621 SYSCALL(unlink)
8622 SYSCALL(fstat)
8623 SYSCALL(link)
8624 SYSCALL(mkdir)
8625 SYSCALL(chdir)
8626 SYSCALL(dup)
8627 SYSCALL(getpid)
8628 SYSCALL(sbrk)
8629 SYSCALL(sleep)
8630 SYSCALL(uptime)
8631 
8632 SYSCALL(change_prio)
8633 SYSCALL(wait2)
8634 SYSCALL(yield2)
8635 SYSCALL(set_prio)
8636 
8637 
8638 
8639 
8640 
8641 
8642 
8643 
8644 
8645 
8646 
8647 
8648 
8649 
