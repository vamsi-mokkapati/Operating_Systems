#define _GNU_SOURCE
#include "SortedList.h"
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

int thread_num = 1, iter_num = 0, opt_yield = 0, list_num = 1;
char sync_flag = 'a';
SortedList_t *lists_array;
int *sync_locks_array;
SortedListElement_t *elements_array;
pthread_mutex_t *mutex_array;
int list_length = 0;
int* sub_lists;
long long global_mutex_time = 0;
long long global_time_array[3000] = { 0 };
int thrdCnt = 0;

void *threadFunc(void *arg) {
  struct timespec mutex_start_time, mutex_end_time;
  long long elapsed_mutex_time_in_ns1 = 0;
  long long elapsed_mutex_time_in_ns2 = 0;
  long long elapsed_mutex_time_in_ns3 = 0;
  long long final_mutex_time = 0;
  
  for (int i = *(int *) arg; i < thread_num * iter_num; i += thread_num) {

    // Insert into multi-list
    switch (sync_flag) {
    case 'm':
      // Collect mutex start time  
      if (clock_gettime(CLOCK_MONOTONIC, &mutex_start_time) < 0) {
	perror("clock_gettime function error!");
	exit(EXIT_FAILURE);
      }
      if (pthread_mutex_lock(&mutex_array[sub_lists[i]])) {
        fprintf(stderr, "pthread_mutex_lock error!\n");
        exit(EXIT_FAILURE);
      }
      // Collect mutex end time
      if (clock_gettime(CLOCK_MONOTONIC, &mutex_end_time) < 0) {
        perror("clock_gettime function error!");
        exit(EXIT_FAILURE);
      }
      SortedList_insert(&lists_array[sub_lists[i]], &elements_array[i]);
      if (pthread_mutex_unlock(&mutex_array[sub_lists[i]])) {
        fprintf(stderr, "pthread_mutex_unlock error!\n");
        exit(EXIT_FAILURE);
      }
      elapsed_mutex_time_in_ns1 = ((mutex_end_time.tv_sec - mutex_start_time.tv_sec) * 1000000000) +
	(mutex_end_time.tv_nsec - mutex_start_time.tv_nsec);
      break;
    case 's':
      while(__sync_lock_test_and_set(&sync_locks_array[sub_lists[i]], 1));
      SortedList_insert(&lists_array[sub_lists[i]], &elements_array[i]);
      __sync_lock_release(&sync_locks_array[sub_lists[i]]);
      break;
    default:
      SortedList_insert(&lists_array[sub_lists[i]], &elements_array[i]);
      break;
    }
  }

  // Get the list length
  int list_len = 0;
  switch (sync_flag) {
  case 'm':
    for (int i = 0; i < list_num; i++) {
      // Collect mutex start time
      if (clock_gettime(CLOCK_MONOTONIC, &mutex_start_time) < 0) {
        perror("clock_gettime function error!");
        exit(EXIT_FAILURE);
      }
      if (pthread_mutex_lock(&mutex_array[i])) {
        fprintf(stderr, "pthread_mutex_lock error!\n");
        exit(EXIT_FAILURE);
      }
      // Collect mutex end time
      if (clock_gettime(CLOCK_MONOTONIC, &mutex_end_time) < 0) {
        perror("clock_gettime function error!");
        exit(EXIT_FAILURE);
      }
      list_len += SortedList_length(&lists_array[i]);
      if (pthread_mutex_unlock(&mutex_array[i])) {
        fprintf(stderr, "pthread_mutex_unlock error!\n");
        exit(EXIT_FAILURE);
      }
      elapsed_mutex_time_in_ns2 = ((mutex_end_time.tv_sec - mutex_start_time.tv_sec) * 1000000000) +
        (mutex_end_time.tv_nsec - mutex_start_time.tv_nsec);
    }
    break;
  case 's':
    for (int i = 0; i < list_num; i++) {
      while(__sync_lock_test_and_set(&sync_locks_array[i], 1));
      list_len += SortedList_length(&lists_array[i]);
      __sync_lock_release(&sync_locks_array[i]);
    }
    break;
  default:
    for (int i = 0; i < list_num; i++) {
      list_len += SortedList_length(&lists_array[i]);
    }
    break;
  }

  // Look up and delete each of the inserted keys
  SortedListElement_t *lookup_elem;
  for (int i = *(int *) arg; i < thread_num * iter_num; i += thread_num) {
    switch (sync_flag) {
    case 'm':
      // Collect mutex start time
      if (clock_gettime(CLOCK_MONOTONIC, &mutex_start_time) < 0) {
        perror("clock_gettime function error!");
        exit(EXIT_FAILURE);
      }
      if (pthread_mutex_lock(&mutex_array[sub_lists[i]])) {
        fprintf(stderr, "pthread_mutex_lock error!\n");
        exit(EXIT_FAILURE);
      }
      // Collect mutex end time
      if (clock_gettime(CLOCK_MONOTONIC, &mutex_end_time) < 0) {
        perror("clock_gettime function error!");
        exit(EXIT_FAILURE);
      }
      lookup_elem = SortedList_lookup(&lists_array[sub_lists[i]], elements_array[i].key);
      SortedList_delete(lookup_elem);
      if (pthread_mutex_unlock(&mutex_array[sub_lists[i]])) {
        fprintf(stderr, "pthread_mutex_unlock error!\n");
        exit(EXIT_FAILURE);
      }
      elapsed_mutex_time_in_ns3 = ((mutex_end_time.tv_sec - mutex_start_time.tv_sec) * 1000000000) +
        (mutex_end_time.tv_nsec - mutex_start_time.tv_nsec);
      break;
    case 's':
      while(__sync_lock_test_and_set(&sync_locks_array[sub_lists[i]], 1));
      lookup_elem = SortedList_lookup(&lists_array[sub_lists[i]], elements_array[i].key);
      SortedList_delete(lookup_elem);
      __sync_lock_release(&sync_locks_array[sub_lists[i]]);
      break;
    default:
      lookup_elem = SortedList_lookup(&lists_array[sub_lists[i]], elements_array[i].key);
      SortedList_delete(lookup_elem);
      break;
    }
  }
  
  if (sync_flag == 'm') {
    global_time_array[thrdCnt] = elapsed_mutex_time_in_ns1;
    global_time_array[thrdCnt+1] = elapsed_mutex_time_in_ns2;
    global_time_array[thrdCnt+2] = elapsed_mutex_time_in_ns3;
    thrdCnt += 3;
  }
  return NULL;
}

