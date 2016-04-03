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
#define SERVER_A_UDP_PORT 21853
#define SERVER_B_UDP_PORT 22853
#define SERVER_C_UDP_PORT 23853
#define SERVER_D_UDP_PORT 24853

/* Structure for holding all servers neighbour cost information*/
typedef struct server {
	char host_name[50];
	int  cost_link;
} server_data_t;

typedef struct clientList {
	struct clientList *next;
	void *obj;
} MyclientList;

/* Global variables */
int server_port = 0, client_port = 0;
MyclientList *anchor = NULL;
struct sockaddr_in client_tcp, client;
struct sockaddr_in server;
int flag = 1, adjacency_matrix[4][4];
int ser_index = 0;
int udp_serv_fd = 0;
char client_ip_address[100];
int flag_a1 = 1,flag_a2 = 1,flag_a3 = 1;
int flag_b1 = 1,flag_b2 = 1,flag_b3 = 1;
char adj_matrix_a1[6],adj_matrix_a2[6],adj_matrix_a3[6];
char adj_matrix_b1[6],adj_matrix_b2[6],adj_matrix_b3[6];
char adj_matrix_c1[6],adj_matrix_c2[6],adj_matrix_c3[6];
char adj_matrix_d1[6],adj_matrix_d2[6],adj_matrix_d3[6];
char *filename = "port.txt";

/* Display error messages */
void error(char *message) {
	perror(message);
	exit(1);
}

/* Load adjacency matrix */
void compute_adjacency(int i,server_data_t *server_data) {
	if(strcmp(server_data->host_name,"serverA") == 0) {
			adjacency_matrix[i][0] = server_data->cost_link; 
	} else if(strcmp(server_data->host_name,"serverB") == 0) {
		adjacency_matrix[i][1] = server_data->cost_link;
	} else if(strcmp(server_data->host_name,"serverC") == 0) {
		adjacency_matrix[i][2] = server_data->cost_link;
	} else if(strcmp(server_data->host_name,"serverD") == 0) {
		adjacency_matrix[i][3] = server_data->cost_link;
	}
}

/* Append all servers neighbour cost data to the list*/
void insert(MyclientList *myList,server_data_t *ser) {
	MyclientList *f = anchor;
	if(flag) {
		myList->next = NULL;
		myList->obj = (void *)ser;
		f->next = myList;
		flag = 0;
	} else {
		while(f->next != NULL) {
			f = f->next;
		}
		myList->next = NULL;
		myList->obj = (void *)ser;
		f->next = myList;
	}
}

/* Create client UDP socket for exchanging servers neighbor cost information */
void create_socket_udp(int port_number,char *send) {

	int bytes_sent = 0;
	socklen_t length = sizeof(struct sockaddr_in);
	char send_data[1024];
	
	memset(send_data,0,sizeof(send_data));
	/* Create UDP socket descriptor */
	if((udp_serv_fd = socket(AF_INET,SOCK_DGRAM,0))==-1) {
				error("UDP socket for each server");
	}
	/* Initialise socket address structure */
	server.sin_family = AF_INET;
	server.sin_port = htons(port_number);
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server.sin_zero),8);
	
	if(strlen(send) != 0) {
		strcpy(send_data,send);
		/* For syncing between client and server */
		usleep(1000000);
	} else {
		strcpy(send_data,"client to server: send neighbours information");
	}
	/* For sending computed adacency information to servers */
	if ((bytes_sent = sendto(udp_serv_fd,(void *)send_data, strlen(send_data), 0,(struct sockaddr *)&server,length)) == -1) {
        error("client: sendto");
    }
    close(udp_serv_fd);
}

