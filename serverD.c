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

#define CLIENT_TCP_PORT 25853
#define SERVER_D_UDP_PORT 24853

/* Structure for holding serverD neighbour cost information*/
typedef struct serverDList {
	void *obj;
	struct serverDList *next;
} MyserverDList;

typedef struct serverD{
	char host_name[50];
	int  cost_link;
}serverD_t;

/* Global variables */
char ip_address[20];
int server_port = 0, client_port = 0, check = 1;
MyserverDList *anchor = NULL;
int flag = 1, udp_serv_fd = 0, tcp_sfd = 0;
struct sockaddr_in server_udp;
struct sockaddr_in server_tcp;

/* Display error messages */
void error(char *message) {
	perror(message);
	exit(1);
}

/* Append serverD neighbour cost data to the list*/
void insert(MyserverDList *myList,serverD_t *serD) {
	MyserverDList *f = anchor;
	if(flag) {
		myList->next = NULL;
		myList->obj = (void *)serD;
		f->next = myList;
		flag = 0;
	} else {
		while(f->next != NULL) {
			f = f->next;
		}
		myList->next = NULL;
		myList->obj = (void *)serD;
		f->next = myList;
	}
}

/* Read contents of serverD.txt, that contains serverD neighbour cost information */
void read_input_file(FILE *handler) {
	char buf[1024];
	MyserverDList *myList = NULL;
	serverD_t *serD = NULL; 
	while(fgets(buf,sizeof(buf),handler)) {
		myList = (MyserverDList *)malloc(sizeof(MyserverDList));
		serD = (serverD_t *)malloc(sizeof(serverD_t));
		sscanf(buf,"%s\t%d",serD->host_name,&serD->cost_link);
		printf("%s       %d\n",serD->host_name,serD->cost_link);
		insert(myList,serD);
	}
}

/* Create server UDP socket for exchanging information with client*/
void create_socket_udp(int flag) {
	
	int bytes_received = 0;
	socklen_t length = sizeof(struct sockaddr_in);
	char receive_data[1024],adjacency[100];
	int cost;
	memset(receive_data,0,sizeof(receive_data));
	/* Create UDP socket descriptor */
	if((udp_serv_fd = socket(AF_INET,SOCK_DGRAM,0))==-1) {
				error("UDP socket for serverD");
	}
	/* Initialise socket address structure */
	server_udp.sin_family = AF_INET;
	server_udp.sin_port = htons(SERVER_D_UDP_PORT);
	server_udp.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_udp.sin_zero),8);
	/* Bind serverC with static UDP port and local IP address for the created UDP socket descriptor */
	if(bind(udp_serv_fd,(struct sockaddr *)&server_udp,(int)length) < 0) {
			error("server: bind");
	}
	
	if(flag == 0) {
		/* For obtaining information from client */
		if ((bytes_received = recvfrom(udp_serv_fd, receive_data, sizeof(receive_data), 0,(struct sockaddr *)&server_udp,&length)) == -1) {
			error("serverD: recvfrom");
		}
	/* To obtain server's peer address and port number */
	getpeername(udp_serv_fd, (struct sockaddr*)&server_udp,&length);
	struct sockaddr_in *s = (struct sockaddr_in *)&server_udp;
	server_port = ntohs(s->sin_port);
	inet_ntop(AF_INET, &s->sin_addr, ip_address, sizeof ip_address);
	} else {
		while(1) {
		memset(receive_data,0,sizeof(receive_data));
		/* For obtaining information from client */
		if ((bytes_received = recvfrom(udp_serv_fd, receive_data, sizeof(receive_data), 0,(struct sockaddr *)&server_udp,&length)) == -1) {
			error("serverD: recvfrom"); 
		}
		/* To obtain server's peer address and port number */
		getpeername(udp_serv_fd, (struct sockaddr*)&server_udp,&length);
		struct sockaddr_in *s = (struct sockaddr_in *)&server_udp;
		client_port = ntohs(s->sin_port);
		FILE *fp = NULL;
		fp = fopen("port.txt","w");
		fprintf(fp,"%d",client_port);
		fclose(fp);
		if(check) {
			printf("The server D has received the network topology from the Client with UDP port number %d and %s IP address as follows:\n",client_port,ip_address);
			printf("Edge------Cost\n");
			check = 0;
		}
			if(strcmp(receive_data,"exit") == 0) 
				break;
			sscanf(receive_data,"%s\t%d",adjacency,&cost);
			printf("%s       %d\n",adjacency,cost);		
		}
	}
	close(udp_serv_fd);
}

