#pragma once
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>


#define M 4
#define N 500000


int A[M][N], sum[M];
int total;
pthread_mutex_t m;
void *func(void *arg);
int print();
int initMatrix();
int matrixSumT();
int matrixSumS();
int hh, mm, ss;
void timer_handler(int sig);
int wallClock();