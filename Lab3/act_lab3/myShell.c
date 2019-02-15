#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define BASIC 0
#define REDIR 1
#define PIPE 2
#define PIPENREDIR 3
///////////////////Globals
char *dirs[64];
int ndirs;
char *names[64];
int nnames;
char *myargv[64];
int narg;
char *home;
///////////////////Functions
void initialize(void)
{
  for(int i=0;i<64;i++)
    {
      names[i]= (char *)malloc(sizeof(""));
    }
  for (int i=0; i <64; i++) {
    dirs[i]=(char *)malloc(sizeof(""));
  }
  for (int i=0; i < 64; i++) {
    myargv[i]=(char *)malloc(sizeof(""));
  }
}
void initializeArr(char *arr[])
{
  for(int i=0;i<64;i++)
    {
      arr[i]= (char *)malloc(sizeof(""));
    }
}
void setPathHome(void)
{
  char *token;
  char temp[320];
  int j=0;
    
  home = getenv("HOME");
  strcpy(temp,getenv("PATH"));
  
  token =strtok(temp,":");
  
  while(token != 0)
    {
      strcpy(dirs[j],token);
      token=strtok(0,":");
      j++;
    }
  ndirs = j;
}
int detCmdType(void)
{
  int r=0,p=0;
  for(int i=0;i<nnames;i++)
    {
      if(strcmp(names[i],">")==0 || strcmp(names[i],"<")==0 || strcmp(names[i],">>")==0)
	      r = REDIR;
      if(strcmp(names[i],"|")==0)
	      p= PIPE;
    }
  if(p==PIPE)
    return PIPE;
  if(r==REDIR)
    return REDIR;
  return BASIC;
     
}
int decodeNames(char *line)
{
  char *token;
  char temp[128];
  int j = 0;
  int cmdType=0;
  
  strcpy(temp,line);
  
  token = strtok(temp," ");
    
   for(int i=0;i<nnames;i++)
   {
     names[i][0]='\0';
   }
  
  while (token!=0) {
    strcpy(names[j],token);
    
    token = strtok(0," ");
    j++;
  }
  
  token=names[j-1];
  strcpy(names[j-1],strtok(token,"\n"));
  
  nnames = j;
  return detCmdType();
}