/* Create server TCP socket for exchanging information with client*/
void create_socket_tcp() {
	int bytes_sent;
	char send_data[1024];
	MyserverDList *node = anchor; 
	char cost[10];
	/* Create TCP socket descriptor */
	if((tcp_sfd = socket(AF_INET,SOCK_STREAM,0))==-1) {
				error("Socket");
		}
	/* Initialise socket address structure */
	server_tcp.sin_family = AF_INET;
	server_tcp.sin_port = htons(CLIENT_TCP_PORT);
	struct in_addr **addr_list;
    char ip[100];
    int i =0;
    struct hostent *hp; 
             
    if ( (hp = gethostbyname( "localhost" ) ) == NULL) 
		error("gethostbyname");
    
     addr_list = (struct in_addr **) hp->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
		strcpy(ip , inet_ntoa(*addr_list[i]) );
		
	server_tcp.sin_addr.s_addr = inet_addr(ip);
	bzero(&(server_tcp.sin_zero),8);
	/* For Syncing between client and server */
	usleep(1000000);
	if((connect(tcp_sfd,(struct sockaddr *)&server_tcp,sizeof(server_tcp)))==-1) {
		error("server: Connect");
	}
	while(node->next != NULL) {
		
		node = node->next;
		memset(send_data,0,sizeof(send_data));
		strcat(send_data,((serverD_t *)node->obj)->host_name);
		sprintf(cost,"%d",((serverD_t *)node->obj)->cost_link);
		strcat(send_data,"\t");
		strcat(send_data,cost);
		/* For Syncing between client and server */
		usleep(1000000);
		/* send the server's neighbour information to client */
		bytes_sent = send(tcp_sfd,send_data,strlen(send_data),0);
		/* For Syncing between client and server */
		usleep(1000000);
		if(bytes_sent < 0) {
			error("server: send");
		}
	}
	send(tcp_sfd,"exit",10,0);
	close(tcp_sfd); 
}

/*Initialise the list */
void myserverD_list_init(MyserverDList *head) {
		head->next = NULL;
		head->obj = NULL;
}

/* Main function for serverD */
int main() {
		
	FILE *fp = NULL;
	anchor = (MyserverDList *)malloc(sizeof(MyserverDList));
	/*Initialise the list */
	myserverD_list_init(anchor);
	
	printf("The Server D is up and running\n");
	printf("The Server D has the following neighbor information:\n");
	printf("Neighbor------Cost\n");
	
	fp = fopen("serverD.txt","r");
	if(fp == NULL) {
		error("File is empty!");
	} else {
		read_input_file(fp);
	}
	fclose(fp);
	/* Create UDP socket for exchanging information with client*/
	create_socket_udp(0);
	/* Create TCP socket for sending serverD neighbour cost information to client */
	create_socket_tcp();
	printf("The Server D finishes sending its neighbor information to the Client with TCP port number %d and IP address %s\n",CLIENT_TCP_PORT,ip_address);
	printf("For this connection with the Client, the Server D has TCP port number %d and IP address %s\n",server_port,ip_address);
	/* For obtaining adjacency matrix from client side*/
	create_socket_udp(1);
	printf("For this connection with the Client, the Server D has UDP port number %d and IP address %s\n",SERVER_D_UDP_PORT,ip_address);
	return 0;
}
