/**
    Handle multiple socket connections with select and fd_set on Linux
*/

#include <stdio.h>
#include <string.h>   //strlen
#include <string>
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h> /* getprotobyname */
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <omp.h>
#include <ctime>
using namespace std;
using std::string;

#define TRUE   1
#define FALSE  0
#define PORT 8888

class Server{

  private:
		time_t czas;
		int opt, master_socket , addrlen , max_clients;
  	struct sockaddr_in address;


		void InitialiseData(int &client,int &new_socket, bool &value, char *file_path, int &filefd){
			if( client == 0 )
			{
				client = new_socket;
				value = false;
				strcpy(file_path, "./x");
    		file_path[2] = (char)client + 0x33;
		//file_path[i] = GenerateFileName(i, i);
				filefd = open(file_path,O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
				printf("Adding to list of sockets as %d\n" , client);
			}
		}
		void setMasterSocket(){
		   //create a master socket
	  	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
	    {
				perror("socket failed!");
				exit(EXIT_FAILURE);
	    }

	    //set master socket to allow multiple connections , this is just a good habit, it will work without this
	    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
	    {
				perror("setsockopt");
				exit(EXIT_FAILURE);
	    }
	    //type of socket created
	    address.sin_family = AF_INET;
	    address.sin_addr.s_addr = INADDR_ANY;
	    address.sin_port = htons( PORT );

	    //bind the socket to localhost port 8888
	    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
	    {
				perror("bind failed");
				exit(EXIT_FAILURE);
	    }
	    printf("Listener on port %d \n", PORT);

	    //try to specify maximum of 30 pending connections for the master socket
	    if (listen(master_socket, max_clients) < 0)
	    {
				perror("listen");
				exit(EXIT_FAILURE);
	    }

	    //accept the incoming connection
	    addrlen = sizeof(address);
	    puts("Waiting for connections ...");
	}

   bool disconnected(int &sd, ssize_t &valread){
		 if (valread == 0)
		 {
				//Somebody disconnected , get his details and print
			 getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
			 printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

				//Close the socket and mark as 0 for reuse
			 sd = 0;
			 valread = 1;
			 close( sd );
			 return true;
		 }
		 return false;
	}
  void ReceiveFile(int &client_socket, bool &value, int &filefd, ssize_t &read_return, char *buffer, int &tid){
		while((value =disconnected(client_socket, read_return))== false)
		{
		  if (filefd == -1) {
				perror("open");
				value = true;
				break;
			}
			read_return = read(client_socket, buffer, 1024);
			write(filefd, buffer, read_return);
			while (read_return > 0){
				read_return = read(client_socket, buffer, 1024);
				if (read_return == -1) {
						perror("read");
						value = true;
						break;
				}
				if (write(filefd, buffer, read_return) == -1) {
						value = true;
						perror("write");
						break;
				}
			}
	}
	delete buffer;
}
public:
	void Serverloop(){
		char buffer[BUFSIZ];
		bool value;
		ssize_t read_return;
		int filefd;
		int client_socket;
		int new_socket;
		int max_sd;
		int activity;
		int tid=0;
		char file_path[]="";
		fd_set readfds;

	 	#pragma omp parallel private(readfds,activity,max_sd, tid, buffer, value, read_return, filefd, file_path, client_socket, new_socket)
		{
			while(true)
    	{
        	FD_ZERO(&readfds);

        //add master socket to set
        	FD_SET(master_socket, &readfds);
        	max_sd = master_socket;

        //add child sockets to set
					if(tid==0){
						memset( buffer, '\0', sizeof(buffer) );
						read_return = 1;
						value = false;
 						tid = omp_get_thread_num();
					}
            //socket descriptor

            //if valid socket descriptor then add to read list
          if(client_socket > 0)
              FD_SET( client_socket , &readfds);

            //highest file descriptor number, need it for the select function
          if(client_socket > max_sd)
              max_sd = client_socket;
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely

        	activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        	if ((activity < 0) && (errno!=EINTR))
        	{
            printf("select error");
        	}
        //If something happened on the master socket , then its an incoming connection

        	if (FD_ISSET(master_socket, &readfds))
        	{
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
              perror("accept");
              exit(EXIT_FAILURE);
            }
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						InitialiseData(client_socket,new_socket, value, file_path, filefd);
					}
          if (FD_ISSET( client_socket , &readfds))
          {
						ReceiveFile(client_socket, value, filefd, read_return, buffer, tid);
          }
				}
			}
		}
public:
	Server(){
		max_clients = 30;
	setMasterSocket();
	}
};
int main(int argc , char *argv[])
{
Server *ss = new Server();
ss->Serverloop();
}
