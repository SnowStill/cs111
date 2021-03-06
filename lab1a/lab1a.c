
//# NAME: Qinglin ZHang
//# EMAIL: qqinglin0327@gmail.com
//# ID: 205356739
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

struct termios save;
int pipe2p[2];
int pipe2c[2];
int pid;
const char lf = '\n';
const char cr = '\r';


void exit_status() {
  int status;
  waitpid(pid, &status, 0);
  //int signal = status & 0x7f;//mask
  //int stat = status & 0xff00;
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
  // Make sure stdin is a terminal. 
  if (!isatty (0)){
    fprintf (stderr, "Not a terminal.\n");
    exit (1);
  }
  int status = tcgetattr(0, &save);
  if(status < 0){
    fprintf(stderr, "Failed to save the original attributes: %s\n", strerror(errno));
    exit(1);
  }
  tcgetattr(0, &tattr);
  tattr.c_iflag = ISTRIP;
  tattr.c_oflag = 0;
  tattr.c_lflag = 0;  status = tcsetattr(0, TCSANOW, &tattr);
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

int main(int argc, char* argv[])
{
  //////////// define and set arguments flags////////////
  int shell = 0;
  int opt;
  char* filename;  
  struct option options [] = {
    {"shell", 1, 0, 's'},
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
    else if (opt == 's'){
      shell = 1;
      filename = optarg;
    }
    else{
      fprintf(stderr, "Arguments Error: --shell\n");
      exit(1);
    }
  }

  ////////////////fork if shell command is specified////////////
  set_terminal();
  if(shell){
    signal(SIGPIPE, pipe_handler);
    int p = pipe(pipe2p);
    if (p == -1) {//create 2 pipes
      fprintf(stderr, "Failed to create one of the pipes.\n");
      restore();
      exit(1);
    }
    p = pipe(pipe2c);
    if (p == -1) {
      fprintf(stderr, "Failed to create one of the pipes.\n");
      restore();
      exit(1);
    }
    pid = fork();//fork to create parent and child process
    if(pid < 0){
      fprintf(stderr, "Fork failed\n");
      restore();
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
	int s;
	waitpid(pid, &s, 0);
	restore();
	exit(1);
      }
    }
    else if (pid > 0){//parent process
      close(pipe2p[1]);//set to read only by disabling the stdout(1)
      close(pipe2c[0]);//set to write only by disabling the stdin(0)
      struct pollfd fds[2];
      fds[0].fd = 0;
      fds[0].events = POLLIN | POLLHUP | POLLERR;
      fds[1].fd = pipe2p[0];
      fds[1].events = POLLIN | POLLHUP | POLLERR;
      int p;
      char c[256];
      int count;
      int i;
      while(1){
	p = poll(fds, 2, 0);
	if(p == -1){
	  fprintf(stderr, "Failed to poll\n");
	  int s;
	  waitpid(pid, &s, 0);
	  restore();
	  exit(1);
	}
	if(fds[0].revents & POLLIN){
	  count = read(0, c,256);
	  for(i = 0; i < count; i++){
	    if(c[i] == 0x04){
	      close(pipe2p[0]);
	      close(pipe2c[1]);
	      int status = kill(pid, SIGHUP);
	      if(status < 0){
		fprintf(stderr, "Kill Error: %s\n",strerror(errno));
		int s;
		waitpid(pid, &s, 0);
		restore();
		exit(1);
	      }
	      exit_status();
	      close(pipe2p[0]);
	      restore();
	      exit(0);
	    }
	    else if(c[i] == 0x03){
	      int status = kill(pid, SIGINT);
	      if(status < 0){
		fprintf(stderr, "Kill Error: %s\n",strerror(errno));
		int s;
		waitpid(pid, &s, 0);
		restore();
		exit(1);
	      }
	    }
	    else if((c[i] == '\n')||(c[i] == '\r')){
	      write(1, &cr, 1);
	      write(1, &lf, 1);
	      write(pipe2c[1], &lf, 1);
	    }
	    else{
	      char temp = c[i];
	      write(1, &temp, 1);
	      write(pipe2c[1], &temp, 1);
	    }
	  }
	}
	if(fds[0].revents & POLLERR){
          fprintf(stderr, "Error with poll for input.\n");
	  kill(pid, SIGINT);
	  restore();
          exit(1);
        }
	if(fds[1].revents & POLLIN){
	  count = read(pipe2p[0], c,256);
	  for(i = 0; i < count; i++){
	    if( (c[i] == '\n') | (c[i] == '\r'))
	      {
		write(1, &cr, 1);
		write(1, &lf, 1);
	      }
	    else
	      {
		char temp = c[i];
		write(1, &temp, 1);
	      }
	    
	  }
	}
	if(fds[1].revents & POLLHUP){
	  exit_status();
	  close(pipe2c[1]);
	  restore();
	  exit(0);
	}
	if(fds[1].revents & POLLERR){
          fprintf(stderr, "Error with poll for output.\n");
	  int s;
	  waitpid(pid, &s, 0);
	  restore();
          exit(1);
        }
      }
    }
  }
  else{
    char c;
    int st;
    while (1) {
      st = read(0, &c, sizeof(char));
      if(st == -1){
	fprintf(stderr, "Couldn't read.: %s", strerror(errno));
	restore();
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
  } 
  restore();
  exit(0);
}
