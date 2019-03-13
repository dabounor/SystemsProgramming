#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX 256

// Define types
typedef struct cmd {
  char *argv[256];
  int argc;
} cmd;

// Globals
struct sockaddr_in server_addr, client_addr, name_addr;
struct hostent *hp;
int mysock, client_sock; // socket descriptors
int serverPort;          // server port number

// LS Globals
struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

// Server initialization
int server_init(char *name) {
  int r, length; // help variables
  printf("==================== server init ======================\n");
  // get DOT name and IP address of this host
  printf("1 : get and show server host info\n");
  hp = gethostbyname(name);
  if (!hp) {
    printf("unknown host\n");
    exit(1);
  }
  printf("    hostname=%s  IP=%s\n", hp->h_name,
         inet_ntoa((struct in_addr){(long)hp->h_addr_list[0]}));
  //  create a TCP socket by socket() syscall
  printf("2 : create a socket\n");
  mysock = socket(AF_INET, SOCK_STREAM, 0);
  if (mysock < 0) {
    printf("socket call failed\n");
    exit(2);
  }
  printf("3 : fill server_addr with host IP and PORT# info\n");
  // initialize the server_addr structure
  server_addr.sin_family = AF_INET;                // for TCP/IP
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // THIS HOST IP address
  server_addr.sin_port = 0;                        // let kernel assign port
  printf("4 : bind socket to host info\n");
  // bind syscall: bind the socket to server_addr info
  r = bind(mysock, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0) {
    printf("bind failed\n");
    exit(3);
  }
  printf("5 : find out Kernel assigned PORT# and show it\n");
  // find out socket port number (assigned by kernel)
  length = sizeof(name_addr);
  r = getsockname(mysock, (struct sockaddr *)&name_addr, (socklen_t *)&length);
  if (r < 0) {
    printf("get socketname error\n");
    exit(4);
  }
  // show port number
  serverPort = ntohs(name_addr.sin_port); // convert to host ushort
  printf("    Port=%d\n", serverPort);
  // listen at port with a max. queue of 5 (waiting clients)
  printf("5 : server is listening ....\n");
  listen(mysock, 5);
  printf("===================== init done =======================\n");
  return 0;
}

// LS
int ls_file(char *fname, int fd) {
  char ftime[MAX], buf[MAX] = {0}, *p_buf = buf, bufbuf[MAX] = {0};
  struct stat fstat, *sp = &fstat;
  int r, i;
  if ((r = lstat(fname, &fstat)) < 0)
    return write(fd, "can't stat \n", MAX);
  if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
    p_buf = stpcpy(p_buf, "-");
  if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    p_buf = stpcpy(p_buf, "d");
  if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
    p_buf = stpcpy(p_buf, "l");
  for (i = 8; i >= 0; i--) {
    if (sp->st_mode & (1 << i)) // print r|w|x
    {
      sprintf(bufbuf, "%c", t1[i]);
      p_buf = stpcpy(p_buf, bufbuf);
    } else {
      sprintf(bufbuf, "%c", t2[i]);
      p_buf = stpcpy(p_buf, bufbuf);
    }
  }
  sprintf(bufbuf, "%4d %4d %4d %8d", (int)sp->st_nlink, sp->st_gid, sp->st_uid,
          (int)sp->st_size);
  p_buf = stpcpy(p_buf, bufbuf);
  strcpy(ftime, ctime(&sp->st_ctime));
  ftime[strlen(ftime) - 1] = 0;
  sprintf(bufbuf, " %s %s", ftime, basename(fname));
  p_buf = stpcpy(p_buf, bufbuf);
  if ((sp->st_mode & 0xF000) == 0xA000) {
    // use readlink() to read linkname
    char linkname[256] = {0};
    readlink(fname, linkname, 256);
    sprintf(bufbuf, " -> %s", linkname);
    p_buf = stpcpy(p_buf, bufbuf); // print linked name
  }
  p_buf = stpcpy(p_buf, "\n");
  write(fd, buf, MAX);
  return 0;
}

int ls_dir(char *dirname, int fd) {
  DIR *dir = opendir(dirname);
  struct dirent *dirdir = 0;
  while ((dirdir = readdir(dir))) {
    ls_file(dirdir->d_name, fd);
  }
  return 0;
}

int do_ls(cmd *c) {
  int argc = c->argc;
  char **argv = c->argv;
  struct stat mystat, *sp = &mystat;
  int r;
  char *filename, path[1024], cwd[256];
  filename = "./"; // default to CWD
  if (argc > 1)
    filename = argv[1]; // if specified a filename
  if ((r = lstat(filename, sp)) < 0)
    return write(client_sock, "no such file\n", MAX);
  strcpy(path, filename);
  if (path[0] != '/') { // filename is relative make absolute
    getcwd(cwd, 256);
    strcpy(path, cwd);
    strcat(path, "/");
    strcat(path, filename);
  }
  if (S_ISDIR(sp->st_mode))
    ls_dir(path, client_sock);
  else
    ls_file(path, client_sock);
  return write(client_sock, "***", 3);
}

int do_cd(cmd *c) {
  char buf[MAX] = {0};
  if (c->argc > 1)
    chdir(c->argv[1]);
  else
    chdir(getenv("HOME"));
  char cwd[MAX];
  getcwd(cwd, sizeof(cwd));
  sprintf(buf, "cd to %s\n", cwd);
  write(client_sock, buf, MAX);
  return 0;
}

