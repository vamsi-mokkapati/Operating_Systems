#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE 4096

int pipe_to_child[2];
int pipe_from_child[2];
struct termios svdAttr;
int shell_flag = 0;
pthread_t thr_id;
pid_t ch_pid = -1;

void resetInput(void) {
  tcsetattr (STDIN_FILENO, TCSANOW, &svdAttr);
  if (shell_flag) {
    int ch_status;
    waitpid(ch_pid, &ch_status, 0);
    if (WIFEXITED(ch_status))
      printf("The child process's exit status is %d\n", WEXITSTATUS(ch_status));
    else if (WIFSIGNALED(ch_status))
      printf("The child process exited because of signal %d\n", WTERMSIG(ch_status));
    else
      printf("The child process exited normally.\n");
  }
}

void handler(int arg) {
  if (arg == SIGPIPE)
    exit(1);
  else if (arg == SIGINT)
    kill(ch_pid, SIGINT);
  else {
    fprintf(stderr, "Signal handler received unexpected argument!");
    perror("Signal handler received unexpected argument!");
  }
}

void set_noncanonical (void) {
  struct termios tattr;

  /* First, make sure stdin is a terminal */
  if (!isatty (STDIN_FILENO)) {
    fprintf(stderr, "Stdin is not a valid terminal.\n");
    exit (EXIT_FAILURE);
  }

  /* Then save terminal attributes so they can be later restored */
  tcgetattr (STDIN_FILENO, &svdAttr);
  atexit (resetInput);

  /* No echo, non-canonical terminal mode set */
  tcgetattr (STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON|ECHO); /* Clears ICANON and ECHO */
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

void read_and_write(int rd_fd, int wr_fd) {
  char buf[BUF_SIZE];
  while (1) {
    int index = 0;
    int rd = read(rd_fd, buf, BUF_SIZE);

    // Checks for a successful read
    if (rd < 0) {
      fprintf(stderr, "Reading Error!");
      perror("Reading Error!");
      exit(EXIT_FAILURE);
    }

    // Exits with a return code of 1 when it receives an EOF from the shell
    else if (rd_fd == pipe_from_child[0] && rd == 0)
      exit(1);
    
    while (index < rd) {
      // Maps <cr> or <lf> into <cr><lf>; also makes sure only <lf> goes to the shell
      if (buf[index] == '\r' || buf[index] == '\n') {
	char combined[] = "\r\n";
	write(1, &combined, 2);
	if (wr_fd == pipe_to_child[1]) {
	  char newline = '\n';
	  write(wr_fd, &newline, 1);
	}
      }

      // Exits on Ctrl-D
      else if (buf[index] == 0x04 && shell_flag == 0) {
	exit(0);
      }
      else if (buf[index] == 0x04 && shell_flag == 1) {
	pthread_cancel(thr_id);
	close(pipe_to_child[0]);
	close(pipe_to_child[1]);
	close(pipe_from_child[0]);
	close(pipe_from_child[1]);
	kill(ch_pid, SIGHUP);
	exit(0);
      }

      // Writes to standard output in the terminal and the shell
      else {
	write(1, buf+index, 1);
	if (wr_fd == pipe_to_child[1]) {
	  write(wr_fd, buf+index, 1);
	}
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

int main(int argc, char** argv) {  
  /* getopt_long usage example from the Linux Programmer's Manual */
  while(1) {
    int opt_index = 0;
    static struct option l_opt[] = {
      {"shell", no_argument, 0, 's'}
    };
    
    int s_opt = getopt_long(argc, argv, "s", l_opt, &opt_index);
    if (s_opt == -1)
      break;
    switch (s_opt) {
    case 's':
      shell_flag = 1;
      break;
    default:
      break;
    }
  }
  /* Puts the console into character-at-a-time, no-echo mode */
  set_noncanonical();
  
  if (!shell_flag) {
    read_and_write(0, 1);
  }
  else {
    signal(SIGPIPE, handler);
    signal(SIGINT, handler);
    
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
  return 0;
}