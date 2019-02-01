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
struct node *pwd; //print working directory
struct node *cwd; //current working directory
struct node *start;
struct node *end;
struct node *tempNode;

FILE *fname;

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
int findCmd(char *command);
void initialize(); //This will create root/file system
struct node * makeNode(char *name, char newtype) //make a node when called



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
    //Switch statement for corresponding command and its function

    //int result = fptr[myCmd](pathname);
    switch(myCmd)
    {
        case 0: printf("mkdir\n"); break;
        case 1: printf("rmdir\n"); break;
        case 2: printf("ls\n"); break;
        case 3: printf("cd\n"); break;
        case 4: printf("pwd\n"); break;
        case 5: printf("creat\n"); break;
        case 6: printf("rm\n"); break;
        case 7: printf("reload\n"); break;
        case 8: printf("save\n"); break;
        case 9: printf("menu\n"); break;
        case 10: printf("quit\n"); break;
        default: break;
    }
    if (myCmd == 10)
        break;
    }
    return 0;
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

int creat(char *pathname)
{
    dbname(pathname);
    struct node *temp = nodePath(dname);
    if(temp != 0)
    {
        //directory first
        if(temp->type == 'D')
        {
            if(!)
        }
    }
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

int pwd(char *pathname)
{
    printf("%s\n", display_pwd(cwd));
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

int tokenize(char *pathname) //Help code from KCWANG divides into strings based on /
{
char *s;
s = strtok(path, "/");  // first call to strtok()
while(s){      
printf(“%s “, s);
s = strtok(0, "/");  // call strtok() until it returns NULL
    }
}

void dbname(char *pathname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));


    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}




