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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int period = 1;
FILE* output_file = NULL;
int running = 1;
int stopped = 0;
char scale = 'F';
mraa_aio_context aio_dev;
#define B 4275
#define R0 100000.0
int log_flag = 0;
int id_flag = 0;
int host_flag = 0;
int firsttime = 1;
int id;
char* hostname;
int port;
struct sockaddr_in serv_addr;
struct hostent *host;
int sockfd;
SSL* ssl;

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

void print_current_time_and_temp(){
  float temp = get_temperature();
  struct timespec ts;
  struct tm* tm;
  char buff[60];
  int ret;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = localtime(&(ts.tv_sec));
  sprintf(buff, "%02d:%02d:%02d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temp);
  ret = SSL_write(ssl,buff,strlen(buff));
  if (ret <= 0){
    fprintf(stderr,"Error: Failed to write report to server: %s\n",  strerror(errno));
    exit(1);
  }
  if(log_flag == 1){
    fprintf(output_file, "%02d:%02d:%02d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temp);
    fflush(output_file);
  }
}
void TCP_connect(){
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    fprintf(stderr, "Error: Failed to open socket: %s\n", strerror(errno));
    mraa_aio_close(aio_dev);
    exit(1);
  }
  host = gethostbyname(hostname);
  if(host == NULL){
    fprintf(stderr, "Error: Failed to get the host server: %s\n", strerror(errno));
    mraa_aio_close(aio_dev);
    exit(1);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)host->h_addr,
	(char *)&serv_addr.sin_addr.s_addr,
	host->h_length);
  serv_addr.sin_port = htons(port);
  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
    fprintf(stderr, "ERROR connecting: %s", strerror(errno));
    mraa_aio_close(aio_dev);
    exit(1);
  }
}
void SSL_init(){
  SSL_CTX * newContext = NULL;
  if(SSL_library_init() < 0){
    fprintf(stderr, "SSL INIT ERROR: %s", strerror(errno));
    exit(1);
  }
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
  newContext = SSL_CTX_new(TLSv1_client_method());
  if (newContext == NULL){
    fprintf(stderr, "SSL CREATING NEW ERROR: %s", strerror(errno));
    exit(1);
  }
  ssl = SSL_new(newContext);
  if(SSL_set_fd(ssl, sockfd) == 0){
    fprintf(stderr, "SSL SETTING UP FD ERROR: %s", strerror(errno));
    exit(1);
  }
  if(SSL_connect(ssl) < 0){
    fprintf(stderr, "SSL CONNECCTION ERROR: %s", strerror(errno));
    exit(1);
  }
}

void off(){
  struct timespec ts;
  struct tm* tm;
  char buff[128];
  int ret;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = localtime(&(ts.tv_sec));
  sprintf(buff, "%02d:%02d:%02d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
  ret = SSL_write(ssl,buff,strlen(buff));
  if (ret <= 0){
    fprintf(stderr,"Error: Failed to write report to server: %s\n", strerror(errno));
    exit(1);
  }
  if(log_flag == 1){
    printf("1");
    fprintf(output_file, "%02d:%02d:%02d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
    printf("2");
    fflush(output_file);
  }
  mraa_aio_close(aio_dev);
  exit(0);
}


int main(int argc, char** argv){
  struct option options[] = {
    {"id",1, NULL, 'i'},
    {"host",1, NULL, 'h'},
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
    else if (opt == 'i'){
      if(strlen(optarg) != 9){
	fprintf(stderr, "Error: Invalid ID Length\n");
	exit(1);
      }
      id_flag = 1;
      id = atoi(optarg);
    }
    else if (opt == 'h'){
      host_flag = 1;
      hostname = optarg;
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
  if (!host_flag)
    {
      fprintf(stderr, "Error: no host name provided\n");
      exit(1);
    }
  //get port number
  port = atoi(argv[argc - 1]);
  if(port <= 0){
    fprintf(stderr, "Invalid port number\n");
    exit(1);
  }
  
  //initialize
  aio_dev = mraa_aio_init(0);
  //setup connection
  TCP_connect();
  //ssl ini
  SSL_init();
  char buff[60];
  int ret;
  sprintf(buff, "ID=%d\n", id);
  ret = SSL_write(ssl,buff,strlen(buff));
  if (ret <= 0){
    fprintf(stderr,"Error: Failed to write report to server: %s\n",  strerror(errno));
    exit(1);
  }
  if(log_flag)
    fprintf(output_file, "ID=%d\n", id);
  
  //init pollers
  struct pollfd pfd;
  pfd.fd = sockfd;
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
      print_current_time_and_temp();
      prevsecond = (time.tv_sec + period);
      if(firsttime){
      	firsttime = 0;
      }      
    }
    //create pollers
    int stat = poll(&pfd,1,0);
    if(stat < 0){
      fprintf(stderr, "Error: failed to create the poll.");
      mraa_aio_close(aio_dev);
      exit(1);
    }
    
    if(pfd.revents & POLLIN){
      int read;
      char input[128];
      char temp[128];
      read = SSL_read(ssl, input, 128);
      if (read <= 0){
	fprintf(stderr,"Error: Failed to write report to server: %s\n",  strerror(errno));
	exit(1);
      }
      //fprintf(stderr,"%s", input);
      //int len = strlen(input);
      int i;
      for(i = 0; i < read; i++){
	if(input[i] == '\n'){
	  temp[i]='\0';
	  if (strcmp(temp, "SCALE=F") == 0)
	    scale = 'F';
	  else if (strcmp(temp, "SCALE=C") == 0)
	    scale = 'C';
	  else if (strcmp(temp, "STOP") == 0)
	    stopped = 1;
	  else if (strcmp(temp, "START") == 0)
	    stopped = 0;
	  else if (strstr(temp, "LOG")!= NULL)
	    ;	
	  else if (strcmp("OFF", temp) == 0){
	    if(log_flag){
	      fprintf(output_file, "OFF\n");
	      fflush(output_file);
	    }
	    off();
	  }
	  else if (strstr(temp, "PERIOD=")!=NULL){
	    period =atoi(&temp[7]);
	  }
	  break;
	}
	else{
	  temp[i]=input[i];
	}
      }
      if(log_flag){
	fprintf(output_file,"%s\n", temp);
	fflush(output_file);
 
      }
    }				
    
  }	

  mraa_aio_close(aio_dev);

  return 0;
}
