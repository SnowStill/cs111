//Name: Qinglin Zhang
//Email: qqinglin0327@gmail.com
//ID: 205356739

#include "SortedList.h"
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

int num_threads=1;
int num_elements = 0;
int num_iterations = 1;
int opt_yield = 0;
int opt_sync = 0;
pthread_mutex_t mutex;
int locktype = 0;
char* name_tag = "add-none";
int spin_lock = 0;
SortedList_t* list;
SortedListElement_t* elements;

void signal_handler(int sigNum){
  if(sigNum == SIGSEGV){
    fprintf(stderr,"segfault");
    exit(2);
  }
}

void get_name(){
  switch(opt_yield){
  case 0:
    if(locktype == 0)
      name_tag = "list-none-none";
    else if(locktype == 1)
      name_tag = "list-none-m";
    else if(locktype == 2)
      name_tag = "list-none-s";
    break;
  case 1:
    if(locktype == 0)
      name_tag = "list-i-none";
    else if(locktype == 1)
      name_tag = "list-i-m";
    else if(locktype == 2)
      name_tag = "list-i-s";
    break;
  case 2:
    if(locktype == 0)
      name_tag = "list-d-none";
    else if(locktype == 1)
      name_tag = "list-d-m";
    else if(locktype == 2)
      name_tag = "list-d-s";
    break;
  case 3:
    if(locktype == 0)
      name_tag = "list-id-none";
    else if(locktype == 1)
      name_tag = "list-id-m";
    else if(locktype == 2)
      name_tag = "list-id-s";
    break;
  case 4:
    if(locktype == 0)
      name_tag = "list-l-none";
    else if(locktype == 1)
      name_tag = "list-l-m";
    else if(locktype == 2)
      name_tag = "list-l-s";
    break;
  case 5:
    if(locktype == 0)
      name_tag = "list-il-none";
    else if(locktype == 1)
      name_tag = "list-il-m";
    else if(locktype == 2)
      name_tag = "list-il-s";
    break;
  case 6:
    if(locktype == 0)
      name_tag = "list-dl-none";
    else if(locktype == 1)
      name_tag = "list-dl-m";
    else if(locktype == 2)
      name_tag = "list-dl-s";
    break;
  case 7:
    if(locktype == 0)
      name_tag = "list-idl-none";
    else if(locktype == 1)
      name_tag = "list-idl-m";
    else if(locktype == 2)
      name_tag = "list-idl-s";
  }
}

void* task(void* ptr){
  SortedListElement_t* subarray = ptr;
  int i;
  //insert
  for(i=0;i < num_iterations;i++){
    //default
    if(locktype == 0){
      SortedList_insert(list, (SortedListElement_t *)subarray+i);
    }
    //mutex
    else if(locktype == 1){
      if(pthread_mutex_lock(&mutex) != 0){
        fprintf(stderr, "Failed to lock: %s \n", strerror(errno));
	exit(1);
      }
      SortedList_insert(list, (SortedListElement_t *)subarray+i);
      if(pthread_mutex_unlock(&mutex) != 0){
        fprintf(stderr, "Failed to unlock: %s \n", strerror(errno));
	exit(1);
      }
    }
    //spin
    else if(locktype ==2){
      while (__sync_lock_test_and_set(&spin_lock, 1));
      SortedList_insert(list, (SortedListElement_t *)subarray+i);
      __sync_lock_release(&spin_lock);
    }
  }
  
  //get length
  int length = 0;
  if(locktype == 0){
    length = SortedList_length(list);
    if (length == -1){
      fprintf(stderr, "Invalid length.\n");
      exit(2);
    }
  }
  else if(locktype == 1){
    if(pthread_mutex_lock(&mutex) != 0){
      fprintf(stderr, "Failed to lock: %s \n", strerror(errno));
      exit(1);
    }
    length = SortedList_length(list);
    if (length == -1){
      fprintf(stderr, "Invalid length.\n");
      exit(2);
    }
    if(pthread_mutex_unlock(&mutex) != 0){
      fprintf(stderr, "Failed to unlock: %s \n", strerror(errno));
      exit(1);
    }
  }
  else if(locktype ==2){
    while (__sync_lock_test_and_set(&spin_lock, 1));
    length = SortedList_length(list);
    if (length == -1){
      fprintf(stderr, "Invalid length.\n");
      exit(2);
    }
    __sync_lock_release(&spin_lock);
  }
  //delete
  SortedListElement_t *temp;
  for(i=0;i < num_iterations;i++){
    //default
    if(locktype == 0){
      temp = SortedList_lookup(list, (subarray+i)->key);
      if(temp == NULL){
	fprintf(stderr, "Failed to look up element. \n");
	exit(2);
      };
      if(SortedList_delete(temp)!=0){
	fprintf(stderr, "Failed to delete element. \n");
	exit(2);
      }
    }
    //mutex
    else if(locktype == 1){
      if(pthread_mutex_lock(&mutex) != 0){
        fprintf(stderr, "Failed to lock: %s \n", strerror(errno));
        exit(1);
      }
      temp = SortedList_lookup(list, (subarray+i)->key);
      if(temp == NULL){
        fprintf(stderr, "Failed to look up element. \n");
        exit(2);
      };
      if(SortedList_delete(temp)!=0){
        fprintf(stderr, "Failed to delete element. \n");
        exit(2);
      }
      if(pthread_mutex_unlock(&mutex) != 0){
        fprintf(stderr, "Failed to lock: %s \n", strerror(errno));
        exit(1);
      }
    }
    //spin
    else if(locktype ==2){
      while (__sync_lock_test_and_set(&spin_lock, 1));
      temp = SortedList_lookup(list, (subarray+i)->key);
      if(temp == NULL){
        fprintf(stderr, "Failed to look up element. \n");
        exit(2);
      };
      if(SortedList_delete(temp)!=0){
        fprintf(stderr, "Failed to delete element. \n");
        exit(2);
      }
      __sync_lock_release(&spin_lock);
    }
  }
  return NULL;
}


