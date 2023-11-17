#include "types.h"
#include "user.h"
#include "stat.h"

#define NUMBER_OF_PROCESSES 20

int main(int argc, char **argv)
{  
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++){
        if (fork() == 0){
            change_prio((getpid() % 3) + 1);
            exec("cpu_bound_test", argv);
        }
    }
    
    int wpid;
    while ((wpid = wait()) > 0){
        printf(1, "pid: %d\n", wpid);
    }
    exit();
}