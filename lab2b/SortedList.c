//Name: Qinglin Zhang
//Email: qqinglin0327@gmail.com
//ID: 205356739

#include "SortedList.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  // fprintf(stderr,"1");
  if(list == NULL)return;
  if(element == NULL) return;
  //fprintf(stderr,"2");
  SortedListElement_t* curr = list -> next;
  SortedListElement_t* prev = list;
  if(curr == NULL){
    if (opt_yield & INSERT_YIELD){
      sched_yield();
    }
    //fprintf(stderr,"3");
    list->next = element;
    //fprintf(stderr,"4");
    element->prev = list;
    //fprintf(stderr,"%s", element->key);
    element->next = NULL;
    return;
  }
  while(curr != NULL){
    if(strcmp(curr->key, element->key) >= 0)
      break;
    prev = curr;
    curr = curr->next;
  }
  if (opt_yield & INSERT_YIELD) {
    sched_yield();
  }
  if (curr != NULL) {
    prev->next = element;
    curr->prev = element;
    element->next = curr;
    element->prev = prev;
    return;
  }
  prev->next = element;
  element->next = NULL;
  element->prev = prev;
  return;
}
int SortedList_delete(SortedListElement_t *element) {
  if (element == NULL)
    return 1;
  if (opt_yield & DELETE_YIELD)
    sched_yield();
  if (element->prev != NULL) {
    if (element->prev->next == element)
      element->prev->next = element->next;
    else 
      return 1;
  }
  if (element->next != NULL) {
    if (element->next->prev == element)
      element->next->prev = element->prev;
    else 
      return 1;
  }    
  //fprintf(stderr,"9");
  return 0;
}
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  if(list ==NULL){
    return NULL;
  }

  //fprintf(stderr,"7");
  SortedListElement_t* iterator = list->next;
  if (opt_yield & LOOKUP_YIELD) {
    sched_yield();
  }
  while(iterator != NULL){
    if (strcmp(key, iterator->key) == 0) {
      //  fprintf(stderr,"8");
      return iterator;
    }
    iterator = iterator->next;
  }
  return NULL;
}
int SortedList_length(SortedList_t *list){
  if (list == NULL) {
    return -1;
  }
  SortedListElement_t* iterator = list->next;
  int count = 0;
  if (opt_yield & LOOKUP_YIELD) {
    sched_yield();
  }
  while(iterator != NULL){
    count++;
    iterator = iterator->next;
  }
  //fprintf(stderr,"6");
  return count;
}
