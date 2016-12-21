#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>
#include <stdbool.h>
#include <mcrypt.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>

#define BUF_SIZE 4096

pthread_t thr_id;
struct termios svdAttr;
int encrypt_flag = 0, sock_fd, new_sock_fd, clnt_len;
pid_t ch_pid = -1;
MCRYPT encr_fd, decr_fd;
int pipe_to_child[2];
int pipe_from_child[2];

void exit_pipes_and_fds() {
  close(sock_fd);
  close(pipe_to_child[1]);
  close(pipe_from_child[0]);
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  kill(ch_pid, SIGKILL);
}

void read_and_write(int rd_fd, int wr_fd) {
  char buf[BUF_SIZE];
  while (1) {
    int index = 0;
    int rd = read(rd_fd, buf, BUF_SIZE);

    // Checks for a successful read
    if (rd < 0 && rd_fd != 0) {
      fprintf(stderr, "Reading Error!");
      perror("Reading Error!");
      exit(EXIT_FAILURE);
    }

    else if (rd_fd == 0) {
      if (rd <= 0) {
	exit_pipes_and_fds();
	exit(1);
      }
      if (encrypt_flag) {
	if (mdecrypt_generic(decr_fd, buf, rd) != 0) {
	  perror("Decryption error!");
          exit(EXIT_FAILURE);
	}
      }
    }

    // Exits with a return code of 2 when it receives an EOF from the shell
    else if (rd_fd == pipe_from_child[0] && rd == 0) {
      exit_pipes_and_fds();
      exit(2);
    }

    while (index < rd) {
      if (wr_fd == 1 && encrypt_flag == 1) {
	if (mcrypt_generic(encr_fd, buf+index, 1) != 0) {
	  perror("Encryption error!");
	  exit(EXIT_FAILURE);
	}
      }
      if (write(wr_fd, buf+index, 1) < 0) {
	perror("Error writing!");
	exit(EXIT_FAILURE);
      }
      index++;
    }
  }
}


void* threadFunc() {
  close(pipe_from_child[1]);
  read_and_write(pipe_from_child[0], 1);
  exit(0);
}

void encryptFunc(char* key_file, char* encr_key, int key_fd, int key_len) {
  key_fd = open(key_file, O_RDONLY);
  if (key_fd < 0) {
    perror("Error opening key file!");
    exit(EXIT_FAILURE);
  }
  struct stat key_stats;
  fstat(key_fd, &key_stats);
  key_len = key_stats.st_size;
  encr_key = (char *) malloc (key_len * sizeof(char *));
  if (read(key_fd, encr_key, key_len) < 0) {
    perror("Key read error!");
    exit(EXIT_FAILURE);
  }

  encr_fd = mcrypt_module_open("blowfish", NULL, "ofb", NULL);
  if (encr_fd == MCRYPT_FAILED) {
    perror("Mcrypt module open failed!");
    exit(EXIT_FAILURE);
  }

  if (mcrypt_generic_init(encr_fd, encr_key, key_len, NULL) < 0) {
    perror("Generic encrypt initialization error!");
    exit(EXIT_FAILURE);
  }

  decr_fd = mcrypt_module_open("blowfish", NULL, "ofb", NULL);
  if (decr_fd == MCRYPT_FAILED) {
    perror("Mcrypt module open failed!");
    exit(EXIT_FAILURE);
  }

  if (mcrypt_generic_init(decr_fd, encr_key, key_len, NULL) < 0) {
    perror("Generic decrypt initialization error!");
    exit(EXIT_FAILURE);
  }
}


int main(int argc, char** argv) {
  int port_no, myKey_fd = 0, myKey_size = 0;
  char* encr_key;
  struct sockaddr_in srv_addr, clnt_addr;
  
  /* getopt_long usage example from the Linux Programmer's Manual */
  while(1) {
    int opt_index = 0;
    static struct option l_opt[] = {
      {"port", required_argument, 0, 'p'},
      {"encrypt", no_argument, 0, 'e'}
    };

    int s_opt = getopt_long(argc, argv, "ep:", l_opt, &opt_index);
    if (s_opt == -1)
      break;
    switch (s_opt) {
    case 'p':
      port_no = atoi(optarg);
      break;
    case 'e':
      encrypt_flag = 1;
      encryptFunc("my.key", encr_key, myKey_fd, myKey_size);
      break;
    default:
      exit(EXIT_FAILURE);
      break;
    }
  }

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("Error opening the socket!");
    exit(EXIT_FAILURE);
  }
  memset((char *) &srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_addr.s_addr = INADDR_ANY;
  srv_addr.sin_port = htons(port_no);
  if (bind(sock_fd, (struct sockaddr *) &srv_addr,
	   sizeof(srv_addr)) < 0) {
    perror("Binding error!");
    exit(EXIT_FAILURE);
  }
  listen(sock_fd, 5);
  clnt_len = sizeof(clnt_addr);
  new_sock_fd = accept(sock_fd, (struct sockaddr *) &clnt_addr, &clnt_len);
  if (new_sock_fd < 0) {
    perror("Accept error!");
    exit(EXIT_FAILURE);
  }

  // Socket redirection
  dup2(new_sock_fd, STDIN_FILENO);
  dup2(new_sock_fd, STDOUT_FILENO);
  dup2(new_sock_fd, STDERR_FILENO);
  close(new_sock_fd);
  
  // Forking process
  if (pipe(pipe_to_child) == -1 || pipe(pipe_from_child) == -1) {
      fprintf(stderr, "Pipe failure!\n");
      perror("Pipe failure!\n");
      exit(EXIT_FAILURE);
  }

  int thrReturn;
  ch_pid = fork();

  if (ch_pid > 0) { // parent process                                                         
      thrReturn = pthread_create(&thr_id, NULL, threadFunc, NULL);
      close(pipe_to_child[0]);
      read_and_write(0, pipe_to_child[1]);
  }
  else if (ch_pid == 0) { // child process
    close(pipe_to_child[1]);
    close(pipe_from_child[0]);
    dup2(pipe_to_child[0], STDIN_FILENO);
    dup2(pipe_from_child[1], STDOUT_FILENO);
    close(pipe_to_child[0]);
    close(pipe_from_child[1]);
    
    char *execvp_argv[2];
    char execvp_filenm[] = "/bin/bash";
    execvp_argv[0] = execvp_filenm;
    execvp_argv[1] = NULL;
    if (execvp(execvp_filenm, execvp_argv) == -1) {
      fprintf(stderr, "Execvp failure!\n");
      perror("Execvp failure!\n");
      exit(EXIT_FAILURE);
    }
  }
  else { // fork failure!
    fprintf(stderr, "Fork failure!\n");
    perror("Fork failure!\n");
    exit(EXIT_FAILURE);
  }
}
