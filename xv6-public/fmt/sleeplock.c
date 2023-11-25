4750 // Sleeping locks
4751 
4752 #include "types.h"
4753 #include "defs.h"
4754 #include "param.h"
4755 #include "x86.h"
4756 #include "memlayout.h"
4757 #include "mmu.h"
4758 #include "proc.h"
4759 #include "spinlock.h"
4760 #include "sleeplock.h"
4761 
4762 void
4763 initsleeplock(struct sleeplock *lk, char *name)
4764 {
4765   initlock(&lk->lk, "sleep lock");
4766   lk->name = name;
4767   lk->locked = 0;
4768   lk->pid = 0;
4769 }
4770 
4771 void
4772 acquiresleep(struct sleeplock *lk)
4773 {
4774   acquire(&lk->lk);
4775   while (lk->locked) {
4776     sleep(lk, &lk->lk);
4777   }
4778   lk->locked = 1;
4779   lk->pid = myproc()->pid;
4780   release(&lk->lk);
4781 }
4782 
4783 void
4784 releasesleep(struct sleeplock *lk)
4785 {
4786   acquire(&lk->lk);
4787   lk->locked = 0;
4788   lk->pid = 0;
4789   wakeup(lk);
4790   release(&lk->lk);
4791 }
4792 
4793 
4794 
4795 
4796 
4797 
4798 
4799 
4800 int
4801 holdingsleep(struct sleeplock *lk)
4802 {
4803   int r;
4804 
4805   acquire(&lk->lk);
4806   r = lk->locked && (lk->pid == myproc()->pid);
4807   release(&lk->lk);
4808   return r;
4809 }
4810 
4811 
4812 
4813 
4814 
4815 
4816 
4817 
4818 
4819 
4820 
4821 
4822 
4823 
4824 
4825 
4826 
4827 
4828 
4829 
4830 
4831 
4832 
4833 
4834 
4835 
4836 
4837 
4838 
4839 
4840 
4841 
4842 
4843 
4844 
4845 
4846 
4847 
4848 
4849 