/* Create client TCP socket for exchanging information with servers */
void  create_socket_tcp(char *server_id) {
	int sfd,cfd;
	char recv_data[1024];
	socklen_t client_size;	
	int yes = 1;
	
	MyclientList *client_list = NULL;
	server_data_t *ser_data = NULL; 
	/* Initialise socket address structure */
	client_tcp.sin_family = AF_INET;
	client_tcp.sin_port = htons(CLIENT_TCP_PORT);
	client_tcp.sin_addr.s_addr = INADDR_ANY; 
	bzero(&(client_tcp.sin_zero),8);
	/* Create TCP socket descriptor */
	if((sfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
				error("Server: Socket");
	}
	
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int)) == -1) {
		error("setsockopt");
	}
	/* Bind client with static TCP port and local IP address for the created TCP socket descriptor */
	if((bind(sfd,(struct sockaddr*)&client_tcp,(int)sizeof(client_tcp))) == -1) {
		close(sfd);	
		error("Client: Bind");
	}
	/*client blocks on listen call. Waiting for incoming client requests*/
	if (listen(sfd,10) == -1) {
			error("Listen");
	}
	client_size = sizeof(client);	
	/* client accepts the connection request from server*/	
		if((cfd = accept(sfd,(struct sockaddr *)&client,&client_size)) < 0) {
			error("Accept");
		}
	char server_ip_address[20];
	getpeername(cfd, (struct sockaddr*)&client,(socklen_t *) &client_size);

	struct sockaddr_in *s = (struct sockaddr_in *)&client;
	server_port = ntohs(s->sin_port);
	inet_ntop(AF_INET, &s->sin_addr, server_ip_address, sizeof server_ip_address);
	
	printf("The Client receives neighbor information from the %s with TCP port number %d and IP address %s\n",server_id,server_port,server_ip_address);
	printf("The %s has the following neighbor information:\n",server_id);
	printf("Neighbour------Cost\n");
		while(1) {
		/* Receives server neighbor cost information */
		memset(recv_data,0,sizeof(recv_data));
		if((recv(cfd,recv_data,1024,0))==-1) {
			error("Receive");
		}
		
		if(strcmp(recv_data,"exit") == 0) 
			break;
			
		client_list = (MyclientList *)malloc(sizeof(MyclientList));
		ser_data = (server_data_t *)malloc(sizeof(server_data_t));
		
		sscanf(recv_data,"%s\t%d",ser_data->host_name,&ser_data->cost_link);
		printf("%s       %d\n",ser_data->host_name,ser_data->cost_link);
		compute_adjacency(ser_index,ser_data);
	
		insert(client_list,ser_data);
	}
	close(cfd);
	close(sfd);	
	
}

/* Receive serverA neighbour cost information */
void read_serverA() {
	ser_index = 0;
	create_socket_udp(SERVER_A_UDP_PORT,"");
	create_socket_tcp("Server A");
	printf("For this connection with Server A, The Client has TCP port number %d and IP address %s\n",CLIENT_TCP_PORT,client_ip_address);
}

/* Receive serverB neighbour cost information */
void read_serverB() {
	ser_index = 1;
	create_socket_udp(SERVER_B_UDP_PORT,"");
	create_socket_tcp("Server B");
	printf("For this connection with Server B, The Client has TCP port number %d and IP address %s\n",CLIENT_TCP_PORT,client_ip_address);
}

/* Receive serverC neighbour cost information */
void read_serverC() {
	ser_index = 2;
	create_socket_udp(SERVER_C_UDP_PORT,"");
	create_socket_tcp("Server C");
	printf("For this connection with Server C, The Client has TCP port number %d and IP address %s\n",CLIENT_TCP_PORT,client_ip_address);
}

/* Receive serverD neighbour cost information */
void read_serverD() {
	ser_index = 3;
	create_socket_udp(SERVER_D_UDP_PORT,"");
	create_socket_tcp("Server D");
	printf("For this connection with Server D, The Client has TCP port number %d and IP address %s\n",CLIENT_TCP_PORT,client_ip_address);
}

/* Send the network topology information to serverA */
void send_serverA(int a) {
	char str1[4], str2[4], str3[4];
	if(adjacency_matrix[a][1] != 0) {
		memset(adj_matrix_a1,0,sizeof(adj_matrix_a1));
		strcat(adj_matrix_a1,"AB\t");
		sprintf(str1,"%d",adjacency_matrix[a][1]);
		strcat(adj_matrix_a1,str1);
		strcat(adj_matrix_a1,"\0");
	} 
	if(adjacency_matrix[a][2] != 0) {
		memset(adj_matrix_a2,0,sizeof(adj_matrix_a2));
		strcat(adj_matrix_a2,"AC\t");
		sprintf(str2,"%d",adjacency_matrix[a][2]);
		strcat(adj_matrix_a2,str2);
		strcat(adj_matrix_a2,"\0");
	} 
	if(adjacency_matrix[a][3] != 0) {
		memset(adj_matrix_a3,0,sizeof(adj_matrix_a3));
		strcat(adj_matrix_a3,"AD\t");
		sprintf(str3,"%d",adjacency_matrix[a][3]);
		strcat(adj_matrix_a3,str3);
		strcat(adj_matrix_a3,"\0");
	}	
}

