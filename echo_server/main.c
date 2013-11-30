#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define true 1
#define false 0

#define CHILD_PROCESSES_COUNT 2

struct server {
  struct sockaddr_in addr;
  int listener;
};

int child_processes_count = 0;

void handle_request(int sock) {
  char buf[1024];
  while(true) {
    ssize_t bytes_read = recv(sock, buf, 1024, 0);

    if(bytes_read <= 0) break;
    send(sock, buf, bytes_read, 0);
  }

  printf("Worker end handle request. \n");
  close(sock);
}

struct server start_server(struct sockaddr_in addr) {
  struct server srv;

  srv.listener = socket(AF_INET, SOCK_STREAM, 0);
  if(srv.listener < 0) {
    perror("listen");
    exit(1);
  }

  if(bind(srv.listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(2);
  }

  return srv;
}

void connection_acceptor(int listener) {
  while(1) {
    int sock = accept(listener, NULL, NULL);

    if(sock < 0) {
      perror("accept");
      exit(3);
    }

    printf("Get incomming connection\n");
    printf("child_processes_count %d\n", child_processes_count);

    if (child_processes_count < CHILD_PROCESSES_COUNT) {
      printf("Can spawn new worker. \n");

      child_processes_count++;
      pid_t pID = fork();

      if (pID == 0) {
        //if is child process
        printf("Worker start handle request. \n");
        handle_request(sock);
        break;
      }
    }
  }
}

void sig_handler(int signo) {
  // when child process died
  if (signo == SIGCHLD) {
    // descrease child process count
    child_processes_count--;
  }
}

int main(int argc, char* argv[]) {
  signal(SIGCHLD, sig_handler);

  int port = atoi(argv[1]);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  struct server srv = start_server(addr);

  printf("Wait for incomming connections...\n");
  listen(srv.listener, 1);
  connection_acceptor(srv.listener);
  return 0;
}
