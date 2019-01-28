/*
Programmer: Zeid Al-Ameedi
PA:         #2
Description: Mimics the behavior of unix file systems by use of data structures
*/

#include<stdio.h>
#include<string.h>

//global variables
struct node *root;
struct node *pwd;
char line[128];
char command[16];
char pathname[64];
char dname[64];
char bname[64];


//Each node within the file system contains the following data entries
typedef struct node
{
    char name[64];
    char type;
    struct node *childPtr;
    struct node *siblingPtr;
    struct node *parentPtr;
}node;

char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "reload",
                    "save", "menu", "quit", NULL};
    //command table that contains a bunch of options for us to check 


//Function prototypes
int findCmd(char *command);
void initialize(); //This will create root/file system

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

    char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "reload",
                    "save", "menu", "quit", NULL};
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

void initialize()
{
    return 0;
}