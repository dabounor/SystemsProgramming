// Programmer: Zeid Al-Ameedi
// Date: 02-14-2018
// Collab: KCWang, TA, Stackoverflow
// Mimics the process that a OS can implement

#include "mtx.h"

/*
Main function that calls our initialze and kfork function. Begins the process tree
switches priority to queue as well
*/
int main() 
{
  init();  
  kfork(); 
  while (1) 
  {
    printf("P0: switch process\n");
    while (!readyQueue);
    tswitch();
  }
}