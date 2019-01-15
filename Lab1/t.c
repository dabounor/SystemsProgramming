/************* t.c file ********************/
#include <stdio.h>
#include <stdlib.h>

int *FP;

int main(int argc, char *argv[ ], char *env[ ])
{
  int a,b,c;
  printf("enter main\n");
  
  printf("&argc=%x argv=%x env=%x\n", &argc, argv, env);
  printf("&a=%8x &b=%8x &c=%8x\n", &a, &b, &c);

//(1). Write C code to print values of argc and argv[] entries - DONE

int i = 0;
for (i=0; i < argc; i++)
{
  printf("Argc: %d, Argv[%d]: %s Address: %x\n", i, i, argv[i], &argv[i]);
}

  a=1; b=2; c=3;
  A(a,b);
  printf("exit main\n");
}

int A(int x, int y)
{
  int d,e,f;
  printf("enter A\n");
  // PRINT ADDRESS OF d, e, f DONE
  d=4; e=5; f=6;
  printf("Address &d: %x\n", &d);
  printf("Address &e: %x\n", &e);
  printf("Address &f: %x\n", &f);

  B(d,e);
  printf("exit A\n");
}

int B(int x, int y)
{
  int g,h,i;
  printf("enter B\n");
  // PRINT ADDRESS OF g,h,i
  printf("Address &g: %x\n", &g);
  printf("Address &h: %x\n", &h);
  printf("Address &i: %x\n", &i);
  g=7; h=8; i=9;
  C(g,h);
  printf("exit B\n");
}

int C(int x, int y)
{
  int u, v, w, i, *p;

  printf("enter C\n");
  // PRINT ADDRESS OF u,v,w,i,p;
  printf("Address &u: %x\n", &u);
  printf("Address &v: %x\n", &v);
  printf("Address &w: %x\n", &w);
  printf("Address &i: %x\n", &i);
  printf("Address &p: %x\n", &p);

  u=10; v=11; w=12; i=13;

  FP = (int *)getebp();

//(2). Write C code to print the stack frame link list. DONE

int index = 0;
printf("\n");
printf("Stack Frame Link List: \n");
while ((FP != NULL) && (FP != 0))
{
  printf("%x\n", FP);
  FP = *FP;
}

printf("\n");
 p = (int *)&p;

//(3). Print the stack contents from p to the frame of main()
    // YOU MAY JUST PRINT 128 entries of the stack contents.

index = 0;
for(index=0; index <= 128; index++)
{
  printf("P: %x\n", p);
  p++;
}

//(4). On a hard copy of the print out, identify the stack contents
  //   as LOCAL VARIABLES, PARAMETERS, stack frame pointer of each function.
}
