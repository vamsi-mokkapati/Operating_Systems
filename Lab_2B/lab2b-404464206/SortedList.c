#define _GNU_SOURCE
#include "SortedList.h"
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  SortedListElement_t *traversing_node = list->next;
  if (element == NULL || list == NULL) {
    fprintf(stderr, "There has to be a list and an element to insert!\n");
    exit(EXIT_FAILURE);
  }

  //Traverse through all the nodes till arriving at first node element is less than or equal to.
  while (traversing_node != list) {
    if (strcmp(traversing_node->key, element->key) > 0)
      break;
    traversing_node = traversing_node->next;
  }

  if (opt_yield & INSERT_YIELD)
    pthread_yield();

  // Modify prev and next pointers to include element within list.
  traversing_node->prev->next = element;
  element->next = traversing_node;
  element->prev = traversing_node->prev;
  traversing_node->prev = element;
}

int SortedList_delete( SortedListElement_t *element) {
  if (element == NULL)
    return 1;
  if (element->prev->next == element && element->next->prev == element) {
    element->prev->next = element->next;
    element->next->prev = element->prev;
    if (opt_yield & DELETE_YIELD)
      pthread_yield();
    return 0;
  }
  else
    return 1;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  if (key == NULL || list == NULL) {
    fprintf(stderr, "There has to be a list and a key to look up!\n");
    return NULL;
  }
  SortedListElement_t *traversing_node = list->next;
  while (traversing_node != list) {
    if (!strcmp(traversing_node->key, key))
      return traversing_node;
    traversing_node = traversing_node->next;
    if (opt_yield & LOOKUP_YIELD)
      pthread_yield();
  }
  return NULL;
}

int SortedList_length(SortedList_t *list) {
  if (list == NULL) {
    fprintf(stderr, "There has to be a list to find the list length!\n");
    return -1;
  }
  SortedListElement_t *traversing_node = list->next;
  int list_length = 0;
  while (traversing_node != list) {
    list_length++;
    traversing_node = traversing_node->next;
    if (opt_yield & LOOKUP_YIELD)
      pthread_yield();
  }
  return list_length;
}
