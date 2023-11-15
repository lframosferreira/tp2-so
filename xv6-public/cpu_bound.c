#include "types.h"
#include "user.h"
#include "stat.h"

#define LOOPS 100
#define ITERATIONS 1000000

int main(){
  printf(1, "oi do cpu bound\n");

  for (int i = 0; i < LOOPS; i++){
    for (int j = 0; j < ITERATIONS; j++){
      continue;
    }
  }

  exit();
}