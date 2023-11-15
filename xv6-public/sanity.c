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

  
  while ((wpid = wait2(&retime, &rutime, &stime)) > 0){
    printf(1, "oiii");
    int wpid_mod3 = wpid % 3;
    char *p_type = wpid_mod3 == 0 ? "CPU-Bound" : wpid_mod3 == 1 ? "S-CPU": "IO-Bound";
    printf(1, "%d %s\n", wpid, p_type);
    printf(1, "%d %d %d\n", retime, rutime, stime);
  };

  exit();
}