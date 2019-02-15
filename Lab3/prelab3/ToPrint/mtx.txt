/*********** t.c file of A Multitasking System *********/
#include "mtx.h"

/*******************************************************
  kfork() creates a child process; returns child pid.
  When scheduled to run, child PROC resumes to body();
********************************************************/

// initialize the MT system; create P0 as initial running process
int init() {
  int i;
  PROC *p;
  for (i = 0; i < NPROC; i++) { // initialize PROCs
    p = &proc[i];
    p->pid = i; // PID = 0 to NPROC-1
    p->status = FREE;
    p->priority = 0;
    p->next = p + 1;
    p->child = NULL;
    p->sibling = NULL;
  }
  proc[NPROC - 1].next = 0;
  freeList = &proc[0]; // all PROCs in freeList
  readyQueue = 0;      // readyQueue = empty

  sleepList = 0; // sleepList = empty

  // create P0 as the initial running process
  p = running = (void *)dequeue(&freeList); // use proc[0]
  p->status = READY;
  p->priority = 0;
  p->ppid = 0; // P0 is its own parent

  printList("freeList", freeList);
  printf("init complete: P0 running\n");
  return 0;
}

int menu() {
  printf("********************************************\n");
  printf(" ps fork switch exit jesus sleep wakeup wait\n");
  printf("********************************************\n");
  return 0;
}

char *status[] = {"FREE", "READY", "SLEEP", "ZOMBIE"};

int do_ps() {
  int i;
  PROC *p;
  printf("PID  PPID  status\n");
  printf("---  ----  ------\n");
  for (i = 0; i < NPROC; i++) {
    p = &proc[i];
    printf(" %d    %d    ", p->pid, p->ppid);
    if (p == running)
      printf("RUNNING\n");
    else
      printf("%s\n", status[p->status]);
  }
  return 0;
}

int do_jesus() {
  int i;
  PROC *p;
  printf("Jesus perfroms miracles here\n");
  for (i = 1; i < NPROC; i++) {
    p = &proc[i];
    if (p->status == ZOMBIE) {
      p->status = READY;
      enqueue(&readyQueue, p);
      printf("raised a ZOMBIE %d to live again\n", p->pid);
    }
  }
  printList("readyQueue", readyQueue);
  return 0;
}

int bodyCall() { return body(MY_NAME); }
int body(char *myname) // process body function
{
  int c;
  char cmd[64];
  printf("proc %d starts from body():", running->pid);
  printf("myname = %s\n", myname);
  while (1) {
    printf("***************************************\n");
    printf("proc %d running: parent=%d\n", running->pid, running->ppid);
    printList("readyQueue", readyQueue);
    printSleep("sleepList ", sleepList);
    printTree("procTree", running);
    menu();
    printf("enter a command : ");
    fgets(cmd, 64, stdin);
    cmd[strlen(cmd) - 1] = 0;

    if (strcmp(cmd, "ps") == 0)
      do_ps();
    else if (strcmp(cmd, "fork") == 0)
      do_kfork();
    else if (strcmp(cmd, "switch") == 0)
      do_switch();
    else if (strcmp(cmd, "exit") == 0)
      do_exit();
    else if (strcmp(cmd, "jesus") == 0)
      do_jesus();
    else if (strcmp(cmd, "sleep") == 0)
      do_sleep();
    else if (strcmp(cmd, "wakeup") == 0)
      do_wakeup();
    else if (strcmp(cmd, "wait") == 0)
      do_wait();
    else
      break;
  }
  return do_exit();
}

// 1). char *myname = "YOUR_NAME";  // your name

//      Rewrite the body() function as
//              int body(char *myname){ printf("%s", myname);  .....  }
//      Modify kfork() to do the following:

//         When a new proc starts to execute body(), print the myname string,
//              which should be YOUR name.

//         When body() ends, e.g. by a null command string, return to do_exit();

//   HINT: AS IF the proc has CALLED body(), passing myname as parameter.

// (2). Modify kfork() to implement process family tree as a binary tree
//        In the body() function, print the children list of the running proc.
int kfork() {
  int i;
  PROC *cur;
  PROC *p = dequeue(&freeList);
  if (!p) {
    printf("no more procs available\n");
    return (-1);
  }
  /* initialize the new proc and its stack */
  p->status = READY;
  p->priority = 1; // ALL PROCs priority=1, except P0
  p->ppid = running->pid;
  p->child = NULL;
  p->sibling = NULL;
  if (!running->child)
    running->child = p;
  else {
    cur = running->child;
    while (cur->sibling)
      cur = cur->sibling;
    cur->sibling = p;
  }

  /************ new task initial stack contents ************
   kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
                      -1   -2  -3  -4  -5  -6  -7  -8   -9
  **********************************************************/
  for (i = 1; i < 10; i++) // zero out kstack cells
    p->kstack[SSIZE - i] = 0;
  p->kstack[SSIZE - 1] = (int)bodyCall; // retPC -> bodyCall()
  p->ksp = &(p->kstack[SSIZE - 9]);     // PROC.ksp -> saved eflag
  enqueue(&readyQueue, p);              // enter p into readyQueue
  return p->pid;
}

int do_kfork() {
  int child = kfork();
  if (child < 0)
    printf("kfork failed\n");
  else {
    printf("proc %d kforked a child = %d\n", running->pid, child);
    printList("readyQueue", readyQueue);
  }
  return child;
}

int do_switch() {
  tswitch();
  return 0;
}

int do_exit() {
  printf("P%d in do_exit, enter exit value :", running->pid);
  int exit;
  scanf("%d", &exit);
  getchar();
  printf("\n");
  kexit(exit);
  return 0;
}

int do_sleep() {
  int event;
  printf("enter an event value to sleep on : ");
  scanf("%d", &event);
  getchar();
  sleep(event);
  return 0;
}

int do_wakeup() {
  int event;
  printf("enter an event value to wakeup with : ");
  scanf("%d", &event);
  getchar();
  wakeup(event);
  return 0;
}

int do_wait() {
  wait(&running->status);
  return 0;
}

/*********** scheduler *************/
int scheduler() {
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
    enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);
  return 0;
}