/* Send the network topology information to serverB */
void send_serverB(int b) {
	
	char str1[4], str2[4], str3[4];
	
	if(adjacency_matrix[b][0] != 0) {
		memset(adj_matrix_b1,0,sizeof(adj_matrix_b1));
		strcat(adj_matrix_b1,"BA\t");
		sprintf(str1,"%d",adjacency_matrix[b][0]);
		strcat(adj_matrix_b1,str1);
		strcat(adj_matrix_b1,"\0");
	} 
	if(adjacency_matrix[b][2] != 0) {
		memset(adj_matrix_b2,0,sizeof(adj_matrix_b2));
		strcat(adj_matrix_b2,"BC\t");
		sprintf(str2,"%d",adjacency_matrix[b][2]);
		strcat(adj_matrix_b2,str2);
		strcat(adj_matrix_b2,"\0");
	}
	if(adjacency_matrix[b][3] != 0) {
		memset(adj_matrix_b3,0,sizeof(adj_matrix_b3));
		strcat(adj_matrix_b3,"BD\t");
		sprintf(str3,"%d",adjacency_matrix[b][3]);
		strcat(adj_matrix_b3,str3);
		strcat(adj_matrix_b3,"\0");
	}
}

/* Send the network topology information to serverC */
void send_serverC(int c) {
	char str1[4], str2[4], str3[4];
	
	if(adjacency_matrix[c][0] != 0) {
		memset(adj_matrix_c1,0,sizeof(adj_matrix_c1));
		strcat(adj_matrix_c1,"CA\t");
		sprintf(str1,"%d",adjacency_matrix[c][0]);
		strcat(adj_matrix_c1,str1);
		strcat(adj_matrix_c1,"\0");
	}
	if(adjacency_matrix[c][1] != 0) {
		memset(adj_matrix_c2,0,sizeof(adj_matrix_c2));
		strcat(adj_matrix_c2,"CB\t");
		sprintf(str2,"%d",adjacency_matrix[c][1]);
		strcat(adj_matrix_c2,str2);
		strcat(adj_matrix_c2,"\0");
	}
	if(adjacency_matrix[c][3] != 0) {
		memset(adj_matrix_c3,0,sizeof(adj_matrix_c3));
		strcat(adj_matrix_c3,"CD\t");
		sprintf(str3,"%d",adjacency_matrix[c][3]);
		strcat(adj_matrix_c3,str3);
		strcat(adj_matrix_c3,"\0");
	}
}

/* Send the network topology information to serverD */
void send_serverD(int d) {
	char str1[4], str2[4], str3[4];
	if(adjacency_matrix[d][0] != 0) {
		memset(adj_matrix_d1,0,sizeof(adj_matrix_d1));
		strcat(adj_matrix_d1,"DA\t");
		sprintf(str1,"%d",adjacency_matrix[d][0]);
		strcat(adj_matrix_d1,str1);
		strcat(adj_matrix_d1,"\0");
	}
	if(adjacency_matrix[d][1] != 0) {
		memset(adj_matrix_d2,0,sizeof(adj_matrix_d2));
		strcat(adj_matrix_d2,"DB\t");
		sprintf(str2,"%d",adjacency_matrix[d][1]);
		strcat(adj_matrix_d2,str2);
		strcat(adj_matrix_d2,"\0");
	} 
	if(adjacency_matrix[d][2] != 0) {
		memset(adj_matrix_d3,0,sizeof(adj_matrix_d3));
		strcat(adj_matrix_d3,"DC\t");
		sprintf(str3,"%d",adjacency_matrix[d][2]);
		strcat(adj_matrix_d3,str3);
		strcat(adj_matrix_d3,"\0");
	}
}

/* Broadcast the computed adjacency matrix to all the servers */
void broadcast_servers() {
	int a = 0, b = 1, c = 2, d = 3;
	send_serverA(a);
	send_serverB(b);
	send_serverC(c);
	send_serverD(d);
}

/* Initialise the list */
void client_list_init(MyclientList *head) {
		head->next = NULL;
		head->obj = NULL;
}

