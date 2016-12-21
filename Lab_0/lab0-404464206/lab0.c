#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>

void handler(int arg) {
  if (arg == SIGSEGV) {
    perror("Caught Segmentation Fault!");
    fprintf(stderr, "Caught Segmentation Fault!\n");
    exit(3);
  }
}

int main(int argc, char** argv) {
  int ifd, ofd, segFault_flag = 0, rd = 0, BUF_SIZE = 2048;

  // getopt_long usage example from the Linux Programmer's Manual
  while(1) {
    int opt_index = 0;
    static struct option l_opt[] = {
      {"input", required_argument, 0, 'i'},
      {"output", required_argument, 0, 'o'},
      {"segfault", no_argument, 0, 's'},
      {"catch", no_argument, 0, 'c'}
    };

    int s_opt = getopt_long(argc, argv, "i:o:cs", l_opt, &opt_index);
    if (s_opt == -1)
      break;

    switch (s_opt) {

      // --input redirects the standard input to the specified input file
    case 'i':
      ifd = open(optarg, O_RDONLY);
      if (ifd >= 0) {
	close(STDIN_FILENO); // closes 0's association with STDIN
	dup(ifd);            // associates 0 with the input file
	close(ifd);          // closes ifd's association with the input file
      }
      else {
	perror("Error opening the input file");
	fprintf(stderr, "Error opening the input file");
	exit(1);
      }
      break;
      // --output redirects the standard output to the specified output file
    case 'o':
      // S_IRWXU makes sure the user has read, write, and execute
      // permissions on the created output file.
      ofd = creat(optarg, S_IRWXU);
      if (ofd >= 0) {
	close(STDOUT_FILENO);  // closes 1's association with STDOUT
	dup(ofd);              // associates 1 with the output file
	close(ofd);            // closes ofd's association with the output file
      }
      else {
	perror("Error creating the output file");
	fprintf(stderr, "Error creating the output file");
	exit(2);
      }
      break;
      // --segfault forces a segmentation fault by storing through a null pointer
    case 's':
      segFault_flag = 1;
      break;
      // --catch uses a SIGSEGV handler to catch a segmentation fault
    case 'c':
      signal(SIGSEGV, handler);
      break;
    }
  }
  
  if (segFault_flag) {
    char* ptr = NULL;
    *ptr = '7';
  }
  
  char buf[BUF_SIZE];
  rd = read(0, buf, sizeof(buf));
  while (rd > 0) {
    write(1, buf, rd);
    rd = read(0, buf, sizeof(buf));
  }
  
  return 0;
}
