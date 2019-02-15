#include "mtx.h"

int sleep(int event) {
  printf("proc %d going to sleep on event=%d\n", running->pid, event);

  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  printList("sleepList", sleepList);
  tswitch();
  return 0;
}

int wakeup(int event) {
  PROC *temp, *p;
  temp = 0;
  printList("sleepList", sleepList);
  p = dequeue(&sleepList);
  while (p) {
    if (p->event == event) {
      printf("wakeup %d\n", p->pid);
      p->status = READY;
      enqueue(&readyQueue, p);
    } else {
      enqueue(&temp, p);
    }
    p = dequeue(&sleepList);
  }
  sleepList = temp;
  printList("sleepList", sleepList);
  return 0;
}

int makeOrphan(PROC *orphan) {
  if (!orphan)
    return 0;
  PROC *orphanage = &proc[1];
  printf("giving away orphaned P%d to P1\n", orphan->pid);
  orphan->ppid = 1;

  if (!orphanage->child)
    orphanage->child = orphan;
  else {
    orphanage = orphanage->child;
    while (orphanage->sibling)
      orphanage = orphanage->sibling;
    orphanage->sibling = orphan;
  }
  PROC *sibling = orphan->sibling;
  orphan->sibling = NULL;
  PROC *child = orphan->sibling;
  orphan->child = NULL;
  makeOrphan(sibling);
  makeOrphan(child);
  return 0;
}

int kexit(int exitValue) {
  if (running->pid == 1)
    return printf("P1 cannot die\n");

  makeOrphan(running->child);
  running->child = NULL;
  running->exitCode = exitValue;
  running->status = ZOMBIE;
  tswitch();
  return 0;
}

int wait(int *status) {

  printf("proc %d waits for a ZOMBIE child\n", running->pid);
  if (!running->child) {
    printf("wait error: no child\n");
    return -1;
  }

  while (1) {
    PROC *zombie = running->child;
    while (!(zombie && zombie->status == ZOMBIE))
      zombie = zombie->sibling;

    if (zombie) {
      int zpid = zombie->pid;
      printf("P%d buried ZOMBIE child P%d with exitCode %d\n", running->pid,
             zombie->pid, zombie->exitCode);
      zombie->status = FREE;
      enqueue(&freeList, zombie);
      return zpid;
    }
    printf("P%d wait for ZOMBIE child\n", running->pid);
    sleep(running->pid);
  }
}