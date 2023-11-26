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
  int retime, rutime, stime;

  /* Tempo médio SLEEPING para cada tipo de processo. */
  int total_cpu_bound_stime = 0;
  int total_s_cpu_stime = 0;
  int total_io_bound_stime = 0;

  /* Tempo médio READY para cada tipo de processo. */
  int total_cpu_bound_retime = 0;
  int total_s_cpu_retime = 0;
  int total_io_bound_retime = 0;

  /* Tempo médio de TURNAROUND para cada tipo de processo. */
  int total_cpu_bound_turnaround = 0;
  int total_s_cpu_turnaround = 0;
  int total_io_bound_turnaround = 0;

  while ((wpid = wait2(&retime, &rutime, &stime)) > 0) {
    int wpid_mod3 = wpid % 3;
    char *p_type;
    switch (wpid_mod3) {
    case CPU_BOUND:
      p_type = "CPU-Bound";

      total_cpu_bound_stime += stime;
      total_cpu_bound_retime += retime;
      total_cpu_bound_turnaround += retime + rutime + stime;

      break;
    case S_CPU:
      p_type = "S-CPU";

      total_s_cpu_stime += stime;
      total_s_cpu_retime += retime;
      total_s_cpu_turnaround += retime + rutime + stime;

      break;
    case IO_BOUND:
      p_type = "IO-Bound";

      total_io_bound_stime += stime;
      total_io_bound_retime += retime;
      total_io_bound_turnaround += retime + rutime + stime;

      break;
    default:
      printf(1, "Não deveria cair aqui: sanity\n");
      break;
    }
    printf(1, "%d %s\n", wpid, p_type);
    printf(1, "%d %d %d\n", retime, rutime, stime);
  };

  printf(1, "Tempo medio no estado SLEEPING\n");
  printf(1, "CPU-Bound: %d\n", total_cpu_bound_stime / (3 * n));
  printf(1, "S-CPU: %d\n", total_s_cpu_stime / (3 * n));
  printf(1, "IO-Bound: %d\n", total_io_bound_stime / (3 * n));

  printf(1, "Tempo medio no estado READY\n");
  printf(1, "CPU-Bound: %d\n", total_cpu_bound_retime / (3 * n));
  printf(1, "S-CPU: %d\n", total_s_cpu_retime / (3 * n));
  printf(1, "IO-Bound: %d\n", total_io_bound_retime / (3 * n));

  printf(1, "Tempo medio de TURNAROUND\n");
  printf(1, "CPU-Bound: %d\n", total_cpu_bound_turnaround / (3 * n));
  printf(1, "S-CPU: %d\n", total_s_cpu_turnaround / (3 * n));
  printf(1, "IO-Bound: %d\n", total_io_bound_turnaround / (3 * n));


  exit();
}