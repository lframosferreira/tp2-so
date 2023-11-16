#include "types.h"
#include "stat.h"
#include "user.h"

#define NUMBER_OF_SLEEP_CALLS 100
#define SLEEP_TIME 1

int main() {
  for (int i = 0; i < NUMBER_OF_SLEEP_CALLS; i++) {
    sleep(SLEEP_TIME);
  }

  exit();
}