int main(int argc, char* argv[]) {
  struct option options[] = {
    {"threads",1, NULL, 't'},
    {"iterations",1, NULL, 'i'},
    {"yield",1, NULL, 'y'},
    {"sync",1, NULL, 's'},
    {0, 0, 0, 0}
  };
  int opt;
  while(1){
    opt = getopt_long(argc, argv, "", options, 0);
    if(opt == -1)
      break;
    else if (opt == '?'){

      fprintf(stderr, "Arguments Error\n");
      exit(1); //1 ... unrecognized argument
    }
    else if (opt == 't'){
      num_threads = atoi(optarg);
    }
    else if(opt == 'i'){
      num_iterations = atoi(optarg);
    }
    else if(opt == 'y'){
      unsigned int i;
      for(i = 0; i < strlen(optarg); i++)
	if (optarg[i] == 'i') {
	  opt_yield |= INSERT_YIELD;
	} 
	else if (optarg[i] == 'd') {
	  opt_yield |= DELETE_YIELD;
	} 
	else if (optarg[i] == 'l') {
	  opt_yield |= LOOKUP_YIELD;
	}
	else{
	  fprintf(stderr, "Wrong yield argument.\n");
	}
    }
    else if(opt == 's'){
      switch (optarg[0]){
      case 'm':
        locktype = 1; //mutex
        if (pthread_mutex_init(&mutex, NULL) != 0){
          fprintf(stderr, "Failed to create mutex: %s\n", strerror(errno));
          exit(1);
        }
        break;
      case 's':
	locktype = 2; //spin lock
        break;
      default:
        fprintf(stderr, "Invalid sync argument.\n");
        exit(1);
      }
    }
    else{
      fprintf(stderr, "Arguments Error for sync.\n");
      exit(1);
    }
  }
  signal(SIGSEGV, signal_handler);
  //initialize an empty list with a dummy head
  list = malloc(sizeof(SortedList_t));
  list->prev = NULL;
  list->next = NULL;
  list->key =NULL;
  //create and initialize elements
  num_elements = num_threads * num_iterations;
  elements = malloc(sizeof(SortedListElement_t)*num_elements);

  //create random keys
  int i;
  srand(time(NULL));
  for(i=0; i < num_elements; i++){
    char* key = malloc(sizeof(char)*5);
    int j;
    for(j = 0; j < 5; j++){
      key[j] = (char)rand()%26 +97;//all valid ASCII characters 
    }
    elements[i].key = key;
    //    fprintf(stderr, "%s", elements[i].key);
  }

  //get the start time
  struct timespec start_time;
  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  //create threads
  pthread_t threadslist[num_threads];

  for (i = 0; i < num_threads; i++)
    {
      if(pthread_create(&threadslist[i], NULL, task, (void*)(elements + num_iterations*i))!= 0){
        fprintf(stderr, "Failed to creat threads: %s\n", strerror(errno));

      }
    }
  //wait threads to join back
  for (i = 0; i < num_threads; i++)
    {
      if (pthread_join(threadslist[i], NULL) != 0)
        fprintf(stderr, "Failed to join threads: %s\n", strerror(errno));
    }
  //get the end time
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  
  //get the number of operations
  long long num_operations = num_iterations * num_threads * 3;
  //get the time
  long long runtime = (end_time.tv_sec - start_time.tv_sec) * 1000000000 +
    (end_time.tv_nsec - start_time.tv_nsec);
  long long time_per_operation = runtime/num_operations;
  //get the name
  get_name();
  long num_lists = 1;
  //print
  printf("%s,%d,%d,%ld,%lld,%lld,%lld\n", name_tag, num_threads, num_iterations, num_lists, num_operations, runtime, time_per_operation);
  long length = SortedList_length(list);
  //  printf("%ld", length);
  if ( length!= 0)
    exit(2);
  
  for(i = 0; i < num_elements; i++){
    free((void*)elements[i].key);
  }
  free(elements);
  free(list);
  exit(0);
}
