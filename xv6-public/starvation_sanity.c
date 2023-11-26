#include "types.h"
#include "stat.h"
#include "user.h"

#define CPU_BOUND 0
#define S_CPU 1
#define IO_BOUND 2

int main(int argc, char **argv) {

  int n = atoi(argv[1]);

  for (int i = 0; i < 3 * n; i++) {
    if (fork() == 0) {
      int child_pid_mod3 = getpid() % 3;
      switch (child_pid_mod3) {
      case 0:
        exec("cpu_bound", argv);
      case 1:
        exec("s_cpu", argv);
      case 2:
        exec("io_bound", argv);
      default:
        printf(1, "Não deveria cair aqui\n");
      }
    }
  }

  int wpid;
  int prio;

  while ((wpid = wait3(&prio)) > 0) {
    int wpid_mod3 = wpid % 3;
    char *p_type;
    switch (wpid_mod3) {
    case CPU_BOUND:
      p_type = "CPU-Bound";
      break;
    case S_CPU:
      p_type = "S-CPU";
      break;
    case IO_BOUND:
      p_type = "IO-Bound";
      break;
    default:
      printf(1, "Não deveria cair aqui: sanity_starvation\n");
      break;
    }
    printf(1, "PID: %d Tipo: %s Prioridade final: %d\n", wpid, p_type, prio);
  };

  exit();
}
