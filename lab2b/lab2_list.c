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
pthread_mutex_t* mutexes;
int locktype = 0;
char* name_tag = "add-none";
int* spin_locks;
SortedList_t* lists;
SortedListElement_t* elements;
long long lock_time;
int list_num = 1;
int* hash;
int hashfunction(const char* key) {
  return (key[0] % list_num);
}

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

void* task(void* id){
  int i = *((int*)id)*num_iterations;
  struct timespec lock_start;
  struct timespec lock_end;
  //insert
  for(;i < num_iterations*(*((int*)id)+1);i++){
    //default
    if(locktype == 0){
      SortedList_insert(&lists[hash[i]], &elements[i]);
    }
    //mutex
    else if(locktype == 1){
      clock_gettime(CLOCK_MONOTONIC, &lock_start);
      if(pthread_mutex_lock(&mutexes[hash[i]]) != 0){
        fprintf(stderr, "Failed to lock: %s \n", strerror(errno));
	exit(1);
      }
      clock_gettime(CLOCK_MONOTONIC, &lock_end);
      lock_time += (lock_end.tv_sec - lock_start.tv_sec) * 1000000000 +
	(lock_end.tv_nsec - lock_start.tv_nsec);
      SortedList_insert(&lists[hash[i]], &elements[i]);
      if(pthread_mutex_unlock(&mutexes[hash[i]]) != 0){
        fprintf(stderr, "Failed to unlock: %s \n", strerror(errno));
	exit(1);
      }
    }
    //spin
    else if(locktype ==2){
      clock_gettime(CLOCK_MONOTONIC, &lock_start);
      while (__sync_lock_test_and_set(&spin_locks[hash[i]], 1));
      clock_gettime(CLOCK_MONOTONIC, &lock_end);
      lock_time += (lock_end.tv_sec - lock_start.tv_sec) * 1000000000 +
        (lock_end.tv_nsec - lock_start.tv_nsec);
      SortedList_insert(&lists[hash[i]], &elements[i]);
      __sync_lock_release(&spin_locks[hash[i]]);
    }
  }
  
  //get length
  int length = 0;
  int j;
  if(locktype == 0){
    for(j =0; j < list_num;j++){
      length += SortedList_length(&lists[j]);
      if (length == -1){
	fprintf(stderr, "Invalid length.\n");
	exit(2);
      }
    }
  }
  else if(locktype == 1){
    for(j =0; j < list_num;j++){
      clock_gettime(CLOCK_MONOTONIC, &lock_start);
      if(pthread_mutex_lock(&mutexes[j]) != 0){
	fprintf(stderr, "Failed to lock: %s \n", strerror(errno));
	exit(1);
      }
      clock_gettime(CLOCK_MONOTONIC, &lock_end);
      lock_time += (lock_end.tv_sec - lock_start.tv_sec) * 1000000000 +
	(lock_end.tv_nsec - lock_start.tv_nsec);
      length += SortedList_length(&lists[j]);
      if (length == -1){
	fprintf(stderr, "Invalid length.\n");
	exit(2);
      }
      if(pthread_mutex_unlock(&mutexes[j]) != 0){
	fprintf(stderr, "Failed to unlock: %s \n", strerror(errno));
	exit(1);
    }
    }
  }
  else if(locktype ==2){
    for(j =0; j < list_num;j++){
      clock_gettime(CLOCK_MONOTONIC, &lock_start);
      while (__sync_lock_test_and_set(&spin_locks[j], 1));
      clock_gettime(CLOCK_MONOTONIC, &lock_end);
      lock_time += (lock_end.tv_sec - lock_start.tv_sec) * 1000000000 +
	(lock_end.tv_nsec - lock_start.tv_nsec);
      length += SortedList_length(&lists[j]);
      if (length == -1){
	fprintf(stderr, "Invalid length.\n");
	exit(2);
      }
      __sync_lock_release(&spin_locks[j]);
    }
  }
  //delete
  SortedListElement_t *temp;
  i = *((int*)id)*num_iterations;
  for(;i < num_iterations*(*((int*)id)+1);i++){
    //default
    if(locktype == 0){
      temp = SortedList_lookup(&lists[hash[i]], elements[i].key);
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
      clock_gettime(CLOCK_MONOTONIC, &lock_start);
      if(pthread_mutex_lock(&mutexes[hash[i]]) != 0){
        fprintf(stderr, "Failed to lock: %s \n", strerror(errno));
        exit(1);
      }
      clock_gettime(CLOCK_MONOTONIC, &lock_end);
      lock_time += (lock_end.tv_sec - lock_start.tv_sec) * 1000000000 +
        (lock_end.tv_nsec - lock_start.tv_nsec);
      temp = SortedList_lookup(&lists[hash[i]], elements[i].key);
      if(temp == NULL){
        fprintf(stderr, "Failed to look up element. \n");
        exit(2);
      };
      if(SortedList_delete(temp)!=0){
	fprintf(stderr, "Failed to delete element. \n");
	exit(2);
      }
      if(pthread_mutex_unlock(&mutexes[hash[i]]) != 0){
	fprintf(stderr, "Failed to lock: %s \n", strerror(errno)); 
	exit(1);
      }
    }//spin
    else if(locktype ==2){ 
      clock_gettime(CLOCK_MONOTONIC, &lock_start);
      while (__sync_lock_test_and_set(&spin_locks[hash[i]], 1));
      clock_gettime(CLOCK_MONOTONIC, &lock_end); 
      lock_time += (lock_end.tv_sec - lock_start.tv_sec) * 1000000000 + 
	(lock_end.tv_nsec - lock_start.tv_nsec);
      temp = SortedList_lookup(&lists[hash[i]], elements[i].key);
      if(temp == NULL){
        fprintf(stderr, "Failed to look up element. \n");
        exit(2);
      };
      if(SortedList_delete(temp)!=0){
        fprintf(stderr, "Failed to delete element. \n");
        exit(2);
      }
      __sync_lock_release(&spin_locks[hash[i]]);
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
    {"lists",1, NULL, 'l'},
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
        break;
      case 's':
	locktype = 2; //spin lock
        break;
      default:
        fprintf(stderr, "Invalid sync argument.\n");
        exit(1);
      }
    }
    else if(opt == 'l'){
      list_num = atoi(optarg);
    }
    else{
      fprintf(stderr, "Arguments Error for sync.\n");
      exit(1);
    }
  }
  signal(SIGSEGV, signal_handler);
  int i;
  //initialize locks
  if(locktype == 1){
    mutexes = malloc(list_num*sizeof(pthread_mutex_t));
    for (i = 0; i < list_num; i++)
      pthread_mutex_init(&mutexes[i], NULL);
  }
  else if (locktype == 2){
    spin_locks = malloc(list_num * sizeof(int));
    for (i = 0; i < list_num; i++)
      spin_locks[i] = 0;
  }

  //initialize  empty lists with a dummy head for each
  lists = malloc(list_num*sizeof(SortedList_t));
  for(i = 0; i < list_num;i++){
    lists[i].prev = NULL;
    lists[i].next = NULL;
    lists[i].key = NULL;
  }
  //create and initialize elements
  num_elements = num_threads * num_iterations;
  elements = malloc(sizeof(SortedListElement_t)*num_elements);

  srand(time(NULL));
  hash = malloc(sizeof(int)* num_elements);//hash table
  //create random keys
  for(i=0; i < num_elements; i++){
    char* key = malloc(sizeof(char)*5);
    int j;
    for(j = 0; j < 5; j++){
      key[j] = (char)rand()%26 +97;//all valid alphabetic characters 
    }
    elements[i].key = key;
    hash[i] = hashfunction(elements[i].key);//distribute elements by using the hash function
  }


  //get the start time
  struct timespec start_time;
  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  //create threads
  pthread_t threadslist[num_threads];
  int thread_id[num_threads];
  for (i = 0; i < num_threads; i++)
    {
      thread_id[i]=i;
      if(pthread_create(&threadslist[i], NULL, task, &thread_id[i])!= 0){
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
  //print
  printf("%s,%d,%d,%d,%lld,%lld,%lld,%lld\n", name_tag, num_threads, num_iterations, list_num, num_operations, runtime, time_per_operation, lock_time/num_operations);
  long length = SortedList_length(lists);
  //  printf("%ld", length);
  if ( length!= 0)
    exit(2);
  
  for(i = 0; i < num_elements; i++){
    free((void*)elements[i].key);
  }
  free(elements);
  free(lists);
  if(locktype == 1){
    free(mutexes);
}
  if(locktype == 2){
    free(spin_locks);
  }
  exit(0);
}
