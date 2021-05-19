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

struct termios save;
int pipe2p[2];
int pipe2c[2];
int pid;
const char lf = '\n';
const char cr = '\r';
z_stream out_stream;
z_stream in_stream;
int sockfd;
int new_socket;

void exit_status() {
  int status;
  waitpid(pid, &status, 0);
  //int signal = status & 0x7f;//mask
  //int stat = status & 0xff00;
  close(sockfd);
  close(new_socket);
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", WIFSIGNALED(status), WEXITSTATUS(status));
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
  /* Make sure stdin is a terminal. */
  if (!isatty (0)){
    fprintf (stderr, "Not a terminal.\n");
    exit (1);
  }
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
void pipe_handler(int signum){
  if(signum == SIGPIPE)
    {
      fprintf(stderr, "receive SIGPIEP.\n"); 
      exit(1);
    }
}
void end_flates(){
  deflateEnd(&out_stream);
  inflateEnd(&in_stream);
}
int main(int argc, char* argv[])
{
  //////////// define and set arguments flags////////////
  int shell = 0;
  int port = 0;
  int compress = 0;
  int opt;
  char* filename;  
  int portno;
  struct sockaddr_in serv_addr;
  struct sockaddr_in client_addr;
  int client_size;

  struct option options [] = {
    {"shell", 1, 0, 's'},
    {"port", 1, 0, 'p'},
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
    else if ( opt == 's'){
      shell = 1;
      filename = optarg;
    }
    else if (opt == 'p'){
      port = 1;
      portno = atoi(optarg);
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
  if(!port){
    fprintf(stderr, "Error: Option port is mandatory.\n");
    exit(1);
  }
  //set_terminal();
  ////////////////create socket if port command is specified////////////
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "ERROR opening socket:%s", strerror(errno));
    exit(1);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  //bind instead of connect for server
  if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){ 
    fprintf(stderr, "Failed to bind: %s", strerror(errno));
    exit(1);
  }
  listen(sockfd, 5);
  client_size = sizeof(client_addr);
  new_socket = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *) &client_size);
  if(new_socket < 0){
    fprintf(stderr, "Failed to accept the new socket.");
    exit(1);
  }
  if(shell){
    signal(SIGPIPE, pipe_handler);
    int p = pipe(pipe2p);
    if (p == -1) {//create 2 pipes
      fprintf(stderr, "Failed to create one of the pipes.\n");
      exit(1);
    }
    p = pipe(pipe2c);
    if (p == -1) {
      fprintf(stderr, "Failed to create one of the pipes.\n");
      exit(1);
    }
    pid = fork();//fork to create parent and child process
    if(pid < 0){
      fprintf(stderr, "Fork failed\n");
      exit(1);
    }
    else if(pid == 0){//child process
      close(pipe2p[0]);//set to write only by disabling the stdin(0)
      close(pipe2c[1]);//set to read only by disabling the stdout(1)
      dup2(pipe2c[0], 0); 
      dup2(pipe2p[1], 1);     //assign the pipe2p to the stdout
      dup2(pipe2p[1], 2);
      close(pipe2c[0]);
      close(pipe2p[1]);
      char *args[]={filename, NULL}; 
      if(execvp(filename,args) == -1){//https://www.geeksforgeeks.org/exec-family-of-functions-in-c/
	fprintf(stderr, "Failed to exectue shell scripts %s\n",strerror(errno) );
	exit(1);
      }
    }
    else if (pid > 0){//parent process
      close(pipe2p[1]);//set to read only by disabling the stdout(1)
      close(pipe2c[0]);//set to write only by disabling the stdin(0)
      struct pollfd fds[2];
      fds[0].fd = new_socket;
      fds[0].events = POLLIN | POLLHUP | POLLERR;
      fds[1].fd = pipe2p[0];
      fds[1].events = POLLIN | POLLHUP | POLLERR;
      int p;
      char c[256];
      int count;
      int i;
      while(1){;
	p = poll(fds, 2, 0);
	if(p == -1){
	  fprintf(stderr, "Failed to poll\n");
	  exit(1);
	}
	if(fds[0].revents & POLLIN){
	  count = read(new_socket, c,256);
	  if(compress){//decompression
	    char comp[256];
	    in_stream.avail_in = count;
	    in_stream.next_in = (unsigned char *) c;
	    in_stream.avail_out = 256;
	    in_stream.next_out = (unsigned char *) comp;
	    do {
	      inflate(&in_stream, Z_SYNC_FLUSH);
	    } while (in_stream.avail_in > 0);
	    for(i = 0; i <((int)(256 - in_stream.avail_out)); i++){
              if(comp[i] == 0x04){
                close(pipe2p[0]);
                close(pipe2c[1]);
                int status = kill(pid, SIGHUP);
                if(status < 0){
                  fprintf(stderr, "Kill Error: %s\n",strerror(errno));
                  exit(1);
                }
                exit_status();
                exit(0);
              }
              else if(comp[i] == 0x03){
                int status = kill(pid, SIGINT);
                if(status < 0){
                  fprintf(stderr, "Kill Error: %s\n",strerror(errno));
                  exit(1);
                }
              }
              else if((comp[i] == '\n')||(comp[i] == '\r')){
		//write(1, &cr, 1);
		//write(1, &lf, 1);
                write(pipe2c[1], &lf, 1);
              }
              else{
                char temp = comp[i];
                //write(1, &temp, 1);
                write(pipe2c[1], &temp, 1);
		//fprintf("%c", temp);
              }
	    }
	  }
	  else{//no decompression
	    for(i = 0; i < count; i++){
	      if(c[i] == 0x04){
		close(pipe2p[0]);
		close(pipe2c[1]);
		int status = kill(pid, SIGHUP);
		if(status < 0){
		  fprintf(stderr, "Kill Error: %s\n",strerror(errno));
		  exit(1);
		}
		exit_status();
		exit(0);
	      }
	      else if(c[i] == 0x03){
		int status = kill(pid, SIGINT);
		if(status < 0){
		  fprintf(stderr, "Kill Error: %s\n",strerror(errno));
		  exit(1);
		}
	      }
	      else if((c[i] == '\n')||(c[i] == '\r')){
		//		write(1, &cr, 1);
		//write(1, &lf, 1);
		write(pipe2c[1], &lf, 1);
	      }
	      else{
		char temp = c[i];
		//write(1, &temp, 1);
		write(pipe2c[1], &temp, 1);
	      }
	    }
	  }
	}
	if(fds[0].revents & (POLLERR | POLLHUP)){
          //fprintf(stderr, "Error with poll for input.\n");
	  kill(pid, SIGINT);
          exit(0);
        }
	if(fds[1].revents & POLLIN){
	  count = read(fds[1].fd, c, 256);
	  //fprintf(stderr,"%s",c);
	  if(compress){//compression
	    char comp[256];
	    out_stream.avail_in = count;
	    out_stream.next_in = (unsigned char *) c;
	    out_stream.avail_out = 256;
	    out_stream.next_out = (unsigned char *) comp;
	    do {
	      deflate(&out_stream, Z_SYNC_FLUSH);
	    } while (out_stream.avail_in > 0);
	    write(new_socket, comp, 256 - out_stream.avail_out);
	    //write(1, comp, 256 - out_stream.avail_out);
	      }
	  else{
	    write(new_socket, c, count);
	  }
	    
	}
	if(fds[1].revents & POLLHUP){
	  exit_status();
	  close(pipe2c[1]);
	  exit(0);
	}
	if(fds[1].revents & POLLERR){
          //fprintf(stderr, "Error with poll for output.\n");
          exit(0);
        }
      }
    }
  }
  /*else{
    char c;
    int st;
    while (1) {
      st = read(0, &c, sizeof(char));
      if(st == -1){
	fprintf(stderr, "Couldn't read.: %s", strerror(errno));
	exit(1);
      }
      if(c == 0x04)
	break;
      if ((c == '\r' )|| (c == '\n')) { 
	write(1, &cr, 1);
	write(1, &lf, 1);
      } else 
	write(1, &c, 1);      
    }
    } */ 
  exit(0);
}
