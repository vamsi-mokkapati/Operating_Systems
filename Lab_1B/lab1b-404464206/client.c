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
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mcrypt.h>

#define BUF_SIZE 4096

int sock_fd, log_flag = 0, log_fd = 0, encrypt_flag = 0;
pthread_t thr_id;
struct termios svdAttr;
MCRYPT encr_fd, decr_fd;

void resetInput(void) {
  if (encrypt_flag) {
    mcrypt_generic_deinit(encr_fd);
    mcrypt_generic_deinit(decr_fd);
    mcrypt_module_close(encr_fd);
    mcrypt_module_close(decr_fd);
  }
  tcsetattr (STDIN_FILENO, TCSANOW, &svdAttr);
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

void handler(int arg) {
  if (!arg) { // EOF
    write(sock_fd, "0", 1);
    close(sock_fd);
    exit(1);
  }
  else { // Ctrl-D
    pthread_cancel(thr_id);
    close(sock_fd);
    exit(0);
  }
}

void log_write(int readBytes, int indx, char* buff, int received) {
  if (received) {
    char recMsg[18] = "RECEIVED 1 bytes: ";
    if (write(log_fd, recMsg, 18) < 0) {
      perror("Writing to log error!");
      exit(EXIT_FAILURE);
    }
    if (write(log_fd, buff+indx, 1) < 0) {
      perror("Writing to log error!");
      exit(EXIT_FAILURE);
    }
  }
  else {
    char sntMsg[14] = "SENT 1 bytes: ";
    if (write(log_fd, sntMsg, 14) < 0) {
      perror("Writing to log error!");
      exit(EXIT_FAILURE);
    }
    if (write(log_fd, buff+indx, 1) < 0) {
      perror("Writing to log error!");
      exit(EXIT_FAILURE);
    }
  }
  
  if (write(log_fd, "\n", 1) < 0) {
      perror("Writing to log error!");
      exit(EXIT_FAILURE);
    }

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

    // Exits with a return code of 1
    else if (rd == 0)
      handler(0);

    if (rd_fd == sock_fd) {
      
      // Write to log first so that incoming data is recorded pre-decryption
      if (log_flag == 1) {
        for (int i = 0; i < rd; i++)
          log_write(rd, i, buf, 1); // 1 is received, 0 is sent
      }
      
      // Complete decryption process
      if (encrypt_flag == 1) {
	if (mdecrypt_generic(decr_fd, buf, rd) != 0) {
	  perror("Decryption error!");
	  exit(EXIT_FAILURE);
	}
      }
    }

    while (index < rd) {
      // Maps <cr> or <lf> into <cr><lf>; also makes sure only <lf> goes to the shell
      if (buf[index] == '\r' || buf[index] == '\n') {
        char combined[] = "\r\n";
        write(1, &combined, 2);
        if (wr_fd == sock_fd) {
	  char newline = '\n';
	  if (encrypt_flag) {
	    if (mcrypt_generic(encr_fd, &newline, 1) != 0) {
	      perror("Encryption error!");
	      exit(EXIT_FAILURE);
	    }
	  }
	  if (log_flag) {
	    log_write(rd, index, buf, 0);
	  }
          write(wr_fd, &newline, 1);
        }
      }

      else if (buf[index] == 0x04) {
	if (!rd_fd)
	  handler(1);
	else
	  handler(0);
      }
      
      // Writes to standard output in the terminal and the shell
      else {
        write(1, buf+index, 1);
        if (wr_fd == sock_fd) {
	  if (encrypt_flag) {
	    if (mcrypt_generic(encr_fd, buf+index, 1) != 0) {
              perror("Encryption error!");
              exit(EXIT_FAILURE);
            }
	  }
	  if (log_flag) {
	    log_write(rd, index, buf, 0);
	  }
          write(wr_fd, buf+index, 1);
        }
      }
      index++;
    }
  }
}

void* threadFunc() {
  read_and_write(sock_fd, 1);
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
  struct sockaddr_in srv_addr;
  struct hostent *srv;
  
  /* getopt_long usage example from the Linux Programmer's Manual */
  while(1) {
    int opt_index = 0;
    static struct option l_opt[] = {
      {"port", required_argument, 0, 'p'},
      {"log", required_argument, 0, 'l'},
      {"encrypt", no_argument, 0, 'e'}
    };

    int s_opt = getopt_long(argc, argv, "ep:l:", l_opt, &opt_index);
    if (s_opt == -1)
      break;
    switch (s_opt) {
    case 'p':
      port_no = atoi(optarg);
      break;
    case 'l':
      log_flag = 1;
      log_fd = creat(optarg, S_IRWXU);
      if (log_fd < 0) {
	perror("Error opening log file!");
	exit(EXIT_FAILURE);
      }
      
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
  
  /* Puts the console into character-at-a-time, no-echo mode */
  set_noncanonical();

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("Error opening the socket!");
    exit(EXIT_FAILURE);
  }
  srv = gethostbyname("localhost");
  if (srv == NULL) {
    fprintf(stderr, "No such host detected!\n");
    exit(0);
  }
  memset((char *) &srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_family = AF_INET;
  memcpy((char *)srv->h_addr,
	 (char *)&srv_addr.sin_addr.s_addr,
	 srv->h_length);
  srv_addr.sin_port = htons(port_no);
  if (connect(sock_fd,(struct sockaddr *)&srv_addr,sizeof(srv_addr)) < 0) {
    perror("Connection error!");
    exit(EXIT_FAILURE);
  }
  pthread_create(&thr_id, NULL, threadFunc, NULL);
  read_and_write(0, sock_fd);

  return 0;
}