void setMyargv(int Mode,char *MYARGV[],char *NAMES[],int *NARG,int *NNAMES)
{
  int index=0;
  for(int i=0; i< *NARG ;i++)
  {
      MYARGV[i][0]='\0';
  }

  if(Mode == BASIC)
  {  
   for (int i =0; i < *NNAMES; i++) {
      strcpy(MYARGV[i],NAMES[i]); 
    }
    MYARGV[*NNAMES]=0; //Set end to null;
    *NARG = *NAMES;
  }
  else if(Mode== REDIR)
  { 
    for(index=0;strcmp(NAMES[index],">")!=0 && strcmp(NAMES[index],"<")!=0 && strcmp(names[index],">>")!=0; index++);//Find REdir
    for(int i=0;i<index;i++)
    {
      strcpy(MYARGV[i],NAMES[i]);
    }
    MYARGV[index]=0;
    *NARG = index;

  }
  else if(Mode == PIPE)
  {


  }
}
void BasicEx(char *env[],char *Myargv[],char *Names[],int *Narg,int *Nnames)
{
  char temp[128];
  
  setMyargv(BASIC,Myargv,Names,Narg,Nnames);
  int r;
  for (int i = 0; i < ndirs; i++) {
	  strcpy(temp,dirs[i]);
	  strcat(temp,"/");
	  strcat(temp,Names[0]);
	  r = execve(temp,Myargv,env);
  }
}
void BasicExP(char *env[],char *Myargv[],char *Cmd)
{
  char temp[128];
  
  int r;
  for (int i = 0; i < ndirs; i++) {
	  strcpy(temp,dirs[i]);
	  strcat(temp,"/");
	  strcat(temp,Cmd);
	  r = execve(temp,Myargv,env);
  }
}
void RedEx(char *env[],char *MYARGV[],char *NAMES[],int *NARG,int *NNAMES)
{
  char temp[128];
  setMyargv(REDIR,MYARGV,NAMES,NARG,NNAMES);

  if(strcmp(NAMES[*NARG],">")==0)
  {
    close(1);
    open(NAMES[*NARG+1], O_WRONLY|O_CREAT,0644);
    int r;
    for (int i = 0; i < ndirs; i++) {
	    strcpy(temp,dirs[i]);
	    strcat(temp,"/");
	    strcat(temp,NAMES[0]);
	    r = execve(temp,MYARGV,env);
    }
  }
  else if(strcmp(NAMES[*NARG],"<")==0)
  {
    close(0);
    open(NAMES[*NARG+1], O_RDONLY);
    int r;
    for (int i = 0; i < ndirs; i++) {
	    strcpy(temp,dirs[i]);
	    strcat(temp,"/");
	    strcat(temp,NAMES[0]);
	    r = execve(temp,MYARGV,env);
    }
  }
  else if(strcmp(NAMES[*NARG],">>")==0)
  {
    close(1);
    //printf("Opening followinf file: %s\n",NAMES[*NARG+1]);
    open(NAMES[*NARG+1], O_WRONLY | O_APPEND);
    int r;
    for (int i = 0; i < ndirs; i++) {
	    strcpy(temp,dirs[i]);
	    strcat(temp,"/");
	    strcat(temp,NAMES[0]);
	    r = execve(temp,MYARGV,env);
    }
  }
}
int checkReArr(char *Arr[],int size)
{
  for(int i=0;i<size;i++)
  {
    if(strcmp(Arr[i],">")==0 || strcmp(Arr[i],"<")==0 || strcmp(Arr[i],">>")==0)
      return 1;
  }
  return 0;
}
int setPipeArgs(char *headarg[],char *headnames[],int *nhnames,char *tailarg[],char *tailnames[],int *ntnames,int *nhead,int *ntail,char *Names[],int nnames)
{
  int pipe=0;
  int i=0;
  int j=0;
  for(pipe=0;strcmp(Names[pipe],"|")!=0;pipe++);
  for(i=0;i<pipe;i++)
  {
    strcpy(headarg[i],Names[i]);
    strcpy(headnames[i],Names[i]);
  }
  headarg[i]=0;
  headnames[i]=0;
  *nhead=i;
  *nhnames=i;
  for(j=pipe+1;j<nnames;j++)
  {
    
    strcpy(tailarg[j-(pipe+1)],Names[j]);
    strcpy(tailnames[j-(pipe+1)],Names[j]);
  }
  tailarg[j-(pipe+1)]=0;
  tailnames[j-(pipe+1)]=0;
  *ntail= j-(pipe+1);
  *ntnames= j-(pipe+1);
  if(checkReArr(headarg,*nhead)==1 && checkReArr(tailarg,*ntail)==1)
  return 3;//Both redir
  if(checkReArr(headarg,*nhead)==0 && checkReArr(tailarg,*ntail)==1)
  return 2;//Second Redir
  if(checkReArr(headarg,*nhead)==1 && checkReArr(tailarg,*ntail)==0)
  return 1;//First Redir
  if(checkReArr(headarg,*nhead)==0 && checkReArr(tailarg,*ntail)==0)
  return 0;//None Redir

}
void PipeEx(char *env[])
{
  char temp[128];
  char *headarg[64];
  char *headnames[64];
  int nhnames;
  int nhead;
  char *tailarg[64];
  char *tailnames[64];
  int ntnames;
  int ntail;
  int redir=0;
  initializeArr(headarg);
  initializeArr(headnames);
  initializeArr(tailarg);
  initializeArr(tailnames);
  redir = setPipeArgs(headarg,headnames,&nhnames,tailarg,tailnames,&ntnames,&nhead,&ntail,names,nnames);
  int r;
  int pd[2],pid;
  pipe(pd);
  pid = fork();
  if(pid)//writter
  {
    close(pd[0]);
    close(1);
    dup(pd[1]);
    close(pd[1]);
    if(redir ==0 || redir ==2)
    BasicExP(env,headarg,headarg[0]);
    if(redir==1||redir==3)
    RedEx(env,headarg,headnames,&nhead,&nhnames);
  }
  else{
    close(pd[1]);
    close(0);
    dup(pd[0]);
    close(pd[0]);
    if(redir ==0 || redir ==1)
    BasicExP(env,tailarg,tailarg[0]);
    if(redir==2||redir==3)
    RedEx(env,tailarg,tailnames,&ntail,&ntnames);
    
  }
}
void exCmd(char *env[],int CmdType)
{
  int pid=0,status=0;
  char temp[128];
  pid = fork();
  if (pid) {
    pid = wait(&status);
   }
  else {
    switch(CmdType)
    {
      case BASIC:
      BasicEx(env,myargv,names,&narg,&nnames);
      break;
      case REDIR:
      RedEx(env,myargv,names,&narg,&nnames);
      break;
      case PIPE:
      PipeEx(env);
      break;
    }  
  }
}




///////////////////Driver
int main(int argc, char *argv[], char *env[])
{
  char line[128];
  char temp[128];
  int pid=0;
  int status=0;
  int i=0;
  int CmdType=0;
  initialize();
  setPathHome();
 
  //////////////////////
  while (1) {
     printf("~$: ");
     fgets(line,128,stdin);
     CmdType = decodeNames(line);
     
     if(strcmp(names[0],"cd")==0 || strcmp(names[0],"exit") ==0)
       {
	       if(strcmp(names[0],"cd") == 0 && strlen(names[1])> 0)
	          chdir(names[1]);
	       if(strcmp(names[0],"cd") == 0 && strlen(names[1]) == 0)
	          chdir(home);
         if(strcmp(names[0],"exit") ==0)
	          exit(1);
       }
     else
       {
         exCmd(env,CmdType);
       }
  }  
  return 0;
}
