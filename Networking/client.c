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
struct hostent *hp;
struct sockaddr_in server_addr;
int server_sock, r;
int SERVER_IP, SERVER_PORT;

// LS Globals
struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

// INIT
int client_init(char *argv[]) {
  printf("CONNECTING\n");
  printf("--getting server info\n");
  hp = gethostbyname(argv[1]);
  if (!hp) {
    printf("unknown host %s\n", argv[1]);
    exit(1);
  }
  SERVER_IP = *(long *)hp->h_addr_list[0];
  SERVER_PORT = atoi(argv[2]);
  printf("--creating TCP socket\n");
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    printf("socket call failed\n");
    exit(2);
  }
  printf("--filling server_addr with server's IP and PORT#\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = SERVER_IP;
  server_addr.sin_port = htons(SERVER_PORT);
  // Connect to server
  printf("--connecting to server ....\n");
  r = connect(server_sock, (struct sockaddr *)&server_addr,
              sizeof(server_addr));
  if (r < 0) {
    printf("connect failed\n");
    exit(1);
  }
  printf("--hostname=%s  IP=%s  PORT=%d\n", hp->h_name,
         inet_ntoa((struct in_addr){SERVER_IP}), SERVER_PORT);
  printf("--connection to %s success\n", hp->h_name);
  printf("CONNECTED\n");
  return 0;
}

// UTILITY
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

int send_cmd(cmd *c) {
  char s[MAX], *ps = s;
  // recombine cmd
  int i = 0;
  for (; i < c->argc - 1; i++)
    ps = stpcpy(stpcpy(ps, c->argv[i]), " ");
  ps = stpcpy(ps, c->argv[i]);
  // write command
  printf("send cmd: %s\n", s);
  return write(server_sock, s, MAX);
}

int read_resp() {
  char resp[MAX];
  int n = read(server_sock, resp, MAX);
  puts(resp);
  return n;
}

