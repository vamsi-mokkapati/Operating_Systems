#define _GNU_SOURCE
#include "SortedList.h"
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

int thread_num = 0, iter_num = 0, opt_yield = 0, sync_lock = 0;
char sync_flag = 'a';
SortedList_t *list;
SortedListElement_t *elements_array;
pthread_mutex_t mutex;
int list_length = 0;

void *threadFunc(void *arg) {
  for (int i = *(int *) arg; i < thread_num * iter_num; i += thread_num) {
    switch (sync_flag) {
    case 'm':
      if (pthread_mutex_lock(&mutex)) {
        fprintf(stderr, "pthread_mutex_lock error!\n");
        exit(EXIT_FAILURE);
      }
      SortedList_insert(list, &elements_array[i]);
      if (pthread_mutex_unlock(&mutex)) {
        fprintf(stderr, "pthread_mutex_unlock error!\n");
        exit(EXIT_FAILURE);
      }
      break;
    case 's':
      while(__sync_lock_test_and_set(&sync_lock, 1));
      SortedList_insert(list, &elements_array[i]);
      __sync_lock_release(&sync_lock);
      break;
    default:
      SortedList_insert(list, &elements_array[i]);
      break;
    }
  }
  
  SortedListElement_t *lookup_elem;
  for (int i = *(int *) arg; i < thread_num * iter_num; i += thread_num) {
    switch (sync_flag) {
    case 'm':
      if (pthread_mutex_lock(&mutex)) {
        fprintf(stderr, "pthread_mutex_lock error!\n");
        exit(EXIT_FAILURE);
      }
      lookup_elem = SortedList_lookup(list, elements_array[i].key);
      SortedList_delete(lookup_elem);
      if (pthread_mutex_unlock(&mutex)) {
        fprintf(stderr, "pthread_mutex_unlock error!\n");
        exit(EXIT_FAILURE);
      }
      break;
    case 's':
      while(__sync_lock_test_and_set(&sync_lock, 1));
      lookup_elem = SortedList_lookup(list, elements_array[i].key);
      SortedList_delete(lookup_elem);
      __sync_lock_release(&sync_lock);
      break;
    default:
      lookup_elem = SortedList_lookup(list, elements_array[i].key);
      SortedList_delete(lookup_elem);
      break;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  struct timespec start_time, end_time;
  char test_name[20] = "list-";
  char* y_opts = "none";
  char* dash = "-";
  char* s_opts = "none";

  /* getopt_long usage example from the Linux Programmer's Manual */
  while (1) {
    int opt_index = 0;
    static struct option l_opt[] = {
      {"threads", required_argument, 0, 't'},
      {"iterations", required_argument, 0, 'i'},
      {"yield", required_argument, 0, 'y'},
      {"sync", required_argument, 0, 's'}
    };

    int s_opt = getopt_long(argc, argv, "y:t:i:s:", l_opt, &opt_index);
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
      if (strlen(optarg) > 3) {
	fprintf(stderr, "Invalid yield option! Only a maximum of 3 characters allowed. \n");
	exit(EXIT_FAILURE);
      }
      for (int i = 0; optarg[i] != '\0'; i++) {
	switch (optarg[i]) {
	case 'i':
	  opt_yield |= INSERT_YIELD;
	  break;
	case 'd':
	  opt_yield |= DELETE_YIELD;
	  break;
	case 'l':
	  opt_yield |= LOOKUP_YIELD;
	  break;
	default:
	  fprintf(stderr, "Invalid yield option! Only 'i', 'd', and 'l' are valid options.\n");
          exit(EXIT_FAILURE);
	  break;
	}
      }
      y_opts = optarg;
      break;
    case 's':
      if (strlen(optarg) == 1 && (optarg[0] == 'm' || optarg[0] == 's')) {
        sync_flag = optarg[0];
	if (optarg[0] == 'm')
	  s_opts = "m";
	else
	  s_opts = "s";
      }
      else {
        fprintf(stderr, "Use only the options 'm' or 's' for the --sync option!\n");
        exit(EXIT_FAILURE);
      }
      break;
    default:
      exit(EXIT_FAILURE);
      break;
    }
  }

  strcat(test_name, y_opts);
  strcat(test_name, dash);
  strcat(test_name, s_opts);
  
  // Doubly linked list initialization
  list = malloc(sizeof(SortedList_t));
  list->key = NULL;
  list->next = list;
  list->prev = list;
  elements_array = malloc(sizeof(SortedListElement_t) * iter_num * thread_num);

  // Initialize elements_array with random keys
  srand(time(NULL)); // Randomize the seed
  for (int p = 0; p < iter_num * thread_num; p++) {
    int key_size = (rand() % 24) + 7; // Key are of random length, minimum size 7 and maximum size 30
    char *elem_key = malloc(sizeof(char) * (key_size+1)); //key_size+1 is allocated because of '\0' at the end
    for (int q = 0; q < key_size; q++)
      elem_key[q] = (rand() % 10); // Each character of key will be a random digit from 0 to 9
    elem_key[key_size] = '\0';
    elements_array[p].key = elem_key;
  }

  pthread_t *thr_addr = malloc(sizeof(pthread_t) * thread_num);
  int *thr_id = malloc(sizeof(int) * thread_num);
  if (sync_flag == 'm')
    pthread_mutex_init(&mutex, NULL);

  // Collect start time
  if (clock_gettime(CLOCK_MONOTONIC, &start_time) < 0) {
    perror("clock_gettime function error!");
    exit(EXIT_FAILURE);
  }

  // Start threads
  for (int i = 0; i < thread_num; i++) {
    thr_id[i] = i;
    int ret = pthread_create(&thr_addr[i], NULL, threadFunc, &thr_id[i]);
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
      fprintf(stderr, "Error destroying mutex!\n");
      exit(EXIT_FAILURE);
    }
  }

  // Check if list is corrupted
  list_length = SortedList_length(list);
  if (list_length != 0) {
    fprintf(stderr, "The list is corrupted!\n");
    exit(EXIT_FAILURE);
  }

  // Calculate the elapsed time, total number of operations,
  // and average time per operation
  long long elapsed_time_in_ns = ((end_time.tv_sec - start_time.tv_sec) * 1000000000) +
    (end_time.tv_nsec - start_time.tv_nsec);
  int total_num_oper = thread_num * iter_num * 3;
  long long avg_time_per_oper = elapsed_time_in_ns / total_num_oper;

  // Print the CSV record to stdout
  printf("%s,%d,%d,1,%d,%lld,%lld\n",
	 test_name, thread_num, iter_num, total_num_oper, elapsed_time_in_ns,
	 avg_time_per_oper);
}
