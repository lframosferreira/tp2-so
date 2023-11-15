void scheduler(void) {
  struct proc *p;

  int ran = 0; // CS550: to solve the 100%-CPU-utilization-when-idling problem

  for (;;) {
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    ran = 0;

    if (sched_policy == 0) {
      for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state != RUNNABLE)
          continue;

        ran = 1;

        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, proc->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        proc = 0;
      }
    } else {
      int flag = 0;
      for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state != RUNNABLE)
          continue;
        if (p->priority == 1)
          continue;
        ran = 1;
        flag = 1;
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        p->running_tick++;
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, proc->context);
        switchkvm();
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        proc = 0;
        if (p->running_tick >= RUNNING_THRESHOLD && p->pid != 1 &&
            p->pid != 2 && p->procpriority != 1) {
          p->priority = 1;
        }
        struct proc *waitp;
        for (waitp = ptable.proc; waitp < &ptable.proc[NPROC]; waitp++) {
          if (waitp->priority == 1) {
            waitp->waiting_tick++;
          }
          if (waitp->waiting_tick >= WAITING_THRESHOLD) {
            waitp->waiting_tick = 0;
            waitp->priority = 0;
            waitp->running_tick = 0;
          }
        }
      }
      if (flag == 0) {
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
          if (p->state != RUNNABLE)
            continue;
          if (p->priority == 0)
            break;
          int max = 0;
          struct proc *maxp;
          for (maxp = ptable.proc; maxp < &ptable.proc[NPROC]; maxp++) {
            if (maxp->state != RUNNABLE) {
              continue;
            }
            if (maxp->waiting_tick > max) {
              max = maxp->waiting_tick;
              p = maxp;
            }
          }
          struct proc *waitp;
          for (waitp = ptable.proc; waitp < &ptable.proc[NPROC]; waitp++) {
            if (waitp->pid != p->pid) {
              waitp->waiting_tick++;
            }
            if (waitp->waiting_tick >= WAITING_THRESHOLD ||
                waitp->procpriority == 1) {
              waitp->waiting_tick = 0;
              waitp->priority = 0;
              waitp->running_tick = 0;
            }
          }
          ran = 1;
          proc = p;
          switchuvm(p);
          p->state = RUNNING;
          swtch(&cpu->scheduler, proc->context);
          switchkvm();

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          proc = 0;
        }
      }
    }
    release(&ptable.lock);

    if (ran == 0) {
      halt();
    }
  }
}