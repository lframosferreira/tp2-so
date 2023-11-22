#include "types.h"
#include "stat.h"
#include "user.h"

#define LOOPS 20
#define ITERATIONS 1000000

int main(){

  for (int i = 0; i < LOOPS; i++){
    for (int j = 0; j < ITERATIONS; j++){
      if (j == 100){
        // yield();
        sleep(1);
      }
    }
  }

  exit();
}