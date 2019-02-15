#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NPROC 9    // number of PROCs
#define SSIZE 1024 // stack size = 4KB

// PROC status
#define FREE 0
#define READY 1
#define SLEEP 2
#define ZOMBIE 3

#define MY_NAME "Zeid Al-Ameedi"

// PROC
typedef struct proc {
  struct proc *next; // next proc pointer
  int *ksp;          // saved sp: at byte offset 4

  int pid;      // process ID
  int ppid;     // parent process pid
  int status;   // PROC status=FREE|READY, etc.
  int priority; // scheduling priority

  int event;    // event value to sleep on
  int exitCode; // exit value

  struct proc *child;   // first child PROC pointer
  struct proc *sibling; // sibling PROC pointer
  struct proc *parent;  // parent PROC pointer

  int kstack[1024]; // process stack
} PROC;

// Globals
PROC proc[NPROC]; // NPROC PROCs
PROC *freeList;   // freeList of PROCs
PROC *readyQueue; // priority queue of READY procs
PROC *running;    // current running proc pointer

PROC *sleepList; // list of SLEEP procs

// Prototype
int init();
int kfork();
int body();
int tswitch();
int do_sleep();
int do_wakeup();
int do_exit();
int do_switch();
int do_kfork();
int do_wait();
int sleep(int event);
int wakeup(int event);
int kexit(int exitValue);

int wait(int *status);
int enqueue(PROC **queue, PROC *p);
PROC *dequeue(PROC **queue);
int printList(char *name, PROC *p);
int printSleep(char *name, PROC *p);
int printTree(char *name, PROC *p);