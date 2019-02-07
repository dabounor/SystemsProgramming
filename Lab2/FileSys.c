/*
    Programmer: Zeid Al-Ameedi
    Date: 01-22-2019 to 01-31-2019
    Details: Simulation that mimics the Unix File system

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

typedef struct node{
    char name[64];
    char type; // node type: 'D' for directory or 'F' for file
    struct node *child, *sibling, *parent;
}NODE;

NODE *root, *cwd, *start;
char line[128];                   // for getting user input line
char command[64], pathname[64];   // for command and pathname strings
char dname[100], bname[64];        // for dirname and basename 
char *name[100];                  // token string pointers 
int  n;                           // number of token strings in pathname 
FILE *fp;


int dbname(char *pathname);
int mkdir(char *pathname);
int rmdir(char *pathname);
int ls(char *pathname);
int cd(char *pathname);
int pwd(char *pathname);
char * pwdHelp(NODE*pParent);
int create(char *pathname);
int rm(char *pathname);
int reload(char *filename);
int save(char *filename);
void saveHelp(NODE*pFam, FILE *saveFile);
int menu(char *filename);
int quit(char *filename);
int tokenize(char *pathname);
NODE *search_child(NODE *parent, char *name);
NODE *path2node(char *pathname);
int findCmd(char *command);
int addChild(NODE * wd, NODE * child);
int removeChild(NODE * wd, NODE * child);
NODE * makeNODE(char *name, char type);


char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "create", "rm", "reload", "save", "menu", "quit", NULL};
int (*fptr[ ])(char *)={(int (*)())mkdir,rmdir,ls,cd,pwd,create,rm,reload,save,menu,quit};

int dbname(char *pathname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}

int mkdir(char *pathname)
{
    dbname(pathname);
    NODE *temp = path2node(dname);
    if (temp != 0)
    {
        if(temp->type == 'D')
        {
            if(!search_child(temp,bname)){
                addChild(temp,makeNODE(bname,'D'));
            }
            else
            {
                printf("‘%s’: Directory exists\n",pathname);
            }
        }
        else
        {
            printf("‘%s’: Not a directory\n",pathname);
        }
    }
    else
    {
        printf("‘%s’: No such file or directory\n",pathname);
    }
    strcpy(dname,"");
    strcpy(bname,"");
}

int rmdir(char *pathname)
{
    dbname(pathname);
    NODE *temp = path2node(pathname);
    if (temp != 0)
    {
        if(temp->type == 'D')
        {
            if(!(temp->child)){
                removeChild(path2node(dname),temp);
            }
            else
            {
                printf("Error: rmdir: cannot remove directory ‘%s’: Directory is not empty\n",pathname);
            }
        }
        else
        {
            printf("Error: rmdir: cannot remove directory ‘%s’: Not a directory\n",pathname);
        }
    }
    else
    {
        printf("Error: rmdir: cannot remove directory ‘%s’: No such file or directory\n",pathname);
    }
    strcpy(dname,"");
    strcpy(bname,"");
}

int ls(char *pathname)
{
    NODE * temp = path2node(pathname);
    if (temp){
        temp = temp->child;
    }
    else{
        printf("‘%s’: No such file or directory\n",pathname);
    }
    while(temp){
        printf("\n%c %s ",temp->type,temp->name);
        temp = temp->sibling;
    }
    putc('\n',stdout);
}

int cd(char *pathname)
{
    NODE * temp = path2node(pathname);
    if(temp && temp->type == 'D')
    {
        cwd = temp;
    }
    else
    {
        printf("‘%s’: No such file or directory\n",pathname);
    }
}

int pwd(char *pathname)
{
    printf("%s\n",pwdHelp(cwd));
}

char * pwdHelp(NODE*pParent)
{
    if(pParent != root){
        char temp[100] = "";
        strcat(temp,pwdHelp(pParent->parent));
        if(strlen(temp)>1){
            strcat(temp,"/");
        }
        strcat(temp,pParent->name);
        char * path = temp;
        return path;
    }
    else{
        return pParent->name;
    }
}

int create(char *pathname)
{
    dbname(pathname);
    NODE *temp = path2node(dname);
    if (temp != 0)
    {
        if(temp->type == 'D')
        {
            if(!search_child(temp,bname)){
                addChild(temp,makeNODE(bname,'F'));
            }
            else
            {
                printf("‘%s’: File exists\n",pathname);
            }
        }
        else
        {
            printf("‘%s’: Not a directory\n",pathname);
        }
    }
    else
    {
        printf("‘%s’: No such file or directory\n",pathname);
    }
    strcpy(dname,"");
    strcpy(bname,"");
}

int rm(char *pathname)
{
    dbname(pathname);
    NODE *temp = path2node(pathname);
    if (temp != 0)
    {
        if(temp->type == 'F')
        {
            removeChild(path2node(dname),temp);
        }
        else
        {
            printf("‘%s’: Not a file\n",pathname);
        }
    }
    else
    {
        printf("‘%s’: No such file or directory\n",pathname);
    }
    strcpy(dname,"");
    strcpy(bname,"");
}

int reload(char *filename)
{
    FILE *fp = fopen(filename, "r+");
    if(fp != NULL){
        char newType, newPath[100];
        fscanf(fp," %c %s",&newType, newPath);
        if(!strcmp(newPath,"/"));
        {
            while(!feof(fp))
            {
                fscanf(fp," %c %s",&newType, newPath);
                if(newType == 'D')
                {
                    mkdir(newPath);
                }
                else if(newType == 'F')
                {
                    create(newPath);
                }
                newType = 0;
            }
        }
    }
}

int save(char *filename)
{
    FILE *fp = fopen(filename, "w+"); // fopen a FILE stream for WRITE
    if (fp != NULL)
    {
        saveHelp(root,fp);
        fclose(fp); // close FILE stream when done
    }
}
void saveHelp(NODE*pFam, FILE *saveFile)
{
    if(pFam)
    {
        fprintf(saveFile, "%c %s\n", pFam->type, pwdHelp(pFam)); // print a line to file
        saveHelp(pFam->child,saveFile);
        saveHelp(pFam->sibling,saveFile);
    }
}

int menu(char *filename)
{
    printf("    - mkdir [pathname] : make a new directory for a given pathname\n");
    printf("    - rmdir [pathname] : remove the directory, if it is empty.\n");
    printf("    - cd [pathname] : change CWD to pathname, or to / if no pathname.\n");
    printf("    - ls [pathname] : list the directory contents of pathname or CWD\n");
    printf("    - pwd : print the (absolute) pathname of CWD\n");
    printf("    - create [pathname] : create a FILE node.\n");
    printf("    - rm [pathname] : remove the FILE node.\n");
    printf("    - save [filename[] : save the current file system tree as a file\n");
    printf("    - reload [filename] : construct a file system tree from a file\n");
    printf("    - menu : show a menu of valid commands\n");
    printf("    - quit : save the file system tree, then terminate the program.\n");
}

int quit(char *filename)
{
    save(filename);
    exit(0);
}



/*decompose pathanme into token strings pointed by name[0], name[1], ..., name[n-1]*/
int tokenize(char *pathname)
{
    int n = 0;
    name[n] = strtok(pathname,"/0");
    while (name[n] != NULL)
    {
        n++;
        name[n] = strtok(NULL,"/0");
    }
    return n;
}

 /**********************************************************************/

 /*Search for name under a parent node. Return the child node pointer if exists, 0 if not.*/
