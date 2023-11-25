3850 #include "types.h"
3851 #include "x86.h"
3852 #include "defs.h"
3853 #include "date.h"
3854 #include "param.h"
3855 #include "memlayout.h"
3856 #include "mmu.h"
3857 #include "proc.h"
3858 
3859 int
3860 sys_fork(void)
3861 {
3862   return fork();
3863 }
3864 
3865 int
3866 sys_exit(void)
3867 {
3868   exit();
3869   return 0;  // not reached
3870 }
3871 
3872 int
3873 sys_wait(void)
3874 {
3875   return wait();
3876 }
3877 
3878 int
3879 sys_kill(void)
3880 {
3881   int pid;
3882 
3883   if(argint(0, &pid) < 0)
3884     return -1;
3885   return kill(pid);
3886 }
3887 
3888 int
3889 sys_getpid(void)
3890 {
3891   return myproc()->pid;
3892 }
3893 
3894 
3895 
3896 
3897 
3898 
3899 
3900 int
3901 sys_sbrk(void)
3902 {
3903   int addr;
3904   int n;
3905 
3906   if(argint(0, &n) < 0)
3907     return -1;
3908   addr = myproc()->sz;
3909   if(growproc(n) < 0)
3910     return -1;
3911   return addr;
3912 }
3913 
3914 int
3915 sys_sleep(void)
3916 {
3917   int n;
3918   uint ticks0;
3919 
3920   if(argint(0, &n) < 0)
3921     return -1;
3922   acquire(&tickslock);
3923   ticks0 = ticks;
3924   while(ticks - ticks0 < n){
3925     if(myproc()->killed){
3926       release(&tickslock);
3927       return -1;
3928     }
3929     sleep(&ticks, &tickslock);
3930   }
3931   release(&tickslock);
3932   return 0;
3933 }
3934 
3935 // return how many clock tick interrupts have occurred
3936 // since start.
3937 int
3938 sys_uptime(void)
3939 {
3940   uint xticks;
3941 
3942   acquire(&tickslock);
3943   xticks = ticks;
3944   release(&tickslock);
3945   return xticks;
3946 }
3947 
3948 
3949 
3950 int sys_change_prio(void){
3951   int priority;
3952 
3953   if (argint(0, &priority) < 0)
3954     return -1;
3955 
3956   if(priority != 1 && priority != 2 && priority != 3)
3957     return -1;
3958 
3959   return change_prio(priority);
3960 }
3961 
3962 int sys_wait2(void){
3963   int *retime, *rutime, *stime;
3964 
3965   if (argptr(0, (char **)&retime, sizeof(int)) < 0)
3966     return -1;
3967   if (argptr(1, (char **)&rutime, sizeof(int)) < 0)
3968     return -1;
3969   if (argptr(2, (char **)&stime, sizeof(int)) < 0)
3970     return -1;
3971 
3972   return wait2((int *)retime, (int *)rutime, (int *)stime);
3973 }
3974 
3975 int sys_yield2(void){
3976   yield();
3977   return 1;
3978 }
3979 
3980 int sys_set_prio(void){
3981     return set_prio();
3982 }
3983 
3984 
3985 
3986 
3987 
3988 
3989 
3990 
3991 
3992 
3993 
3994 
3995 
3996 
3997 
3998 
3999 
