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
    printf(1, "%i = %d\n", i);
  }
  

  while (wait() > 0);

  return 0;
}