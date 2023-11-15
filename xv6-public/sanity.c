#include "types.h"
#include "user.h"
#include "stat.h"

int main(int argc, char **argv){

  int n = atoi(argv[1]);

  for (int i = 0; i < 3 * n; i++){
    if (fork() == 0){
      int child_pid_mod3 = getpid() % 3;
      switch(child_pid_mod3){
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
  
  
  while ((wpid = wait2(&retime, &rutime, &stime)) > 0){
    int wpid_mod3 = wpid % 3;
    // adicionar switch aqui. Além de ficar mais legivel vai faciliar pegar as médias
    char *p_type = wpid_mod3 == 0 ? "CPU-Bound" : wpid_mod3 == 1 ? "S-CPU": "IO-Bound";
    printf(1, "%d %s\n", wpid, p_type);
    printf(1, "%d %d %d\n", retime, rutime, stime);
  };

  exit();
}