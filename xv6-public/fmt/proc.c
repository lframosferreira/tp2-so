2400 #include "types.h"
2401 #include "defs.h"
2402 #include "param.h"
2403 #include "memlayout.h"
2404 #include "mmu.h"
2405 #include "x86.h"
2406 #include "proc.h"
2407 #include "spinlock.h"
2408 
2409 struct {
2410   struct spinlock lock;
2411   struct proc proc[NPROC];
2412 } ptable;
2413 
2414 static struct proc *initproc;
2415 
2416 int nextpid = 1;
2417 extern void forkret(void);
2418 extern void trapret(void);
2419 
2420 static void wakeup1(void *chan);
2421 
2422 void pinit(void) { initlock(&ptable.lock, "ptable"); }
2423 
2424 // Must be called with interrupts disabled
2425 int cpuid() { return mycpu() - cpus; }
2426 
2427 // Must be called with interrupts disabled to avoid the caller being
2428 // rescheduled between reading lapicid and running through the loop.
2429 struct cpu *mycpu(void) {
2430   int apicid, i;
2431 
2432   if (readeflags() & FL_IF)
2433     panic("mycpu called with interrupts enabled\n");
2434 
2435   apicid = lapicid();
2436   // APIC IDs are not guaranteed to be contiguous. Maybe we should have
2437   // a reverse map, or reserve a register to store &cpus[i].
2438   for (i = 0; i < ncpu; ++i) {
2439     if (cpus[i].apicid == apicid)
2440       return &cpus[i];
2441   }
2442   panic("unknown apicid\n");
2443 }
2444 
2445 
2446 
2447 
2448 
2449 
2450 // Disable interrupts so that we are not rescheduled
2451 // while reading proc from the cpu structure
2452 struct proc *myproc(void) {
2453   struct cpu *c;
2454   struct proc *p;
2455   pushcli();
2456   c = mycpu();
2457   p = c->proc;
2458   popcli();
2459   return p;
2460 }
2461 
2462 
2463 //  Look in the process table for an UNUSED proc.
2464 //  If found, change state to EMBRYO and initialize
2465 //  state required to run in the kernel.
2466 //  Otherwise return 0.
2467 static struct proc *allocproc(void) {
2468   struct proc *p;
2469   char *sp;
2470 
2471   acquire(&ptable.lock);
2472 
2473   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2474     if (p->state == UNUSED)
2475       goto found;
2476 
2477   release(&ptable.lock);
2478   return 0;
2479 
2480 found:
2481   p->state = EMBRYO;
2482   p->pid = nextpid++;
2483 
2484   p->priority = 2;
2485   p->retime = 0;
2486   p->rutime = 0;
2487   p->stime = 0;
2488   p->ctime = 0;
2489 
2490   release(&ptable.lock);
2491 
2492   // Allocate kernel stack.
2493   if ((p->kstack = kalloc()) == 0) {
2494     p->state = UNUSED;
2495     return 0;
2496   }
2497   sp = p->kstack + KSTACKSIZE;
2498 
2499 
2500   // Leave room for trap frame.
2501   sp -= sizeof *p->tf;
2502   p->tf = (struct trapframe *)sp;
2503 
2504   // Set up new context to start executing at forkret,
2505   // which returns to trapret.
2506   sp -= 4;
2507   *(uint *)sp = (uint)trapret;
2508 
2509   sp -= sizeof *p->context;
2510   p->context = (struct context *)sp;
2511   memset(p->context, 0, sizeof *p->context);
2512   p->context->eip = (uint)forkret;
2513 
2514   return p;
2515 }
2516 
2517 
2518 //  Set up first user process.
2519 void userinit(void) {
2520   struct proc *p;
2521   extern char _binary_initcode_start[], _binary_initcode_size[];
2522 
2523   p = allocproc();
2524 
2525   initproc = p;
2526   if ((p->pgdir = setupkvm()) == 0)
2527     panic("userinit: out of memory?");
2528   inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
2529   p->sz = PGSIZE;
2530   memset(p->tf, 0, sizeof(*p->tf));
2531   p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
2532   p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
2533   p->tf->es = p->tf->ds;
2534   p->tf->ss = p->tf->ds;
2535   p->tf->eflags = FL_IF;
2536   p->tf->esp = PGSIZE;
2537   p->tf->eip = 0; // beginning of initcode.S
2538 
2539   safestrcpy(p->name, "initcode", sizeof(p->name));
2540   p->cwd = namei("/");
2541 
2542   // this assignment to p->state lets other cores
2543   // run this process. the acquire forces the above
2544   // writes to be visible, and the lock is also needed
2545   // because the assignment might not be atomic.
2546   acquire(&ptable.lock);
2547 
2548   p->state = RUNNABLE;
2549 
2550   release(&ptable.lock);
2551 }
2552 
2553 // Grow current process's memory by n bytes.
2554 // Return 0 on success, -1 on failure.
2555 int growproc(int n) {
2556   uint sz;
2557   struct proc *curproc = myproc();
2558 
2559   sz = curproc->sz;
2560   if (n > 0) {
2561     if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
2562       return -1;
2563   } else if (n < 0) {
2564     if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
2565       return -1;
2566   }
2567   curproc->sz = sz;
2568   switchuvm(curproc);
2569   return 0;
2570 }
2571 
2572 // Create a new process copying p as the parent.
2573 // Sets up stack to return as if from system call.
2574 // Caller must set state of returned proc to RUNNABLE.
2575 int fork(void) {
2576   int i, pid;
2577   struct proc *np;
2578   struct proc *curproc = myproc();
2579 
2580   // Allocate process.
2581   if ((np = allocproc()) == 0) {
2582     return -1;
2583   }
2584 
2585   // Copy process state from proc.
2586   if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
2587     kfree(np->kstack);
2588     np->kstack = 0;
2589     np->state = UNUSED;
2590     return -1;
2591   }
2592   np->sz = curproc->sz;
2593   np->parent = curproc;
2594   *np->tf = *curproc->tf;
2595 
2596   // Clear %eax so that fork returns 0 in the child.
2597   np->tf->eax = 0;
2598 
2599 
2600   for (i = 0; i < NOFILE; i++)
2601     if (curproc->ofile[i])
2602       np->ofile[i] = filedup(curproc->ofile[i]);
2603   np->cwd = idup(curproc->cwd);
2604 
2605   safestrcpy(np->name, curproc->name, sizeof(curproc->name));
2606 
2607   pid = np->pid;
2608 
2609   acquire(&ptable.lock);
2610 
2611   np->state = RUNNABLE;
2612 
2613   np->priority = 2; // Prioridade inicial de todo processo criado
2614 
2615   release(&ptable.lock);
2616 
2617   return pid;
2618 }
2619 
2620 // Exit the current process.  Does not return.
2621 // An exited process remains in the zombie state
2622 // until its parent calls wait() to find out it exited.
2623 void exit(void) {
2624   struct proc *curproc = myproc();
2625   struct proc *p;
2626   int fd;
2627 
2628   if (curproc == initproc)
2629     panic("init exiting");
2630 
2631   // Close all open files.
2632   for (fd = 0; fd < NOFILE; fd++) {
2633     if (curproc->ofile[fd]) {
2634       fileclose(curproc->ofile[fd]);
2635       curproc->ofile[fd] = 0;
2636     }
2637   }
2638 
2639   begin_op();
2640   iput(curproc->cwd);
2641   end_op();
2642   curproc->cwd = 0;
2643 
2644   acquire(&ptable.lock);
2645 
2646   // Parent might be sleeping in wait().
2647   wakeup1(curproc->parent);
2648 
2649 
2650   // Pass abandoned children to init.
2651   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
2652     if (p->parent == curproc) {
2653       p->parent = initproc;
2654       if (p->state == ZOMBIE)
2655         wakeup1(initproc);
2656     }
2657   }
2658 
2659   // Jump into the scheduler, never to return.
2660   curproc->state = ZOMBIE;
2661   sched();
2662   panic("zombie exit");
2663 }
2664 
2665 // Wait for a child process to exit and return its pid.
2666 // Return -1 if this process has no children.
2667 int wait(void) {
2668   struct proc *p;
2669   int havekids, pid;
2670   struct proc *curproc = myproc();
2671 
2672   acquire(&ptable.lock);
2673   for (;;) {
2674     // Scan through table looking for exited children.
2675     havekids = 0;
2676     for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
2677       if (p->parent != curproc)
2678         continue;
2679       havekids = 1;
2680       if (p->state == ZOMBIE) {
2681         // Found one.
2682         pid = p->pid;
2683         kfree(p->kstack);
2684         p->kstack = 0;
2685         freevm(p->pgdir);
2686         p->pid = 0;
2687         p->parent = 0;
2688         p->name[0] = 0;
2689         p->killed = 0;
2690         p->state = UNUSED;
2691         release(&ptable.lock);
2692         return pid;
2693       }
2694     }
2695 
2696 
2697 
2698 
2699 
2700     // No point waiting if we don't have any children.
2701     if (!havekids || curproc->killed) {
2702       release(&ptable.lock);
2703       return -1;
2704     }
2705 
2706     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
2707     sleep(curproc, &ptable.lock); // DOC: wait-sleep
2708   }
2709 }
2710 
2711 
2712 
2713 
2714 
2715 
2716 
2717 
2718 
2719 
2720 
2721 
2722 
2723 
2724 
2725 
2726 
2727 
2728 
2729 
2730 
2731 
2732 
2733 
2734 
2735 
2736 
2737 
2738 
2739 
2740 
2741 
2742 
2743 
2744 
2745 
2746 
2747 
2748 
2749 
2750 //  Per-CPU process scheduler.
2751 //  Each CPU calls scheduler() after setting itself up.
2752 //  Scheduler never returns.  It loops, doing:
2753 //   - choose a process to run
2754 //   - swtch to start running that process
2755 //   - eventually that process transfers control
2756 //       via swtch back to the scheduler.
2757 void scheduler(void) {
2758   struct proc *p;
2759   struct proc *highest_priority_p = 0;
2760   struct cpu *c = mycpu();
2761   c->proc = 0;
2762 
2763   /* for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2764     cprintf( "%s: %d\n", p->name, p->priority);
2765   } */
2766 
2767   for (;;) {
2768     // Enable interrupts on this processor.
2769     sti();
2770 
2771     // Loop over process table looking for process to run.
2772     acquire(&ptable.lock);
2773 
2774     for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
2775       if (p->state != RUNNABLE)
2776         continue;
2777 
2778       // Achei primeiro processo que é RUNNABLE, e ele, por enquanto, é o de maior prioridade
2779       highest_priority_p = p;
2780 
2781       // Checo a lista pra ver se encontro algum processo de prioridade maior que a do atual
2782       for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2783         if (p->state != RUNNABLE)
2784           continue;
2785         // Encontrei alguém maior
2786         if (p->priority > highest_priority_p->priority){
2787           highest_priority_p = p;
2788         }
2789       }
2790 
2791       // Se highest_priority_p for NULL, quer dizer que nenhum processo é RUNNABLE, então loop denovo
2792       if (highest_priority_p == 0){
2793         break;
2794       }
2795 
2796       p = highest_priority_p;
2797 
2798 
2799 
2800       // Switch to chosen process.  It is the process's job
2801       // to release ptable.lock and then reacquire it
2802       // before jumping back to us.
2803       c->proc = p;
2804       switchuvm(p);
2805 
2806       p->state = RUNNING;
2807       p->time_slice = INTERV;
2808       p->rutime++; // Aumento running time count do processo
2809 
2810       swtch(&(c->scheduler), p->context);
2811       switchkvm();
2812 
2813       // Process is done running for now.
2814       // It should have changed its p->state before coming back.
2815       c->proc = 0;
2816 
2817       /* Ao final da execução daquele processo, passo mais uma vez por cada
2818       processo e faço as checagens necessárias, isto é, checo se limite de tempo
2819       de espera passou para atualizar prioridade e somo tempo de espera. */
2820       struct proc *waiting_p;
2821       for (waiting_p = ptable.proc; waiting_p < &ptable.proc[NPROC];
2822            waiting_p++) {
2823         waiting_p->ctime++; // tempo de turnaround?
2824         if (waiting_p->pid != p->pid) {
2825           if (waiting_p->state == SLEEPING){
2826             waiting_p->stime++;
2827           } else { // Caso READY/RUNNABLE
2828             waiting_p->retime++;
2829           }
2830         }
2831         if (waiting_p->priority == 2 && waiting_p->retime >= P2TO3) {
2832           // waiting_p->retime = 0;
2833           waiting_p->priority = 3;
2834         }
2835         if (waiting_p->priority == 1 && waiting_p->retime >= P1TO2) {
2836           // waiting_p->retime = 0;
2837           waiting_p->priority = 2;
2838         }
2839       }
2840     }
2841 
2842     release(&ptable.lock);
2843   }
2844 }
2845 
2846 
2847 
2848 
2849 
2850 // Enter scheduler.  Must hold only ptable.lock
2851 // and have changed proc->state. Saves and restores
2852 // intena because intena is a property of this
2853 // kernel thread, not this CPU. It should
2854 // be proc->intena and proc->ncli, but that would
2855 // break in the few places where a lock is held but
2856 // there's no process.
2857 void sched(void) {
2858   int intena;
2859   struct proc *p = myproc();
2860 
2861   if (!holding(&ptable.lock))
2862     panic("sched ptable.lock");
2863   if (mycpu()->ncli != 1)
2864     panic("sched locks");
2865   if (p->state == RUNNING)
2866     panic("sched running");
2867   if (readeflags() & FL_IF)
2868     panic("sched interruptible");
2869   intena = mycpu()->intena;
2870   swtch(&p->context, mycpu()->scheduler);
2871   mycpu()->intena = intena;
2872 }
2873 
2874 // Give up the CPU for one scheduling round.
2875 void yield(void) {
2876   acquire(&ptable.lock); // DOC: yieldlock
2877   myproc()->state = RUNNABLE;
2878   sched();
2879   release(&ptable.lock);
2880 }
2881 
2882 // A fork child's very first scheduling by scheduler()
2883 // will swtch here.  "Return" to user space.
2884 void forkret(void) {
2885   static int first = 1;
2886   // Still holding ptable.lock from scheduler.
2887   release(&ptable.lock);
2888 
2889   if (first) {
2890     // Some initialization functions must be run in the context
2891     // of a regular process (e.g., they call sleep), and thus cannot
2892     // be run from main().
2893     first = 0;
2894     iinit(ROOTDEV);
2895     initlog(ROOTDEV);
2896   }
2897 
2898   // Return to "caller", actually trapret (see allocproc).
2899 }
2900 // Atomically release lock and sleep on chan.
2901 // Reacquires lock when awakened.
2902 void sleep(void *chan, struct spinlock *lk) {
2903   struct proc *p = myproc();
2904 
2905   if (p == 0)
2906     panic("sleep");
2907 
2908   if (lk == 0)
2909     panic("sleep without lk");
2910 
2911   // Must acquire ptable.lock in order to
2912   // change p->state and then call sched.
2913   // Once we hold ptable.lock, we can be
2914   // guaranteed that we won't miss any wakeup
2915   // (wakeup runs with ptable.lock locked),
2916   // so it's okay to release lk.
2917   if (lk != &ptable.lock) { // DOC: sleeplock0
2918     acquire(&ptable.lock);  // DOC: sleeplock1
2919     release(lk);
2920   }
2921   // Go to sleep.
2922   p->chan = chan;
2923   p->state = SLEEPING;
2924 
2925   sched();
2926 
2927   // Tidy up.
2928   p->chan = 0;
2929 
2930   // Reacquire original lock.
2931   if (lk != &ptable.lock) { // DOC: sleeplock2
2932     release(&ptable.lock);
2933     acquire(lk);
2934   }
2935 }
2936 
2937 
2938 
2939 
2940 
2941 
2942 
2943 
2944 
2945 
2946 
2947 
2948 
2949 
2950 //  Wake up all processes sleeping on chan.
2951 //  The ptable lock must be held.
2952 static void wakeup1(void *chan) {
2953   struct proc *p;
2954 
2955   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2956     if (p->state == SLEEPING && p->chan == chan)
2957       p->state = RUNNABLE;
2958 }
2959 
2960 // Wake up all processes sleeping on chan.
2961 void wakeup(void *chan) {
2962   acquire(&ptable.lock);
2963   wakeup1(chan);
2964   release(&ptable.lock);
2965 }
2966 
2967 // Kill the process with the given pid.
2968 // Process won't exit until it returns
2969 // to user space (see trap in trap.c).
2970 int kill(int pid) {
2971   struct proc *p;
2972 
2973   acquire(&ptable.lock);
2974   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
2975     if (p->pid == pid) {
2976       p->killed = 1;
2977       // Wake process from sleep if necessary.
2978       if (p->state == SLEEPING)
2979         p->state = RUNNABLE;
2980       release(&ptable.lock);
2981       return 0;
2982     }
2983   }
2984   release(&ptable.lock);
2985   return -1;
2986 }
2987 
2988 
2989 
2990 
2991 
2992 
2993 
2994 
2995 
2996 
2997 
2998 
2999 
3000 //  Print a process listing to console.  For debugging.
3001 //  Runs when user types ^P on console.
3002 //  No lock to avoid wedging a stuck machine further.
3003 void procdump(void) {
3004   static char *states[] = {
3005       [UNUSED] "unused",   [EMBRYO] "embryo",  [SLEEPING] "sleep ",
3006       [RUNNABLE] "runble", [RUNNING] "run   ", [ZOMBIE] "zombie"};
3007   int i;
3008   struct proc *p;
3009   char *state;
3010   uint pc[10];
3011 
3012   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
3013     if (p->state == UNUSED)
3014       continue;
3015     if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
3016       state = states[p->state];
3017     else
3018       state = "???";
3019     cprintf("%d %s %s", p->pid, state, p->name);
3020     if (p->state == SLEEPING) {
3021       getcallerpcs((uint *)p->context->ebp + 2, pc);
3022       for (i = 0; i < 10 && pc[i] != 0; i++)
3023         cprintf(" %p", pc[i]);
3024     }
3025     cprintf("\n");
3026   }
3027 }
3028 
3029 int change_prio(int priority) {
3030   int pid = myproc()->pid;
3031 
3032   struct proc *p;
3033   acquire(&ptable.lock);
3034   for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
3035     if (p->pid == pid) {
3036 
3037       p->priority = priority;
3038 
3039       release(&ptable.lock);
3040       return 0;
3041     }
3042   }
3043   release(&ptable.lock);
3044   cprintf("Couldn't find process with pid: %d\n in change_prio()", pid);
3045   return -1;
3046 }
3047 
3048 
3049 
3050 int wait2(int *retime, int *rutime, int *stime) {
3051   struct proc *p;
3052   int havekids, pid;
3053   struct proc *curproc = myproc();
3054 
3055   acquire(&ptable.lock);
3056   for (;;) {
3057     // Scan through table looking for exited children.
3058     havekids = 0;
3059     for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
3060       if (p->parent != curproc)
3061         continue;
3062       havekids = 1;
3063       if (p->state == ZOMBIE) {
3064         // Found one.
3065         pid = p->pid;
3066         kfree(p->kstack);
3067         p->kstack = 0;
3068         freevm(p->pgdir);
3069         p->pid = 0;
3070         p->parent = 0;
3071         p->name[0] = 0;
3072         p->killed = 0;
3073         p->state = UNUSED;
3074 
3075         *retime = p->retime;
3076         *rutime = p->rutime;
3077         *stime = p->stime;
3078         p->priority = 1;
3079         p->retime = 0;
3080         p->rutime = 0;
3081         p->stime = 0;
3082 
3083         release(&ptable.lock);
3084         return pid;
3085       }
3086     }
3087 
3088     // No point waiting if we don't have any children.
3089     if (!havekids || curproc->killed) {
3090       release(&ptable.lock);
3091       return -1;
3092     }
3093 
3094     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
3095     sleep(curproc, &ptable.lock); // DOC: wait-sleep
3096   }
3097 }
3098 
3099 
3100 int set_prio(void) {
3101   int i, pid;
3102   struct proc *np;
3103   struct proc *curproc = myproc();
3104 
3105   // Allocate process.
3106   if ((np = allocproc()) == 0) {
3107     return -1;
3108   }
3109 
3110   // Copy process state from proc.
3111   if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
3112     kfree(np->kstack);
3113     np->kstack = 0;
3114     np->state = UNUSED;
3115     return -1;
3116   }
3117   np->sz = curproc->sz;
3118   np->parent = curproc;
3119   *np->tf = *curproc->tf;
3120 
3121   // Clear %eax so that fork returns 0 in the child.
3122   np->tf->eax = 0;
3123 
3124   for (i = 0; i < NOFILE; i++)
3125     if (curproc->ofile[i])
3126       np->ofile[i] = filedup(curproc->ofile[i]);
3127   np->cwd = idup(curproc->cwd);
3128 
3129   safestrcpy(np->name, curproc->name, sizeof(curproc->name));
3130 
3131   pid = np->pid;
3132 
3133   acquire(&ptable.lock);
3134 
3135   np->state = RUNNABLE;
3136 
3137   np->priority = (pid % 3) + 1;
3138 
3139   release(&ptable.lock);
3140 
3141   return pid;
3142 }
3143 
3144 
3145 
3146 
3147 
3148 
3149 
