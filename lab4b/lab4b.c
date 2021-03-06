//NAME: Qinglin Zhang
//EMAIL: qqinglin0327@gmail.com
//ID: 205356739
#ifdef DUMMY
#define MRAA_GPIO_IN 0
#define MRAA_GPIO_EDGE_RISING 0
typedef int mraa_aio_context;
typedef int mraa_gpio_context;
typedef int mraa_result_t;
typedef int mraa_gpio_dir_t;
typedef int mraa_gpio_edge_t;

mraa_aio_context mraa_aio_init(__attribute__((unused)) int p){
  return 0;
}

mraa_aio_context mraa_gpio_init(__attribute__((unused)) int p){
  return 0;
}

int mraa_aio_read(__attribute__((unused)) mraa_aio_context c)    {
  return 650;
}
void mraa_aio_close(__attribute__((unused)) mraa_aio_context c)  {
}
void mraa_gpio_close(__attribute__((unused)) mraa_aio_context c)  {
}

mraa_result_t mraa_gpio_isr(__attribute__((unused)) mraa_gpio_context dev, __attribute__((unused)) mraa_gpio_edge_t edg, __attribute__((unused)) void* ptr, __attribute__((unused))void* a) {
  mraa_result_t MRAA_SUCCESS = 0;
  return MRAA_SUCCESS;
}


mraa_result_t mraa_gpio_dir(__attribute__((unused)) mraa_gpio_context dev, __attribute__((unused)) mraa_gpio_dir_t d) {
  mraa_result_t MRAA_SUCCESS = 0;
  return MRAA_SUCCESS;
}
#else
#include <mraa.h>
#include <mraa/aio.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <poll.h>
#include <string.h>
//#include <mraa.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

int period = 1;
FILE* output_file = NULL;
int running = 1;
int stopped = 0;
char scale = 'F';
mraa_gpio_context dpio_dev;
mraa_aio_context aio_dev;
#define B 4275
#define R0 100000.0
int log_flag = 0;
int firsttime = 1;

void print_current_time(){
  struct timespec ts;
  struct tm* tm;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = localtime(&(ts.tv_sec));
  printf("%02d:%02d:%02d ", tm->tm_hour, tm->tm_min, tm->tm_sec);
  if(log_flag == 1){
    fprintf(output_file, "%02d:%02d:%02d ", tm->tm_hour, tm->tm_min, tm->tm_sec);
    fflush(output_file);
  }
}

void off(){
  print_current_time();
  printf("SHUTDOWN\n");
  if (log_flag == 1)
    {
      fprintf(output_file, "SHUTDOWN\n");
      fflush(output_file);
      exit(0);
    }
  exit(0);
}

float get_temperature(){
  int reading = mraa_aio_read(aio_dev);
  float R = 1023.0/((float) reading) - 1.0;
  R = R0*R;
  float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
  float F = (C * 9)/5 + 32;
  if(scale == 'C'){
    return C;
  }
  else{
    return F;
  }

}

int main(int argc, char** argv){
  struct option options[] = {
    {"period",1, NULL, 'p'},
    {"scale",1, NULL, 's'},
    {"log",1, NULL, 'l'},
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
    else if (opt == 'p'){
      period = atoi(optarg);
    }
    else if(opt == 's'){
      if(optarg[0] == 'F' || optarg[0] == 'C')
	scale = optarg[0];
      else{
	fprintf(stderr, "Error: Unrecognized argument\n");
	exit(1);
      }
    }
    else if(opt == 'l'){
      log_flag = 1;
      output_file = fopen(optarg, "w");
      if(output_file == NULL){
	fprintf(stderr, "Error: Cannot open the logfile\n");
	exit(1);
      }
    }
  }
  //initialize
  dpio_dev = mraa_gpio_init(73);
  mraa_gpio_dir(dpio_dev, MRAA_GPIO_IN);
  mraa_gpio_isr(dpio_dev, MRAA_GPIO_EDGE_RISING,&off,NULL);
  aio_dev = mraa_aio_init(0);

  //init pollers
  struct pollfd pfd;
  pfd.fd = 0;
  pfd.events = POLLIN;
  //struct timespec currts;
  //struct tm* currtm;
  time_t prevsecond = 0;
  struct timespec time;
  while(1){
    //clock_gettime(CLOCK_REALTIME,&currts);
    //currtm = localtime(&(currts.tv_sec));
    clock_gettime(CLOCK_REALTIME,&time);
    if(!stopped &&( firsttime || (time.tv_sec >= prevsecond))){
      print_current_time();
      prevsecond = (time.tv_sec + period);
      float temp = get_temperature();
      printf("%.1f \n", temp);
      if(log_flag == 1){
	fprintf(output_file, "%.1f \n", temp);
	fflush(output_file);
    	}
      if(firsttime){
      	firsttime = 0;
      }      
    }
    //create pollers
    int stat = poll(&pfd,1,0);
    if(stat < 0){
      fprintf(stderr, "Error: failed to create the poll.");
    }
    
    if(pfd.revents & POLLIN){
      char* input;
      input = malloc(1024 * sizeof(char));
      fgets(input, 1024, stdin);
      //fprintf(stderr,"%s", input);
      //int len = strlen(input);
      if (strcmp(input, "SCALE=F\n") == 0)
	scale = 'F';
      else if (strcmp(input, "SCALE=C\n") == 0)
	scale = 'C';
      else if (strcmp(input, "STOP\n") == 0)
	stopped = 1;
      else if (strcmp(input, "START\n") == 0)
	stopped = 0;
      else if (strstr(input, "LOG")!= NULL)
	;	
      else if (strcmp("OFF\n", input) == 0){
	if(log_flag){
	  fprintf(output_file, "OFF\n");
	  fflush(output_file);
	}
	off();
      }
      else if (strstr(input, "PERIOD=")!=NULL){
	period =atoi(&input[7]);
      }	
      if(log_flag){
	fprintf(output_file,"%s", input);
	fflush(output_file);
      }		
    }				
			
  }	

  mraa_gpio_close(dpio_dev);
  mraa_aio_close(aio_dev);

  return 0;
}
