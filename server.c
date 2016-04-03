/*EE450 - Client-Server Program */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/wait.h>

#define SERVER_TCP_PORT 6423

void error(char *message) {
	perror(message);
	exit(1);
}

int main() {
		struct sockaddr_in server,client;
		int pfd,cfd;
		char recv_data[1024],send_data[1024];
		socklen_t client_size;
		
		if((pfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
				error("Socket");
		}
		server.sin_family = AF_INET;
		server.sin_port = htons(SERVER_TCP_PORT);
		server.sin_addr.s_addr = INADDR_ANY; 
		bzero(&(server.sin_zero),8);
		
		if((bind(pfd,(struct sockaddr *)&server,(int)sizeof(server))) == -1) {
				error("Binding");
		}
		
		if((listen(pfd,10)) == -1) {
				error("Listen");
		}
		
		client_size = sizeof(client);
		cfd = accept(pfd,(struct sockaddr *)&client,&client_size);
		if(cfd < 0) {
			error("Accept");
		}
		if((recv(cfd,recv_data,1024,0))==-1) {
			error("Receive");
		}
		printf("\nReceived data: %s",recv_data);
		
		/*sendto(cfd,send_data,strlen(send_data),0,(struct sockaddr *)&client,sizeof(client_size));
		printf("send data: %s",send_data); */
		close(cfd);
		close(pfd);
		return 0;
}
