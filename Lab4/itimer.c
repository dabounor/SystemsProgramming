#include "type.h"

void *func(void *arg) 
{
  int j, row, temp, mysum = 0;
  pthread_t tid = pthread_self(); 
  row = (int)arg;                
  printf("thread %d computes sum of row %d : ", row, row);

  for (j = 0; j < N; j++) // compute sum of A[row]in global sum[row]
    mysum += A[row][j];
  pthread_mutex_lock(&m);
  temp = total;  // get total
  temp += mysum; // add mysum to temp

  sleep(1); 

  total = temp; 
  pthread_mutex_unlock(&m);

  pthread_exit((void *)0); // thread exit: 0=normal termination
}

int print() {
  int i, j;
  for (i = 0; i < M; i++) {
    for (j = 0; j < N; j++) {
      printf("%4d ", A[i][j]);
    }
    printf("\n");
  }
  return 0;
}

int initMatrix() {
  printf("initialize A matrix\n");
  for (int i = 0; i < M; i++)
    for (int j = 0; j < N; j++)
      A[i][j] = i + j + 1;
  return 0;
}

int matrixSumT() {
  total = 0;
  pthread_mutex_init(&m, NULL);
  struct timeval tStart, tStop;
  pthread_t thread[M]; // thread IDs
  int status;

  printf("\nThreaded:\n");
  gettimeofday(&tStart, NULL);
  printf("Start timer at %ld s %ld us\n", tStart.tv_sec, tStart.tv_usec);

  printf("create %d threads\n", M);
  for (int i = 0; i < M; i++)
    pthread_create(&thread[i], NULL, func, (void *)(long)i);

  printf("try to join with threads\n");
  for (int i = 0; i < M; i++) {
    pthread_join(thread[i], (void *)&status);
    printf("joined with thread %d : status=%d\n", i, status);
  }

  printf("compute and print total : ");
  for (int i = 0; i < M; i++)
    total += sum[i];
  printf("total = %d\n", total);

  gettimeofday(&tStop, NULL);
  printf("Stop timer at %ld s %ld us\n", tStop.tv_sec, tStop.tv_usec);
  printf("Time Elapsed %ld s %ld us\n", tStop.tv_sec - tStart.tv_sec,
         tStop.tv_usec - tStart.tv_usec);

  return total;
}

int matrixSumS() {
  total = 0;
  struct timeval tStart, tStop;
  printf("\nSequential:\n");
  gettimeofday(&tStart, NULL);
  printf("Start timer at %ld s %ld us\n", tStart.tv_sec, tStart.tv_usec);

  printf("compute and print total : ");
  for (int i = 0; i < M; i++)
    for (int j = 0; j < N; j++)
      total += A[i][j];
  printf("total = %d\n", total);

  gettimeofday(&tStop, NULL);
  printf("Stop timer at %ld s %ld us\n", tStop.tv_sec, tStop.tv_usec);
  printf("Time Elapsed %ld s %ld us\n", tStop.tv_sec - tStart.tv_sec,
         tStop.tv_usec - tStart.tv_usec);

  return total;
}