// LOCALS
int do_lcat(cmd *c) {
  int fd_out = STDIN_FILENO, fd_in, m = 0, n;
  char buf[MAX];
  if (c->argc > 1)
    fd_in = open(c->argv[1], O_RDONLY);
  if (fd_in < 0)
    return puts("could no open file");
  while ((n = read(fd_in, buf, MAX)))
    m += write(fd_out, buf, n);
  return m;
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

int do_lls(cmd *c) {
  int argc = c->argc;
  char **argv = c->argv;
  struct stat mystat, *sp = &mystat;
  int r;
  char *filename, path[1024], cwd[256];
  filename = "./"; // default to CWD
  if (argc > 1)
    filename = argv[1]; // if specified a filename
  if ((r = lstat(filename, sp)) < 0)
    return write(STDOUT_FILENO, "no such file\n", MAX);
  strcpy(path, filename);
  if (path[0] != '/') { // filename is relative make absolute
    getcwd(cwd, 256);
    strcpy(path, cwd);
    strcat(path, "/");
    strcat(path, filename);
  }
  if (S_ISDIR(sp->st_mode))
    ls_dir(path, STDOUT_FILENO);
  else
    ls_file(path, STDOUT_FILENO);
  return write(STDOUT_FILENO, "***", 3);
}

int do_lcd(cmd *c) {
  if (c->argc > 1)
    chdir(c->argv[1]);
  else
    chdir(getenv("HOME"));
  char cwd[MAX];
  getcwd(cwd, sizeof(cwd));
  printf("cd to %s\n", cwd);
  return 0;
}
int do_lpwd(cmd *c) {
  char cwd[MAX];
  getcwd(cwd, sizeof(cwd));
  printf("cwd: %s\n", cwd);
  return 0;
}

int do_lmkdir(cmd *c) {
  int n = 1;
  if (c->argc > 1)
    n = mkdir(c->argv[1], 0777);
  printf("mkdir: %s\n", (!n) ? "success" : "failure");
  return 0;
}

int do_lrmdir(cmd *c) {
  int n = 1;
  if (c->argc > 1)
    n = rmdir(c->argv[1]);
  printf("rmdir: %s\n", (!n) ? "success" : "failure");
  return 0;
}

int do_lrm(cmd *c) {
  int n = 1;
  if (c->argc > 1)
    n = remove(c->argv[1]);
  printf("lrm: %s\n", (!n) ? "success" : "failure");
  return 0;
}

int do_quit(cmd *c) {
  puts("Goodbye");
  exit(0);
}

// REMOTES
int do_get(cmd *c) {
  send_cmd(c);
  char line[MAX] = {"\0"};
  int n = 0;
  int f_size;
  char f_name[MAX];
  read(server_sock, line, MAX);
  sscanf(line, "%s %d", f_name, &f_size);
  if (!f_size) {
    return printf("File: %s, FAILED", line);
  }
  int fd = open(f_name, O_WRONLY | O_CREAT, 0777);
  while (n < f_size) {
    n += read(server_sock, line, MAX);
    write(fd, line, MAX);
  }
  puts("file transfer complete");
  return n;
}

int do_put(cmd *c) {
  char buf[MAX] = {"\0"};
  int n = 0, f_size;
  // check if arg
  if (c->argc < 2)
    return puts("no file specified");
  // check file exist
  if (access(c->argv[1], F_OK))
    return puts("file not found");
  // send command to server
  send_cmd(c);
  // get file size
  struct stat st;
  stat(c->argv[1], &st);
  f_size = st.st_size;
  sprintf(buf, "%s %d", c->argv[1], f_size);
  // print special first line
  write(server_sock, buf, MAX);
  // open file
  int fd = open(c->argv[1], O_RDONLY);
  // write file contents to server
  do {
    n += read(fd, buf, MAX);
    write(server_sock, buf, MAX);
  } while (n < f_size);
  puts("file sent");
  return n;
}

int do_ls(cmd *c) {
  send_cmd(c);
  char resp[MAX] = {"\0"};
  int n = 0;
  do {
    n += read(server_sock, resp, MAX);
    puts(resp);
  } while (!strstr(resp, "***"));
  return n;
}
int do_cd(cmd *c) {
  send_cmd(c);
  return read_resp();
}
int do_pwd(cmd *c) {
  send_cmd(c);
  return read_resp();
}
int do_mkdir(cmd *c) {
  send_cmd(c);
  return read_resp();
}
int do_rmdir(cmd *c) {
  send_cmd(c);
  return read_resp();
}
int do_rm(cmd *c) {
  send_cmd(c);
  return read_resp();
}

// MASTER COMMANDER
int do_cmd(cmd *c) {
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
  } else if (!strcmp(c->argv[0], "lcat")) {
    do_lcat(c);
  } else if (!strcmp(c->argv[0], "lls")) {
    do_lls(c);
  } else if (!strcmp(c->argv[0], "lcd")) {
    do_lcd(c);
  } else if (!strcmp(c->argv[0], "lpwd")) {
    do_lpwd(c);
  } else if (!strcmp(c->argv[0], "lmkdir")) {
    do_lmkdir(c);
  } else if (!strcmp(c->argv[0], "lrmdir")) {
    do_lrmdir(c);
  } else if (!strcmp(c->argv[0], "lrm")) {
    do_lrm(c);
  } else if (!strcmp(c->argv[0], "quit")) {
    do_quit(c);
  } else {
    puts("Bad command");
  }
  return 0;
}

// MAIN
int main(int argc, char *argv[]) {
  int n;
  char line[MAX];
  if (argc < 3) {
    printf("Usage : client ServerName SeverPort\n");
    exit(1);
  }
  client_init(argv);
  // sock <---> server
  while (1) {
    printf("\n"
           "----------------------------------------\n"
           "get  put ls  cd   pwd    mkdir  rmdir rm\n"
           "lcat lls lcd lpwd lmkdir lrmdir lrm quit\n"
           "----------------------------------------\n");
    printf("@%s: ", hp->h_name);
    // get user input
    bzero(line, MAX);           // zero out line[ ]
    fgets(line, MAX, stdin);    // get a line (end with \n) from stdin
    line[strlen(line) - 1] = 0; // kill \n at end
    if (!line[0])               // exit if NULL line
      exit(1);
    // do command
    cmd c;
    parse_input(line, &c);
    do_cmd(&c);
  }
  return 0;
}