/*
Zeid Al-Ameedi
Lab4 parallel computing
Implemented Wallclock using multiple threads
*/

#include "type.h"

int main(int argc, char *argv[]) {
  initMatrix();
  matrixSumS();
  matrixSumT();
  wallClock();
  return 0;
}