NODE *search_child(NODE *parent, char *name)
{
    if (!strcmp(name,".."))
    {
        return parent->parent;
    }
    NODE * pGen = parent->child;
    while (pGen != NULL)
    {
        if(!strcmp(pGen->name,name))
        {
            return pGen;
        }
        pGen = pGen->sibling;
    }
    return 0;
}

/*return the node pointer of a pathname, or 0 if the node does not exist. */
NODE *path2node(char *pathname)
{
    if (pathname[0] == '/')
    {
        start = root;
    }
    else
    {
        start = cwd;
    }             
    if (!strcmp(pathname,".")){
        strcpy(pathname,"");
    }

    n = tokenize(pathname); // NOTE: pass a copy of pathname

    NODE *p = start;

    for (int i=0; i < n; i++){
       p = search_child(p, name[i]);
       if (p==0) return 0;            // if name[i] does not exist
    }
    return p;
}

int findCmd(char *command)
{
    int i = 0;
    while(cmd[i]){
        if (!strcmp(command, cmd[i])){
            return i; // found command: return index i
        }
        i++;
    }
    return -1; // not found: return -1
}
int initialize()
{
    root = makeNODE("/", 'D');
    cwd = root;
}

int addChild(NODE * wd, NODE * child){
    NODE*pGen = wd;
    if(!(wd->child)){
        wd->child = child;
    }
    else{
        pGen = wd->child;
        while(pGen->sibling != NULL){
            pGen = pGen->sibling;
        }
        pGen->sibling = child;
    }
    child->parent = wd;
}

int removeChild(NODE * wd, NODE * child)
{
    NODE * pGen = wd->child, *pPrev = wd;
    if(pGen != child){
        while(pGen != child){
            pPrev = pGen;
            pGen = pGen->sibling;
        }
        pPrev->sibling = pGen->sibling;
        free(pGen);
    }
    else{
        pPrev->child = pGen->sibling;
        free(pGen);
    }
}

NODE * makeNODE(char *name, char type)
{
    NODE * pNew = (NODE *)malloc(sizeof(NODE));
    strcpy(pNew->name,name);
    pNew->type = type;
    pNew->child = pNew->parent = pNew->sibling = NULL;
    return pNew;
}

int main(){
    printf("Unix/Linux File System Tree Simulator.\n\n");    
    initialize(); //initialize root node of the file system tree
    while(1){




        printf("~$: %s ",pwdHelp(cwd));
        fgets(line, 128, stdin); // get at most 128 chars from stdin
        line[strlen(line)-1] = 0; // kill \n at end of line
        sscanf(line, "%s %s", command, pathname);
        if (findCmd(command) >= 0)
        {
            int r = fptr[findCmd(command)](pathname);
        }





        else
        {
            printf("Invalid Input: Use the menu command for help\n");
        }
        memset(pathname,0,strlen(pathname));
    }
    return 0;
}
