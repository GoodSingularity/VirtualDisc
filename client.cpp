#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h> /* getprotobyname */
#include <unistd.h>
#include <sys/stat.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    int filefd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    const char *file_path = "input.tmp";
        ssize_t read_return;

    char buffer[BUFSIZ];
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket!");
    server = gethostbyname(argv[1]);
    portno = atoi(argv[2]);
    file_path = argv[3];
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
        filefd = open(file_path, O_RDONLY);
        int i = 0;
        read_return = 1;
        std::cout << "Sending" << "\n";

do{
  read_return = read(filefd, buffer, 1024);
  std::cout << ".";
  if (read_return == 0)
      perror("read");
  if (read_return == -1) {
      perror("read");
  }
  if (write(sockfd, buffer, read_return) == -1) {
      perror("write");

  }
}while(read_return > 0);
    return 0;
}