/* Computing minimum spanning tree for the adjacency matrix using Prim's algorithm */
void compute_min_spanning_tree()
{
	int a,b,u,v,i,j,ne=1;
	int visited[10]={0},min,min_cost=0;
	char tree[1024];
	char cost[10];
	
	memset(tree,0,sizeof(tree));
	visited[0]=1;
	for(i=0;i<4;i++) {
		for(j=0;j<4;j++) {
			if(i!=j && adjacency_matrix[i][j] == 0) {
				adjacency_matrix[i][j]=999;
			}
		}
	}
	while(ne < 4)
	{
		for(i=0,min=999;i<4;i++)
			for(j=0;j<4;j++)
				if(adjacency_matrix[i][j]< min)
					if(visited[i]!=0)
					{
						min=adjacency_matrix[i][j];
						a=u=i;
						b=v=j;
					}
		
		if(visited[u]==0 || visited[v]==0)
		{
			if(a==0)
			{
				if(b == 1) {
					strcat(tree,"AB\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
				if (b == 2) {
					strcat(tree,"AC\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
				if(b == 3) {
					strcat(tree,"AD\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
			}
			else if(a==1)
			{
				if(b == 0) {
					strcat(tree,"BA\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
				if(b == 2) {
					strcat(tree,"BC\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");	
				}
				if(b == 3) {
					strcat(tree,"BD\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
			}
			else if(a==2)
			{
				if(b == 0) {
					strcat(tree,"CA\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");	
				}
				if(b == 1) {
					strcat(tree,"CB\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
				if(b == 3) {
					strcat(tree,"CD\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
			}
			else if(a==3)
			{
				if(b == 0) {
					strcat(tree,"DA\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
				if(b == 1) {
					strcat(tree,"DB\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
				if(b == 2) {
					strcat(tree,"DC\t");
					sprintf(cost,"%d",min);
					strcat(tree,cost);
					strcat(tree,"\n");
				}
			}
			min_cost+=min;
			visited[b]=1;
			ne++;
		}
		adjacency_matrix[a][b]=adjacency_matrix[b][a]=999;
	}
	printf("The Client has calculated a tree.The tree cost is %d\n",min_cost);
	printf("Edge----Cost\n");
	printf("%s",tree);
}

/* Main function for client */
int main() {
	FILE *fp = NULL;
	char buff[25];
	memset(adjacency_matrix,0,sizeof(adjacency_matrix));
	anchor = (MyclientList *)malloc(sizeof(MyclientList));
	client_list_init(anchor);
	/* For obtaining localhost IP address */
	struct hostent *hp; 
	int i =0;
	struct in_addr **addr_list;
	if ( (hp = gethostbyname( "localhost" ) ) == NULL) 
	error("gethostbyname");

	addr_list = (struct in_addr **) hp->h_addr_list;

	for(i = 0; addr_list[i] != NULL; i++) 
	strcpy(client_ip_address , inet_ntoa(*addr_list[i]) );

	printf("The Client has TCP port number %d and IP address %s\n",CLIENT_TCP_PORT,client_ip_address);

	/* Receive each of the server's neighbours information */
	read_serverA();
	read_serverB();
	read_serverC();
	read_serverD();

	/* Broadcast the network topology to all servers */
	broadcast_servers();
	
	printf("The Client has sent the network topology to the Server A with UDP port number %d and IP address as %s follows:\n",SERVER_A_UDP_PORT,client_ip_address);
	printf("Edge----Cost\n");
	if(strlen(adj_matrix_a1) != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_a1);
		printf("%s\n",adj_matrix_a1);
		flag_a1 = 0;
	}
	if(strlen(adj_matrix_a2) != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_a2);
		printf("%s\n",adj_matrix_a2);
		flag_a2 = 0;
	}
	if(strlen(adj_matrix_a3) != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_a3);
		printf("%s\n",adj_matrix_a3);
		flag_a3 = 0;
	}

	if(strlen(adj_matrix_b1) != 0 && flag_a1 != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_b1);
		if(flag_a1)
			printf("%s\n",adj_matrix_b1);
	}
	if(strlen(adj_matrix_b2) != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_b2);
		printf("%s\n",adj_matrix_b2);
		flag_b1 = 0;
	}
	if(strlen(adj_matrix_b3) != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_b3);
		printf("%s\n",adj_matrix_b3);
		flag_b2 = 0;
	}
		
	if(strlen(adj_matrix_c1) != 0 && flag_a2 != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_c1);
		if(flag_a2)
		printf("%s\n",adj_matrix_c1);
	}
	if(strlen(adj_matrix_c2) != 0 && flag_b1 != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_c2);
			if(flag_b1)
		printf("%s\n",adj_matrix_c2);
	}
	if(strlen(adj_matrix_c3) != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_c3);
		printf("%s\n",adj_matrix_c3);
		flag_b3 = 0;
	}
		
	if(strlen(adj_matrix_d1) != 0 && flag_a3 != 0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_d1);
		if(flag_a3)
		printf("%s\n",adj_matrix_d1);
	}
	if(strlen(adj_matrix_d2) != 0 && flag_b2!=0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_d2);
		if(flag_b2)
		printf("%s\n",adj_matrix_d2);
	}
	if(strlen(adj_matrix_d3) != 0 && flag_b3!=0) {
		create_socket_udp(SERVER_A_UDP_PORT,adj_matrix_d3);
		if(flag_b3)
		printf("%s\n",adj_matrix_d3);
	}
	fp = fopen(filename,"r");
	fgets(buff,sizeof(buff),fp);
	client_port = atoi(buff);
	fclose(fp);
	create_socket_udp(SERVER_A_UDP_PORT,"exit"); 
	printf("For this connection with Server A, The Client has UDP port number %d and IP address %s\n",client_port,client_ip_address);

	printf("The Client has sent the network topology to the Server B with UDP port number %d and IP address as %s follows:\n",SERVER_B_UDP_PORT,client_ip_address);
	printf("Edge----Cost\n");
	if(strlen(adj_matrix_a1) != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_a1);
		printf("%s\n",adj_matrix_a1);
		flag_a1 = 0;
	}
	if(strlen(adj_matrix_a2) != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_a2);
		printf("%s\n",adj_matrix_a2);
		flag_a2 = 0;
	}
	if(strlen(adj_matrix_a3) != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_a3);
		printf("%s\n",adj_matrix_a3);
		flag_a3 = 0;
	}
		
	if(strlen(adj_matrix_b1) != 0 && flag_a1 != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_b1);
		if(flag_a1)
			printf("%s\n",adj_matrix_b1);
	}
	if(strlen(adj_matrix_b2) != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_b2);
		printf("%s\n",adj_matrix_b2);
		flag_b1 = 0;
	}
	if(strlen(adj_matrix_b3) != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_b3);
		printf("%s\n",adj_matrix_b3);
		flag_b2 = 0;
	}
		
	if(strlen(adj_matrix_c1) != 0 && flag_a2 != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_c1);
		if(flag_a2)
		printf("%s\n",adj_matrix_c1);
	}
	if(strlen(adj_matrix_c2) != 0 && flag_b1 != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_c2);
		if(flag_b1)
		printf("%s\n",adj_matrix_c2);
	}
	if(strlen(adj_matrix_c3) != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_c3);
		printf("%s\n",adj_matrix_c3);
		flag_b3 = 0;
	}
		
	if(strlen(adj_matrix_d1) != 0 && flag_a3 != 0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_d1);
		if(flag_a3)
		printf("%s\n",adj_matrix_d1);
	}
	if(strlen(adj_matrix_d2) != 0 && flag_b2!=0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_d2);
		if(flag_b2)
		printf("%s\n",adj_matrix_d2);
	}
	if(strlen(adj_matrix_d3) != 0 && flag_b3!=0) {
		create_socket_udp(SERVER_B_UDP_PORT,adj_matrix_d3);
		if(flag_b3)
		printf("%s\n",adj_matrix_d3);
	}
	fp = fopen(filename,"r");
	fgets(buff,sizeof(buff),fp);
	client_port = atoi(buff);
	fclose(fp);
	create_socket_udp(SERVER_B_UDP_PORT,"exit"); 
	printf("For this connection with Server B, The Client has UDP port number %d and IP address %s\n",client_port,client_ip_address);

	printf("The Client has sent the network topology to the Server C with UDP port number %d and IP address as %s follows:\n",SERVER_C_UDP_PORT,client_ip_address);
	printf("Edge----Cost\n");
	if(strlen(adj_matrix_a1) != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_a1);
		printf("%s\n",adj_matrix_a1);
		flag_a1 = 0;
	}
	if(strlen(adj_matrix_a2) != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_a2);
		printf("%s\n",adj_matrix_a2);
		flag_a2 = 0;
	}
	if(strlen(adj_matrix_a3) != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_a3);
		printf("%s\n",adj_matrix_a3);
		flag_a3 = 0;
	}
		
	if(strlen(adj_matrix_b1) != 0 && flag_a1 != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_b1);
		if(flag_a1)
			printf("%s\n",adj_matrix_b1);
	}
	if(strlen(adj_matrix_b2) != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_b2);
		printf("%s\n",adj_matrix_b2);
		flag_b1 = 0;
	}
	if(strlen(adj_matrix_b3) != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_b3);
		printf("%s\n",adj_matrix_b3);
		flag_b2 = 0;
	}
		
	if(strlen(adj_matrix_c1) != 0 && flag_a2 != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_c1);
		if(flag_a2)
		printf("%s\n",adj_matrix_c1);
	}
	if(strlen(adj_matrix_c2) != 0 && flag_b1 != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_c2);
		if(flag_b1)
		printf("%s\n",adj_matrix_c2);
	}
	if(strlen(adj_matrix_c3) != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_c3);
		printf("%s\n",adj_matrix_c3);
		flag_b3 = 0;
	}
		
	if(strlen(adj_matrix_d1) != 0 && flag_a3 != 0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_d1);
		if(flag_a3)
		printf("%s\n",adj_matrix_d1);
	}
	if(strlen(adj_matrix_d2) != 0 && flag_b2!=0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_d2);
		if(flag_b2)
		printf("%s\n",adj_matrix_d2);
	}
	if(strlen(adj_matrix_d3) != 0 && flag_b3!=0) {
		create_socket_udp(SERVER_C_UDP_PORT,adj_matrix_d3);
		if(flag_b3)
		printf("%s\n",adj_matrix_d3);
	}
	fp = fopen(filename,"r");
	fgets(buff,sizeof(buff),fp);
	client_port = atoi(buff);
	fclose(fp);
	create_socket_udp(SERVER_C_UDP_PORT,"exit"); 
	printf("For this connection with Server C, The Client has UDP port number %d and IP address %s\n",client_port,client_ip_address);

	printf("The Client has sent the network topology to the Server D with UDP port number %d and IP address as %s follows:\n",SERVER_D_UDP_PORT,client_ip_address);
	printf("Edge----Cost\n");
	if(strlen(adj_matrix_a1) != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_a1);
		printf("%s\n",adj_matrix_a1);
		flag_a1 = 0;
	}
	if(strlen(adj_matrix_a2) != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_a2);
		printf("%s\n",adj_matrix_a2);
		flag_a2 = 0;
	}
	if(strlen(adj_matrix_a3) != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_a3);
		printf("%s\n",adj_matrix_a3);
		flag_a3 = 0;
	}
		
	if(strlen(adj_matrix_b1) != 0 && flag_a1 != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_b1);
		if(flag_a1)
			printf("%s\n",adj_matrix_b1);
	}
	if(strlen(adj_matrix_b2) != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_b2);
		printf("%s\n",adj_matrix_b2);
		flag_b1 = 0;	
	}
	if(strlen(adj_matrix_b3) != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_b3);
		printf("%s\n",adj_matrix_b3);
		flag_b2 = 0;
	}
	if(strlen(adj_matrix_c1) != 0 && flag_a2 != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_c1);
		if(flag_a2)
		printf("%s\n",adj_matrix_c1);
	}
	if(strlen(adj_matrix_c2) != 0 && flag_b1 != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_c2);
		if(flag_b1)
		printf("%s\n",adj_matrix_c2);
	}
	if(strlen(adj_matrix_c3) != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_c3);
		printf("%s\n",adj_matrix_c3);
		flag_b3 = 0;
	}

	if(strlen(adj_matrix_d1) != 0 && flag_a3 != 0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_d1);
		if(flag_a3)
		printf("%s\n",adj_matrix_d1);
	}
	if(strlen(adj_matrix_d2) != 0 && flag_b2!=0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_d2);
		if(flag_b2)
		printf("%s\n",adj_matrix_d2);
	}
	if(strlen(adj_matrix_d3) != 0 && flag_b3!=0) {
		create_socket_udp(SERVER_D_UDP_PORT,adj_matrix_d3);
		if(flag_b3)
		printf("%s\n",adj_matrix_d3);
	}
	fp = fopen(filename,"r");
	fgets(buff,sizeof(buff),fp);
	client_port = atoi(buff);
	fclose(fp);
	create_socket_udp(SERVER_D_UDP_PORT,"exit"); 
	printf("For this connection with Server D, The Client has UDP port number %d and IP address %s\n",client_port,client_ip_address);
	/* Computing minimum spanning tree */
	compute_min_spanning_tree();
	remove(filename);
	return 0;
}
