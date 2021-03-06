#include "mtx.h"
/***************** queue.c file *****************/
int enqueue(PROC **queue, PROC *p) {
  PROC *q = *queue;
  if (q == 0 || p->priority > q->priority) {
    *queue = p;
    p->next = q;
  } else {
    while (q->next && p->priority <= q->next->priority)
      q = q->next;
    p->next = q->next;
    q->next = p;
  }
  return 0;
}
PROC *dequeue(PROC **queue) {
  PROC *p = *queue;
  if (p)
    *queue = (*queue)->next;
  return p;
}
int printList(char *name, PROC *p) {
  printf("%s = ", name);
  while (p) {
    printf("[%d %d]->", p->pid, p->priority);
    p = p->next;
  }
  printf("NULL\n");
  return 0;
}

int printSleep(char *name, PROC *p) {
  printf("%s = ", name);
  while (p) {
    printf("[%d event=%d]->", p->pid, p->event);
    p = p->next;
  }
  printf("NULL\n");
  return 0;
}

int printTree(char *name, PROC *p) {
  PROC *children[8] = {0};
  int i = 0;
  printf("%s = ", name);
  while (p) {
    printf("->[%d %d]", p->pid, p->priority);
    if (p->child)
      children[i++] = p->child;
    p = p->sibling;
  }
  printf("\n");
  for (i = 0; children[i]; i++)
    printTree(name, children[i]);
  return 0;
}