int do_pwd(cmd *c) {
  char cwd[MAX];
  char buf[MAX] = {0};
  getcwd(cwd, sizeof(cwd));
  sprintf(buf, "cwd: %s\n", cwd);
  write(client_sock, buf, MAX);
  return 0;
}

// TODO CHECK FOR ARGV1
int do_mkdir(cmd *c) {
  char buf[MAX] = {0};
  int n = 1;
  if (c->argc > 1)
    n = mkdir(c->argv[1], 0777);
  sprintf(buf, "mkdir: %s\n", (!n) ? "success" : "failure");
  write(client_sock, buf, MAX);
  return 0;
}

int do_rmdir(cmd *c) {
  char buf[MAX] = {0};
  int n = 1;
  if (c->argc > 1)
    n = rmdir(c->argv[1]);
  sprintf(buf, "rmdir: %s\n", (!n) ? "success" : "failure");
  write(client_sock, buf, MAX);
  return 0;
}

int do_rm(cmd *c) {
  char buf[MAX] = {0};
  int n = 1;
  if (c->argc > 1)
    n = unlink(c->argv[1]);
  sprintf(buf, "rmdir: %s\n", (!n) ? "success" : "failure");
  write(client_sock, buf, MAX);
  return 0;
}

int do_get(cmd *c) {
  char buf[MAX] = {"\0"};
  int n = 0, f_size;
  // check if arg and if file exists
  if (c->argc < 2 || access(c->argv[1], F_OK))
    return write(client_sock, "nofile 0", MAX);
  // get file size
  struct stat st;
  stat(c->argv[1], &st);
  f_size = st.st_size;
  sprintf(buf, "%s %d", c->argv[1], f_size);
  // print special first line
  write(client_sock, buf, MAX);
  // open file
  int fd = open(c->argv[1], O_RDONLY);
  // write file contents to server
  do {
    n += read(fd, buf, MAX);
    write(client_sock, buf, MAX);
  } while (n <= f_size);
  puts("file sent");
  return n;
}

int do_put(cmd *c) {
  char line[MAX] = {"\0"};
  int n = 0;
  int f_size = 0;
  char f_name[MAX];
  read(client_sock, line, MAX);
  sscanf(line, "%s %d", f_name, &f_size);
  if (!f_size) {
    return printf("File: %s, FAILED", line);
  }
  int fd = open(f_name, O_WRONLY | O_CREAT, 0777);
  while (n < f_size - MAX) {
    n += read(client_sock, line, MAX);
    write(fd, line, MAX);
  }
  n += read(client_sock, line, MAX);
  write(fd, line, f_size % MAX);
  puts("file transfer complete");
  return n;
}

// MASTER COMMANDER
int do_cmd(cmd *c) {
  printf("do %s with %d args\n", c->argv[0], c->argc - 1);
  if (!strcmp(c->argv[0], "pwd")) {
    do_pwd(c);
  } else if (!strcmp(c->argv[0], "ls")) {
    do_ls(c);
  } else if (!strcmp(c->argv[0], "cd")) {
    do_cd(c);
  } else if (!strcmp(c->argv[0], "mkdir")) {
    do_mkdir(c);
  } else if (!strcmp(c->argv[0], "rmdir")) {
    do_rmdir(c);
  } else if (!strcmp(c->argv[0], "rm")) {
    do_rm(c);
  } else if (!strcmp(c->argv[0], "get")) {
    do_get(c);
  } else if (!strcmp(c->argv[0], "put")) {
    do_put(c);
  } else {
    write(client_sock, "cmd FAIL", MAX);
  }
  return 0;
}
int parse_input(char *line, cmd *c) {
  // split by whitespace into cmd struct
  int i = 0;
  char *s = strtok(line, " \t");
  for (; s; i++) {
    c->argv[i] = s;
    s = strtok(NULL, " \t");
  }
  c->argc = i;
  c->argv[i] = NULL;
  return i;
}
int main(int argc, char *argv[]) {
  int length, n; // help variables

  char *hostname;
  char line[MAX];
  if (argc < 2)
    hostname = "localhost";
  else
    hostname = argv[1];
  server_init(hostname);
  // Try to accept a client request
  while (1) {
    printf("server: accepting new connection ....\n");
    // Try to accept a client connection as descriptor newsock
    length = sizeof(client_addr);
    client_sock =
        accept(mysock, (struct sockaddr *)&client_addr, (socklen_t *)&length);
    if (client_sock < 0) {
      printf("server: accept error\n");
      exit(1);
    }
    printf("server: accepted a client connection from\n");
    printf("-----------------------------------------------\n");
    printf("        IP=%s  port=%d\n",
           inet_ntoa((struct in_addr){client_addr.sin_addr.s_addr}),
           ntohs(client_addr.sin_port));
    printf("-----------------------------------------------\n");
    // Processing loop: newsock <----> client
    while (1) {
      // read thing
      n = read(client_sock, line, MAX);
      if (!n) {
        printf("server: client died, server loops\n");
        close(client_sock);
        break;
      }
      // split by whitespace into cmd struct
      struct cmd *c;
      parse_input(line, c);
      // do command
      do_cmd(c);
    }
  }
  return 0;
}
