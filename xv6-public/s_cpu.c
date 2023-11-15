#include "types.h"
#include "stat.h"
#include "user.h"

int main(){

  printf(1, "oi do s cpu cpu_bound\n");

  for (int i = 0; i < 20; i++){
    for (int j = 0; j < 1000000; j++){
      if (j == 100){
        /* yield(); */
      }
    }
  }


  exit();
}