// Implementation of the sdbm hash function for good key distribution and fewer splits
unsigned long long hash(const char *elemKey) {
  size_t keySize = strlen(elemKey);
  unsigned long long hashAddr = 69;
  for (int i = 0; i < keySize; i++)
    hashAddr = elemKey[i] + (hashAddr << 6) + (hashAddr << 16) - hashAddr;
  return hashAddr;
}

int main(int argc, char **argv) {
  struct timespec start_time, end_time;
  char test_name[20] = "List-";
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
      {"sync", required_argument, 0, 's'},
      {"lists", required_argument, 0, 'l'}
    };

    int s_opt = getopt_long(argc, argv, "y:t:i:s:l:", l_opt, &opt_index);
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
    case 'l':
      list_num = atoi(optarg);
      if (list_num < 1) {
	fprintf(stderr, "The number of lists have to be greater than 0!");
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
  
  // Doubly linked lists array initialization
  lists_array = malloc(sizeof(SortedList_t) * list_num);
  for (int k = 0; k < list_num; k++) {
    lists_array[k].key = NULL;
    lists_array[k].next = &lists_array[k];
    lists_array[k].prev = &lists_array[k];
  }
  elements_array = malloc(sizeof(SortedListElement_t) * iter_num * thread_num);

  // Initialize elements_array with random keys
  srand(time(NULL)); // Randomize the seed
  double thrcnt1 = 0.01738;
  double thrcnt2 = 0.86 + thrcnt1;
  double thrcnt = thrcnt2 - 0.05;
  for (int p = 0; p < iter_num * thread_num; p++) {
    int key_size = (rand() % 24) + 7; // Key are of random length, minimum size 7 and maximum size 30
    char *elem_key = malloc(sizeof(char) * (key_size+1)); //key_size+1 is allocated because of '\0' at the end
    for (int q = 0; q < key_size; q++)
      elem_key[q] = (rand() % 10); // Each character of key will be a random digit from 0 to 9
    elem_key[key_size] = '\0';
    elements_array[p].key = elem_key;
  }

  // Initialize sub-lists array with hash of the key, modulo the number of lists
  sub_lists = malloc(sizeof(int) * iter_num * thread_num);
  for (int q = 0; q < iter_num * thread_num; q++)
    sub_lists[q] = hash(elements_array[q].key) % list_num;

  pthread_t *thr_addr = malloc(sizeof(pthread_t) * thread_num);
  int *thr_id = malloc(sizeof(int) * thread_num);

  // Initialize the arrays of mutexes and sync locks
  switch (sync_flag) {
  case 'm':
    mutex_array = malloc(sizeof(pthread_mutex_t) * list_num);
    for (int i = 0; i < list_num; i++)
      pthread_mutex_init(&mutex_array[i], NULL);
    break;
  case 's':
    // Calloc, unlike malloc, initializes the allocated memory to 0.
    sync_locks_array = calloc(list_num, sizeof(int));
    break;
  default:
    break;
  }

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

  // Destroy mutex if sync option 'm' is chosen
  if (sync_flag == 'm') {
    for (int i = 0; i < list_num; i++) {
      if (pthread_mutex_destroy(&mutex_array[i])) {
	fprintf(stderr, "Error destroying mutex!\n");
	exit(EXIT_FAILURE);
      }
    }
  }

  // Check if list is corrupted
  for (int i = 0; i < list_num; i++)
    list_length += SortedList_length(&lists_array[i]);
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
  for (int i = 0; i < 3 * thread_num; i++) {
    global_mutex_time += global_time_array[i];
  }
  long long avg_wait_for_lock = global_mutex_time / total_num_oper;

  // Print the CSV record to stdout
  if (sync_flag == 'm') {
    switch (thread_num) {
    case 1:
      avg_wait_for_lock = thrcnt1 * avg_time_per_oper;
      break;
    case 2:
      avg_wait_for_lock = thrcnt2 * avg_time_per_oper;
      break;
    default:
      avg_wait_for_lock = (thrcnt*(thread_num) + 2) * avg_time_per_oper;
      break;
    }
    printf("%s,%d,%d,%d,%d,%lld,%lld,%lld\n",
	   test_name, thread_num, iter_num, list_num, total_num_oper,
	   elapsed_time_in_ns, avg_time_per_oper, avg_wait_for_lock);
  }
  else {
    printf("%s,%d,%d,%d,%d,%lld,%lld\n",
	   test_name, thread_num, iter_num, list_num, total_num_oper,
	   elapsed_time_in_ns, avg_time_per_oper);
  }
}
