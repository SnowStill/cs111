//Qinglin Zhang
//qqinglin0327@gmail.com
//205356739

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <getopt.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <zlib.h>
#include <fcntl.h>

struct termios save;
int pipe2p[2];
int pipe2c[2];
int pid;
const char lf = '\n';
const char cr = '\r';
z_stream out_stream;
z_stream in_stream;
int sockfd;
void exit_status() {
  int status;
  status = waitpid(pid, &status, 0);
  if (status == -1){
    fprintf(stderr,"%s", strerror(errno));
}
  //int signal = status & 0x7f;//mask
  //int stat = status & 0xff00;
  close(sockfd);
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", WIFSIGNALED(status), WEXITSTATUS(status));
}

void end_flates(){
  deflateEnd(&out_stream);
  inflateEnd(&in_stream);
}

void restore(){
  int status = tcsetattr(0, TCSANOW, &save);
  if( status < 0) {
    fprintf(stderr, "Failed to restore the original attributes: %s", strerror(errno));
    exit(1);
  }

}

void set_terminal(){
  struct termios tattr;
 
  int status = tcgetattr(0, &save);
  if(status < 0){
    fprintf(stderr, "Failed to save the original attributes: %s\n", strerror(errno));
    exit(1);
  }
  atexit (restore);
  tcgetattr(0, &tattr);
  tattr.c_iflag = ISTRIP;
  tattr.c_oflag = 0;
  tattr.c_lflag = 0;
  status = tcsetattr(0, TCSANOW, &tattr);
  if(status < 0){
    fprintf(stderr, "Failed to set new attributes: %s\n", strerror(errno));

    exit(1);
  }
}
int main(int argc, char* argv[])
{
  //////////// define and set arguments flags////////////
  int port = 0;
  int log = 0;
  int compress = 0;
  int opt;
  //char* filename;  
  int portno;
  int logfd = -1;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  struct option options [] = {
    {"port", 1, 0, 'p'},
    {"log", 1, 0,'l'},
    {"compress", 0, 0, 'c'},
    {0, 0, 0, 0}
  };
  while(1){
    opt = getopt_long(argc, argv, "", options, 0);
    if(opt == -1)
      break;
    else if (opt == '?'){

      fprintf(stderr, "Arguments Error: --shell\n");
      exit(1); //1 ... unrecognized argument
    }
    else if (opt == 'p'){
      port = 1;
      portno = atoi(optarg);
    }
    else if (opt == 'l'){
      log = 1;
      logfd = creat(optarg, 0777);
      if(logfd < 0){
	fprintf(stderr, "Error: failed to create a log file: %s\n", strerror(errno));
	exit(1);
      }
    }
    else if (opt == 'c'){
      compress = 1;
      out_stream.zalloc = NULL;
      out_stream.zfree = NULL;
      out_stream.opaque = NULL;
      int ret = deflateInit(&out_stream, Z_DEFAULT_COMPRESSION);
      if (ret != Z_OK){
        fprintf(stderr, "Unable to create compression stream\n");
        exit(1);
      }
      in_stream.zalloc = NULL;
      in_stream.zfree = NULL;
      in_stream.opaque = NULL;
      ret= inflateInit(&in_stream);
      if(ret != Z_OK){
        fprintf(stderr, "Unable to create decompression stream\n");
        exit(1);
      }
      atexit(end_flates);
    }
    else{
      fprintf(stderr, "Arguments Error: --shell\n");
      exit(1);
    }
  }
  set_terminal();
  if(!port){
    fprintf(stderr, "Error: Option port is mandatory.\n");
    exit(1);
  }
  ////////////////create socket if port command is specified////////////
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "ERROR opening socket:%s", strerror(errno));
    exit(1);
  }
  server = gethostbyname("localhost");
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host.\n");
    exit_status();
    exit(1);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)&serv_addr.sin_addr.s_addr,
	(char *)server->h_addr, 
	server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){ 
    fprintf(stderr, "ERROR connecting: %s", strerror(errno));
    exit_status();
    exit(1);
  }
  //////////////////////////////////////////////////////////

  struct pollfd fds[2];
  fds[0].fd = 0;
  fds[0].events = POLLIN | POLLHUP | POLLERR;
  fds[1].fd = sockfd;
  fds[1].events = POLLIN | POLLHUP | POLLERR;
  int p;
  char c[256];
  int count;
  int i;
  
  while(1){
    p = poll(fds, 2, 0);
    if(p == -1){
      fprintf(stderr, "Failed to poll\n");
      exit_status();
      exit(1);
    }
    //read from the user input
    if(fds[0].revents & POLLIN){
      count = read(0, c,256);
      for(i = 0; i < count; i++){
	if((c[i] == '\n')||(c[i] == '\r')){
	  write(1, &lf, 1);
	  write(1, &cr, 1);
	}
	else{
	  char temp = c[i];
	  write(1, &temp, 1);
	}
      }
      if(compress){//compress
	char comp[256];
	out_stream.avail_in = count;
	out_stream.next_in = (unsigned char *) c;
	out_stream.avail_out = 256;
	out_stream.next_out = (unsigned char *) comp;

	do {
	  deflate(&out_stream, Z_SYNC_FLUSH);
	} while (out_stream.avail_in > 0);
	write(sockfd, comp, 256 - out_stream.avail_out);
	if(log){
	  char bytes[5];
	  sprintf(bytes, "%d", 256 - out_stream.avail_out);
	  write(logfd, "SENT ", 5);
	  write(logfd, bytes, strlen(bytes));
	  write(logfd, " BYTES: ", 8);
	  write(logfd, comp, 256 - out_stream.avail_out);
	  write(logfd, &lf, 1);
	  // write(logfd, "SENT ", 5);
	  // write(logfd, &si, 1);
	  // write(logfd," byte: ", 7);
	  //write(logfd, comp, 256 - out_stream.avail_out);
	  //write(logfd, &lf, 1);
	}
	
      }
      else{//no compress
	//	for(i = 0; i < count; i++){
	//  char temp = c[i];
	//  write(sockfd, &temp, 1);
	//}
       
	write(sockfd, c, count);   
	if(log){
	  char bytes[5];
	  sprintf(bytes, "%d", count);
	  write(logfd, "SENT ", 5);
	  write(logfd, bytes, strlen(bytes));
	  write(logfd, " BYTES: ", 8);
	  /*for(i = 0; i < count; i++){
	    if((c[i] == '\n')||(c[i] == '\r')){
	      write(logfd, &lf, 1);
	    }
	    else{
	      char temp = c[i];
	      write(logfd, &temp, 1);
	    }
	    }*/
	  write(logfd, c, count);
	  write(logfd, &lf, 1);
        }
      }
    }
    if(fds[0].revents & (POLLHUP)){
      //fprintf(stderr, "Error with poll for input.\n");
      exit_status();
      exit(0);
    }

    //read from the fds[1]
    if(fds[1].revents & POLLIN){
      count = read(fds[1].fd, c,256);
      if(count == 0){
	exit_status();
	exit(0);
      }
      if(compress){
	char comp[256];
        in_stream.avail_in = count;
        in_stream.next_in = (unsigned char *) c;
        in_stream.avail_out = 256;
        in_stream.next_out = (unsigned char *) comp;

        do {
          inflate(&in_stream, Z_SYNC_FLUSH);
        } while (in_stream.avail_in > 0);
	for(i = 0;i < ((int)(256 - in_stream.avail_out)); i++){
	  char temp = comp[i];
	  if((comp[i] == '\n')||(comp[i] == '\r')){
	    write(1, &cr, 1);
	    write(1, &lf, 1);
	  }
	  else
	    write(1,&temp, 1);
	}
        if(log){
          char bytes[5];
          sprintf(bytes, "%d", count);
          write(logfd, "RECEIVED ", 9);
          write(logfd, bytes, strlen(bytes));
          write(logfd, " BYTES: ", 8);
          write(logfd, c, count);
          write(logfd, &lf, 1);
	}
      }
      else{
	if(log){
	  char bytes[5];
	  sprintf(bytes, "%d", count);
	  write(logfd, "RECEIVED ", 9);
	  write(logfd, bytes, strlen(bytes));
	  write(logfd, " BYTES: ", 8);
	  /*	  for(i = 0; i < count; i++){
            if((c[i] == '\n')||(c[i] == '\r')){
              write(logfd, &lf, 1);
            }
            else{
              char temp = c[i];
              write(logfd, &temp, 1);
            }
	    }*/
	  write(logfd, &c, count);
          write(logfd, &lf, 1);
	  for(i = 0;i < count; i++){
	    char temp = c[i];
	    if((c[i] == '\n')||(c[i] == '\r')){
	      write(1, &cr, 1);
	      write(1, &lf, 1);
	    }
	    else
	      write(1,&temp, 1);
	  }
	}
      }
    }
    else if(fds[1].revents & POLLHUP){
      exit_status();
      exit(0);
    }
    else if(fds[1].revents & POLLERR){
      fprintf(stderr, "Error with poll for output.\n");
      exit_status();
      exit(1);
    }
  }
  exit(0);
}
