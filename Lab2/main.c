/*
Programmer: Zeid Al-Ameedi
PA:         #2
Description: Unix file system simulation
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<libgen.h>

//global variables
struct node *root; //root node
//struct node *pwd; //print working directory //turned into a function
struct node *cwd; //current working directory
struct node *start;
struct node *end;
struct node *tempNode;

FILE *fp;

char line[128];
char command[16];
char pathname[64];
char dname[64]; //once decomposed will contain all parents
char bname[64]; //once decomposed will contain the cwd
char *name[100]; //string of ptrs token

int numTokens; //Number of tokens found within the path "/"



//Each node within the file system contains the following data entries
struct node
{
    char name[64];
    char type;
    struct node *childPtr;
    struct node *siblingPtr;
    struct node *parentPtr;
};

char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "reload",
                    "save", "menu", "quit", NULL};
    //command table that contains a bunch of options for us to check 

//int (*fptr[ ])(char *)={(int (*)())mkdir,rmdir,ls,cd,pwd,creat,rm,reload,save,menu,quit};
//function names that correspond to the indice that cmd function produces (function calls)

//Function prototypes
int mkdir(char *pathname);
int rmdir(char *pathname);
int findCmd(char *command);
int ls(char *pathname);
struct node * makeNode(char *name, char newtype);
struct node * nodePath(char *pathname);
struct node *searchChild(struct node *p, char *name);
int removeChild(struct node *n, struct node *child);
int menu(char *filename);
int save(char *filename);
void saveHelp(struct node*pFam, FILE *saveFile);
int reload(char *filename);
int rm(char *pathname);
int create(char *pathname);
int cd(char *pathname);
void pwd(char *pathname);
int insertChildNode(struct node *n, struct node * child);
char *display_pwd(struct node *p);
int quit(char *filename);
void initalize();
int tokenize(char *pathname);
void dbname(char *pathname);









int main()
{
    //Main logic
    initialize();
    printf("Unix/Linux File System Tree Simulator.\n\n");
    while(1)
    {
    printf("~$: ");
    fgets(line, 128, stdin); //Gets 128 lines from stdin and store into global line
    line[strlen(line)-1] = 0; //Gets rid of the new character line
    sscanf(line, "%s %s", command, pathname); //sscanf, extracts from line cmd/pathname

    //Avoid bunch of strcmps/if/switch statement by doing;

    int myCmd = findCmd(command);
    if(myCmd == -1)
    {
        printf("Command not found.\n");
    }
    else 
    {
    //int r = fptr[myCmd](pathname);
    //Switch statement for corresponding command and its function

    //int result = fptr[myCmd](pathname);
    switch(myCmd)
    {
        case 0: mkdir(pathname);
        case 1: rmdir(pathname);
        case 2: ls(pathname);
        case 3: cd(pathname);
        case 4: pwd(pathname);
        case 5: create(pathname);
        case 6: rm(pathname);
        case 7: reload(fp);
        case 8: save(fp);
        case 9: menu(fp);
        case 10: quit(fp);
        default: break;
    }
    if (myCmd == 10)
        break;
    }
    }
    return 0;
}

int mkdir(char *pathname)
{
    dbname(pathname);
    struct node *temp = nodePath(dname);
    if (temp != 0)
    {
        if(temp->type == 'D')
        {
            if(!searchChild(temp,bname)){
                insertChildNode(temp,makeNode(bname,'D'));
            }
            else
            {
                printf("‘%s’: That's a directory\n",pathname);
            }
        }
        else
        {
            printf("‘%s’: Not a directory\n",pathname);
        }
    }
    else
    {
        printf("‘%s’: Error\n",pathname);
    }
    strcpy(dname,"");
    strcpy(bname,"");
}

int rmdir(char *pathname)
{
    dbname(pathname);
    struct node *temp = nodePath(pathname);
    if (temp != 0)
    {
        if(temp->type == 'D')
        {
            if(!(temp->childPtr)){
                removeChild(nodePath(dname),temp);
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

int findCmd(char *command)
{
    int i=0;
    while(cmd[i] != NULL)
    {
        if(!strcmp(command, cmd[i])) //compare your command with current cmd table value
            return i; //found it so return
        i++;
    }

    return -1; //Was not in the table, cmd[i] reached null.
}

int ls(char *pathname)
{
    struct node* temp = nodePath(pathname);
    if (temp){
        temp = temp->childPtr;
    }
    else{
        printf("‘%s’: No such file or directory\n",pathname);
    }
    while(temp){
        printf("\nFileType %c FileName %s ",temp->type,temp->name);
        temp = temp->siblingPtr;
    }
    putc('\n',stdout);
}


struct node * makeNode(char *name, char newtype)
{
    struct node *p = (struct node*)(malloc(sizeof(struct node))); //creates a node
    //now lets initalize all that new nodes info
    strcpy(p->name, name);
    p->type = newtype;
    p->childPtr = NULL;
    p->parentPtr=NULL;
    p->siblingPtr=NULL;
    return p;
}

struct node * nodePath(char *pathname)
{
    if (pathname[0]=="/") //start from root
    {
        start = root;
    }
    else //start from whatever directory you are currently in
    {
        start = cwd; 
    }
    if(!strcmp(pathname, "."))
    {
        strcpy(pathname, "");
    }

    //Now we begin to get the path
    numTokens=tokenize(pathname);
    struct node *p = start; //either from root or cwd
    int index=0;
    for(index=0; index<numTokens; index++)
    {
        p=searchChild(p, name[index]); //once the path is gotten
        if(p==0)
        {
            return 0;
        }
    }

    return p;
}

struct node *searchChild(struct node *p, char *name)
{
    if(!strcmp(name,"..")) //go back to parent
    {
        return p->parentPtr;
    }
    struct node *p2=p->childPtr;
    while(p2 != NULL)
    {
        if(!strcmp(p2->name, name))
        {
            return p2;
        }
        p2=p2->siblingPtr;
    }
    return 0;
}

int removeChild(struct node *n, struct node *child)
{
    struct node *p=n->childPtr;
    struct node *prev = n;

    if(p!=child)
    {
        while(p!=child)
        {
            prev=p;
            p=p->siblingPtr;
        }
        prev->siblingPtr=p->siblingPtr;
        free(p);
    }
}
int menu(char *filename)
{
    //From KC wangs book
    printf("mkdir [pathname] : make a new directory for a given pathname\n");
    printf("rmdir [pathname] : remove the directory, if it is empty.\n");
    printf("cd [pathname] : change CWD to pathname, or to / if no pathname.\n");
    printf("ls [pathname] : list the directory contents of pathname or CWD\n");
    printf("pwd : print the (absolute) pathname of CWD\n");
    printf("creat [pathname] : create a FILE node.\n");
    printf("rm [pathname] : remove the FILE node.\n");
    printf("save [filename[] : save the current file system tree as a file\n");
    printf("reload [filename] : construct a file system tree from a file\n");
    printf("menu : show a menu of valid commands\n");
    printf("quit : save the file system tree, then terminate the program.\n");
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
void saveHelp(struct node*pFam, FILE *saveFile)
{
    if(pFam)
    {
        fprintf(saveFile, "%c %s\n", pFam->type, display_pwd(pFam)); // print a line to file
        saveHelp(pFam->childPtr,saveFile);
        saveHelp(pFam->siblingPtr,saveFile);
    }
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

int rm(char *pathname)
{
    dbname(pathname);
    struct node *temp = nodePath(pathname);
    if (temp != 0)
    {
        if(temp->type == 'F')
        {
            removeChild(nodePath(dname),temp);
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

int create(char *pathname)
{
    dbname(pathname);
    struct node *temp = nodePath(dname);
    if (temp != 0)
    {
        if(temp->type == 'D')
        {
            if(!searchChild(temp,bname)){
                insertChildNode(temp,makeNode(bname,'F'));
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

int cd(char *pathname)
{
    struct node *t = nodePath(pathname);
    if(t != NULL)
    {
        if(t->type == 'D') // if its a directory
        {
            cwd = t;
        }
        else
        {
            printf("Error that directory DNE.\n");
        }
        
    }
}

void pwd(char *pathname)
{
    printf("%s\n", display_pwd(cwd));
}

int insertChildNode(struct node *n, struct node * child)
{
    struct node *p=n;
    if(n->childPtr == NULL)
    {
        n->childPtr=child;

    }

    else
    {
        p=n->childPtr;
        while(p->siblingPtr != NULL)
        {
            p=p->siblingPtr;
        }
        p->siblingPtr=child;
    }
    child->parentPtr=n;
    
}

char *display_pwd(struct node *p)
{
    if(p != root)
    {
        char temp[100]="";
        strcat(temp,display_pwd(p->parentPtr));
        if(strlen(temp) > 1)
        {
            strcat(temp,"/");
        }
        strcat(temp, p->name);
        char *ph=temp;
        return ph;
    }
    else
    {
        return p->name;
    }
    
}

int quit(char *filename)
{
    save(filename);
    exit(0); //calls kernel and exits the program indefinitely
}



void initialize()
{
    root = makeNode("/", 'D'); //makes the root indicated by / and gives it the type of directory
    cwd = root; //Once tree is initalized you must begin from root
}

// int tokenize(char *pathname) //Help code from KCWANG divides into strings based on /
// {
// char *s;
// s = strtok(path, "/");  // first call to strtok()
// while(s){      
// printf("%s ", s);
// s = strtok(0, "/");  // call strtok() until it returns NULL
//     }
// }

int tokenize(char *pathname)
{
    int num=0;
    name[num]=strtok(pathname, "/0");
    while(name[num]!=NULL)
    {
        num++;
        name[num]=strtok(NULL,"/0");
    }
    return num;
}

void dbname(char *pathname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));


    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}




