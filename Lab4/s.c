/**** s.c file: compute matrix sum sequentially ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include<time.h>


#define  M   4
#define  N   500000
  

struct timeval t1, t2;
int res=0, res2=0;
res=gettimeofday(&t1, NULL);
res2=gettimeofday(&t2, NULL);

int A[M][N], sum[M];

int total;

void *func(void *arg)              // threads function
{
  int j, row, mysum;
   pthread_t tid = pthread_self(); // get thread ID number

   row = (int)arg;                 // get row number from arg
   printf("thread %d computes sum of row %d : ", row, row);
   mysum = 0;

   for (j=0; j < N; j++)     // compute sum of A[row]in global sum[row]
       mysum += A[row][j];

   sum[row] = mysum;

   printf("thread %d done: sum[%d] = %ld\n", row, row, sum[row]);
   pthread_exit((void *)row); // thread exit: 0=normal termination
}

// print the matrix (if N is small, do NOT print for large N)
int print()
{
   int i, j;
   for (i=0; i < M; i++){
     for (j=0; j < N; j++){
       printf("%4d ", A[i][j]);
     }
     printf("\n");
   }
}

int main (int argc, char *argv[])
{
   pthread_t thread[M];      // thread IDs
   int i, j, status;

   printf("main: initialize A matrix\n");

   for (i=0; i < M; i++){
     for (j=0; j < N; j++){
       A[i][j] = i + j + 1;
     }
   }

   print();

   printf("main: create %d threads\n", M);
   for(i=0; i < M; i++) {
      pthread_create(&thread[i], NULL, func, (void *)i); 
   }

   printf("main: try to join with threads\n");
   for(i=0; i < M; i++) {
     pthread_join(thread[i], (void *)&status);
     printf("main: joined with thread %d : status=%d\n", i, status);
   }

   printf("main: compute and print total : ");
   for (i=0; i < M; i++)
       total += sum[i];
   printf("tatal = %ld\n", total);

   pthread_exit(NULL);
}
