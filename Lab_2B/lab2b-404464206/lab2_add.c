#define _GNU_SOURCE
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

int iter_num = 0, opt_yield = 0, sync_lock = 0;
char sync_flag = 'a';
pthread_mutex_t mutex;

void comp_and_swap(long long *pointer, long long value) {
  int compVal, sumExchVal;
  compVal = *pointer;
  sumExchVal = compVal + value;
  while (__sync_val_compare_and_swap(pointer, compVal, sumExchVal) != compVal) {
    compVal = *pointer;
    sumExchVal = compVal + value;
    if (opt_yield)
      pthread_yield();
  }
}

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    pthread_yield();
  *pointer = sum;
}

void* threadFunc(void* ct) {
  
  // Add 1 to the counter
  for (int i = 0; i < iter_num; i++) {
    switch (sync_flag) {
    case 'm':
      if (pthread_mutex_lock(&mutex)) {
        fprintf(stderr, "pthread_mutex_lock error!\n");
        exit(EXIT_FAILURE);
      }
      add((long long *) ct, 1);
      if (pthread_mutex_unlock(&mutex)) {
        fprintf(stderr, "pthread_mutex_unlock error!\n");
        exit(EXIT_FAILURE);
      }
      break;
    case 's':
      while(__sync_lock_test_and_set(&sync_lock, 1));
      add((long long *) ct, 1);
      __sync_lock_release(&sync_lock);
      break;
    case 'c':
      comp_and_swap((long long *) ct, 1);
      break;
    default:
      add((long long *) ct, 1);
      break;
    }
  }

  // Add -1 to the counter
  for (int i = 0; i < iter_num; i++) {
    switch (sync_flag) {
    case 'm':
      if (pthread_mutex_lock(&mutex)) {
        fprintf(stderr, "pthread_mutex_lock error!\n");
        exit(EXIT_FAILURE);
      }
      add((long long *) ct, -1);
      if (pthread_mutex_unlock(&mutex)) {
        fprintf(stderr, "pthread_mutex_unlock error!\n");
        exit(EXIT_FAILURE);
      }
      break;
    case 's':
      while(__sync_lock_test_and_set(&sync_lock, 1));
      add((long long *) ct, -1);
      __sync_lock_release(&sync_lock);
      break;
    case 'c':
      comp_and_swap((long long *) ct, -1);
      break;
    default:
      add((long long *) ct, -1);
      break;
    }
  }
  return NULL;
}

int main(int argc, char** argv) {
  long long cntr = 0;
  int thread_num = 0;
  struct timespec start_time, end_time;
  char *test_name = "add-none";

  /* getopt_long usage example from the Linux Programmer's Manual */
  while(1) {
    int opt_index = 0;
    static struct option l_opt[] = {
      {"threads", required_argument, 0, 't'},
      {"iterations", required_argument, 0, 'i'},
      {"yield", no_argument, 0, 'y'},
      {"sync", required_argument, 0, 's'}
    };

    int s_opt = getopt_long(argc, argv, "yt:i:s:", l_opt, &opt_index);
    if (s_opt == -1)
      break;
    switch (s_opt) {
    case 't':
      thread_num = atoi(optarg);
      if (thread_num <= 0) {
	fprintf(stderr, "Invalid number of threads!\n");
	exit(EXIT_FAILURE);
      }
      break;
    case 'i':
      iter_num = atoi(optarg);
      if (iter_num <= 0) {
	fprintf(stderr, "Invalid number of iterations!\n");
	exit(EXIT_FAILURE);
      }
      break;
    case 'y':
      opt_yield = 1;
      break;
    case 's':
      if (strlen(optarg) == 1 && (optarg[0] == 'm' || optarg[0] == 's' || optarg[0] == 'c'))
	sync_flag = optarg[0];
      else {
	fprintf(stderr, "Use only the options 'm', 's', and 'c' for the --sync option!\n");
	exit(EXIT_FAILURE); 
      }
      break;
    default:
      exit(EXIT_FAILURE);
      break;
    }
  }

  if (opt_yield)
    test_name = "add-yield-none";
  
  if (sync_flag == 'm') {
    test_name = "add-m";
    if (opt_yield)
      test_name = "add-yield-m";
  }
  else if (sync_flag == 's') {
    test_name = "add-s";
    if (opt_yield)
      test_name = "add-yield-s";
  }
  else if (sync_flag == 'c') {
    test_name = "add-c";
    if (opt_yield)
      test_name = "add-yield-c";
  }

  pthread_t *thr_addr = malloc(sizeof(pthread_t) * thread_num);
  if (sync_flag == 'm')
    pthread_mutex_init(&mutex, NULL);

  // Collect start time
  if (clock_gettime(CLOCK_MONOTONIC, &start_time) < 0) {
    perror("clock_gettime function error!");
    exit(EXIT_FAILURE);
  }

  // Start threads
  for (int i = 0; i < thread_num; i++) {
    int ret = pthread_create(&thr_addr[i], NULL, threadFunc, &cntr);
    if (ret) {
      fprintf(stderr, "Thread creation error!\n");
      exit(EXIT_FAILURE);
    }
  }

  // Wait for all threads to exit
  for (int i = 0; i < thread_num; i++) {
    int ret = pthread_join(thr_addr[i], NULL);
    if (ret) {
      fprintf(stderr, "Thread joining error!\n");
      exit(EXIT_FAILURE);
    }
  }

  // Collect end time
  if (clock_gettime(CLOCK_MONOTONIC, &end_time) < 0) {
    perror("clock_gettime function error!");
    exit(EXIT_FAILURE);
  }

  if (sync_flag == 'm') {
    if (pthread_mutex_destroy(&mutex)) {
      fprintf(stderr, "Error destructing mutex!\n");
      exit(EXIT_FAILURE);
    }
  }

  // Calculate the elapsed time, total number of operations,
  // and average time per operation
  long long elapsed_time_in_ns = ((end_time.tv_sec - start_time.tv_sec) * 1000000000) +
    (end_time.tv_nsec - start_time.tv_nsec);
  int total_num_oper = thread_num * iter_num * 2;
  long long avg_time_per_oper = elapsed_time_in_ns / total_num_oper;
  
  // Print the CSV record to stdout
  printf("%s,%d,%d,%d,%lld,%lld,%lld\n",
	 test_name, thread_num, iter_num, total_num_oper, elapsed_time_in_ns,
	 avg_time_per_oper, cntr);
}
