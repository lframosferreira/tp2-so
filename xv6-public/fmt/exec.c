6750 #include "types.h"
6751 #include "param.h"
6752 #include "memlayout.h"
6753 #include "mmu.h"
6754 #include "proc.h"
6755 #include "defs.h"
6756 #include "x86.h"
6757 #include "elf.h"
6758 
6759 int
6760 exec(char *path, char **argv)
6761 {
6762   char *s, *last;
6763   int i, off;
6764   uint argc, sz, sp, ustack[3+MAXARG+1];
6765   struct elfhdr elf;
6766   struct inode *ip;
6767   struct proghdr ph;
6768   pde_t *pgdir, *oldpgdir;
6769   struct proc *curproc = myproc();
6770 
6771   begin_op();
6772 
6773   if((ip = namei(path)) == 0){
6774     end_op();
6775     cprintf("exec: fail\n");
6776     return -1;
6777   }
6778   ilock(ip);
6779   pgdir = 0;
6780 
6781   // Check ELF header
6782   if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
6783     goto bad;
6784   if(elf.magic != ELF_MAGIC)
6785     goto bad;
6786 
6787   if((pgdir = setupkvm()) == 0)
6788     goto bad;
6789 
6790   // Load program into memory.
6791   sz = 0;
6792   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
6793     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
6794       goto bad;
6795     if(ph.type != ELF_PROG_LOAD)
6796       continue;
6797     if(ph.memsz < ph.filesz)
6798       goto bad;
6799     if(ph.vaddr + ph.memsz < ph.vaddr)
6800       goto bad;
6801     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
6802       goto bad;
6803     if(ph.vaddr % PGSIZE != 0)
6804       goto bad;
6805     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
6806       goto bad;
6807   }
6808   iunlockput(ip);
6809   end_op();
6810   ip = 0;
6811 
6812   // Allocate two pages at the next page boundary.
6813   // Make the first inaccessible.  Use the second as the user stack.
6814   sz = PGROUNDUP(sz);
6815   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
6816     goto bad;
6817   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
6818   sp = sz;
6819 
6820   // Push argument strings, prepare rest of stack in ustack.
6821   for(argc = 0; argv[argc]; argc++) {
6822     if(argc >= MAXARG)
6823       goto bad;
6824     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
6825     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
6826       goto bad;
6827     ustack[3+argc] = sp;
6828   }
6829   ustack[3+argc] = 0;
6830 
6831   ustack[0] = 0xffffffff;  // fake return PC
6832   ustack[1] = argc;
6833   ustack[2] = sp - (argc+1)*4;  // argv pointer
6834 
6835   sp -= (3+argc+1) * 4;
6836   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
6837     goto bad;
6838 
6839   // Save program name for debugging.
6840   for(last=s=path; *s; s++)
6841     if(*s == '/')
6842       last = s+1;
6843   safestrcpy(curproc->name, last, sizeof(curproc->name));
6844 
6845   // Commit to the user image.
6846   oldpgdir = curproc->pgdir;
6847   curproc->pgdir = pgdir;
6848   curproc->sz = sz;
6849   curproc->tf->eip = elf.entry;  // main
6850   curproc->tf->esp = sp;
6851   switchuvm(curproc);
6852   freevm(oldpgdir);
6853   return 0;
6854 
6855  bad:
6856   if(pgdir)
6857     freevm(pgdir);
6858   if(ip){
6859     iunlockput(ip);
6860     end_op();
6861   }
6862   return -1;
6863 }
6864 
6865 
6866 
6867 
6868 
6869 
6870 
6871 
6872 
6873 
6874 
6875 
6876 
6877 
6878 
6879 
6880 
6881 
6882 
6883 
6884 
6885 
6886 
6887 
6888 
6889 
6890 
6891 
6892 
6893 
6894 
6895 
6896 
6897 
6898 
6899 
