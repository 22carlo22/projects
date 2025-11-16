#include <stdbool.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>                                                          
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/wait.h>

#define PEER_MAX 10
#define CONTENT_MAX 10
#define FILES_MAX 10
#define ADDR_MAX 10

//different commands
#define R 0
#define S 1
#define O 2
#define T 3
#define Q 4
#define A 5
#define E 6
#define D 7
#define C 8

//States
#define WAIT_EVENT 0
#define PEER_REQ 1
#define SEND_FILE 2
#define PEER_ERROR 3
#define USER_INPUT 4
#define R_TYPE 5
#define USER_ERROR 6
#define S_TYPE 7
#define O_TYPE 8
#define T_TYPE 9
#define Q_TYPE 10

//PDU for holding the user input
struct PDU{
    int command;
    char peer[PEER_MAX];
    char content[CONTENT_MAX];
    char addr[ADDR_MAX];
    char sock[10];
} myPDU;

//PDU for holding the server and peer communication
struct PDU_v2{
    int command;
    char data[100];
} serverPDU, peerPDU;


//My content servers
struct Files{
    char peer[PEER_MAX];
    char content[CONTENT_MAX];
    char addr[ADDR_MAX];
    char sock[10];
    int file_des;
} myFiles[FILES_MAX];

int files_len = 0;

fd_set rfds, afds;

int server_sock;

/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}


//Listen for any pending connections in the socket
void waitEvent(){
    FD_ZERO(&afds);
    FD_SET(0, &afds);
    int i;
    for(i = 0; i < files_len; i++) FD_SET(myFiles[i].file_des, &afds);
    memcpy(&rfds, &afds, sizeof(rfds));
    select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
}


void replaceChar(char text[], char old, char new){
    int i;
    for(i = 0; text[i] != '\0'; i++){
        if(text[i] == old) text[i] = new;
    }
}

//Extract the command and parammeters from the stdin
bool extractUserCmd(char text[]){
    char to_test[30];
    to_test[0] = text[0];
    to_test[1] = '\0';

    int i;
    for(i = 0; text[i] != '\0'; i++){
        if(text[i] == ' '){
            strcat(to_test, " ");
        }
    }


    i = 2;
    if(strcmp(to_test, "R  ") == 0 && strcmp(text, "R  ") != 0){
        replaceChar(text, ' ', '\0');

        myPDU.command = R;
        strcpy(myPDU.peer, &text[i]);
        while(text[i++] != '\0');
        strcpy(myPDU.content, &text[i]);
        return true;
    }
    else if(strcmp(to_test, "S  ") == 0 && strcmp(text, "S  ") != 0){
        replaceChar(text, ' ', '\0');

        myPDU.command = S;
        strcpy(myPDU.peer, &text[i]);
        while(text[i++] != '\0');
        strcpy(myPDU.content, &text[i]);
        return true;
    }
    else if(strcmp(to_test, "O") == 0){
        myPDU.command = O;
        return true;
    }
    else if(strcmp(to_test, "T  ") == 0 && strcmp(text, "T  ") != 0){
        replaceChar(text, ' ', '\0');

        myPDU.command = T;
        strcpy(myPDU.peer, &text[i]);
        while(text[i++] != '\0');
        strcpy(myPDU.content, &text[i]);
        return true;
    }
    else if(strcmp(to_test, "Q") == 0){
        myPDU.command = Q;
        return true;
    }
    return false;
}

//Read the PDU packet from the index server
void getServer(){
    char text[100];

    read(server_sock, text, 100);
    printf("Received from server: ");
    printf(text);
    printf("\n");

    switch(text[0]){
        case 'A':
            serverPDU.command = A;
            break;
        case 'E':
            serverPDU.command = E;
            strcpy(serverPDU.data, &text[2]);
            break;
        case 'O':
            serverPDU.command = O;
            strcpy(serverPDU.data, &text[2]);
            break;
        case 'S':
            serverPDU.command = S;
            strcpy(serverPDU.data, &text[2]);
            break;
    }
}

//Send a PDU packet to the server
void sendServer(int cmd, char p[], char c[], char addr[], char sock[]){
    char data[100] = "";
    switch(cmd){
        case R:
            strcpy(data, "R ");
            strcat(data, p);
            strcat(data, " ");
            strcat(data, c);
            strcat(data, " ");
            strcat(data, addr);
            strcat(data, " ");
            strcat(data, sock);
            break;
        case S:
            strcpy(data, "S ");
            strcat(data, c);
            break;
        case O:
            strcpy(data, "O");
            break;
        case T:
            strcpy(data, "T ");
            strcat(data, p);
            strcat(data, " ");
            strcat(data, c);
            break;
    }

    write(server_sock, data, 100);

    printf("Transmit to server: ");
    printf(data);
    printf("\n");
}

//Get the registered contents from the index server
void getServerList(char text[], int len){
    int i;
    for(i = 0; i < len; i++){
        getServer();
        strcat(text, serverPDU.data);
        strcat(text, "\n");
    }
}

//Send a packet to the connected peer
void sendPeer(int cmd, char text[], int socket){
    char data[100];
    switch(cmd){
        case E:
            strcpy(data, "E ");
            strcat(data, text);
            break;
        case D:
            strcpy(data, "D ");
            strcat(data, text);
            break;
        case C:
            strcpy(data, "C ");
            strcat(data, text);
            break;

    }
    write(socket, data, 100);
    
    printf("Transmit to peer: ");
    printf(data);
    printf("\n");
}

//Read the packet from the connected peer
void getPeer(int socket){
    char text[100];
    read(socket, text, 100);
    
    printf("Received from peer: ");
    printf(text);
    printf("\n");
    
    switch(text[0]){
        case 'E':
            peerPDU.command = E;
            strcpy(peerPDU.data, &text[2]);
            break;
        case 'D':
            peerPDU.command = D;
            strcpy(peerPDU.data, &text[2]);
            break;
        case 'C':
            peerPDU.command = C;
            strcpy(peerPDU.data, &text[2]);
            break;
        
    }
}

//Get the size of a given file
int getFileLength(char filename[]){
    FILE* fp; 
    int count = 0; 
    char c;
    fp = fopen(filename, "r"); 
    for (c = getc(fp); c != EOF; c = getc(fp)) count = count + 1; 
    fclose(fp); 
    return count;
}

//Read the incoming file from the content server
void getPeerFile(char filename[], int len, int socket){
    FILE *fptr;
    fptr = fopen(filename, "a");
    int i;
    for(i = 0; i < len; i++){
        getPeer(socket);             //Example
        fprintf(fptr, peerPDU.data);
    }

    fclose(fptr);
}

//Send file to the connected peer
void sendPeerFile(char filename[], int socket){
    char data[100];
    strcpy(data, "C ");

    int len = getFileLength(filename)/97 + 1;

    FILE *fptr;
    fptr = fopen(filename, "r");
    int i;
    for(i = 0; i < len; i++){
        fgets(&data[2], 98, fptr);

        write(socket, data, 100);
        
        printf("Transmit to peer: ");
        printf(data); 
        printf("\n");
    }

    fclose(fptr);
}

//Check if the file exist
bool fileExist(char filename[]){
    FILE* fp; 
    fp = fopen(filename, "r"); 
    bool exist = true;
    if (fp == NULL) exist = false;
    else fclose(fp);
    return exist;
}

//Create a new TCP socket for a given file
void createFileSocket(char peer[], char content[]){
    int 	sd;
	struct	sockaddr_in server, reg_addr;

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(0);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) signal(SIGCHLD, reaper);

    int alen = sizeof (struct sockaddr_in);
    getsockname(sd, (struct sockaddr *) &reg_addr, &alen);

    myFiles[files_len].file_des = sd;
    sprintf(myPDU.sock, "%d", reg_addr.sin_port);
    sprintf(myPDU.addr, "%d", server.sin_addr.s_addr);
    strcpy(myFiles[files_len].peer, peer);
    strcpy(myFiles[files_len].content, content);
    strcpy(myFiles[files_len].addr, myPDU.addr);
    strcpy(myFiles[files_len].sock, myPDU.sock);
    files_len++;
}

//Close an existing TCP socket
void destroyFileSocket(char peer[], char content[]){
    int i;
    int j;
    for(i = 0; i < files_len; i++){
        if(strcmp(myFiles[i].peer, peer) == 0 && strcmp(myFiles[i].content, content) == 0 ){
            for(j = i; j < files_len-1; j++){
                close(myFiles[j].file_des);
                strcpy(myFiles[j].peer, myFiles[j+1].peer);
                strcpy(myFiles[j].content, myFiles[j+1].content);
                strcpy(myFiles[j].addr, myFiles[j+1].addr);
                strcpy(myFiles[j].sock, myFiles[j+1].sock);
		myFiles[j].file_des = myFiles[j+1].file_des;
            }
        }
    }
    files_len--;
}

//Close the recently created socket
void destroyRecentFileSocket(){
  files_len--;
  close(myFiles[files_len].file_des);
}

//Establish a connection to the server
void connectServer(int argc, char **argv){
    char	*host = "localhost";
	int	port = 3000;
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, n, type;	/* socket descriptor and socket type	*/

	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                                                                
        sin.sin_port = htons(port);
                                                                                        
    /* Map host name to IP address, allowing for dotted decimal */
        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "Can't create socket \n");
	
                                                                                
    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Can't connect to %s\n", host);

    server_sock = s;
}

//Establish a a peer-to-peer connection
int connectPeerServer(char addr[], char sock[]){
	int 	sd, port;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	int host;

        host = atoi(addr);
        port = atoi(sock);

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = port;
        server.sin_addr.s_addr = host; 

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  exit(1);
	}

    return sd;
}

int main(int argc, char **argv){
    connectServer(argc, argv);

    char error[50];
    int state = WAIT_EVENT;
    char file_length[10];

    char user_input[30];
    char myList[150] = "";

    int temp_sock;
    struct sockaddr_in client;
    int client_len;
    int j;
    int i;

    while(true){
        switch(state){
            //Wait until one of the sockets has a pending connection
            case WAIT_EVENT:
                waitEvent();
                if(FD_ISSET(0,&rfds)) state = USER_INPUT;
                else{
                    for(i = 0; i < files_len; i++){
                        temp_sock = myFiles[i].file_des;
                        if(FD_ISSET(temp_sock, &rfds)){
			                client_len = sizeof(client);
                            temp_sock = accept(temp_sock, (struct sockaddr *)&client, &client_len);
                            state = PEER_REQ;
                            break;
                        }
                    }
                }
                break;
            //Read the command from connected peer
            case PEER_REQ:
                getPeer(temp_sock);
                if(fileExist(peerPDU.data)) state = SEND_FILE;
                else{
                    state = PEER_ERROR;
                    strcpy(error, "File does not exist");
                }
                break;
            //if the requested file exist, send the file to the connected peer
            case SEND_FILE:
                sprintf(file_length, "%d", getFileLength(peerPDU.data)/97 + 1);
                sendPeer(C, file_length, temp_sock);
                sendPeerFile(peerPDU.data, temp_sock);
                close(temp_sock);
                state = WAIT_EVENT;
                break;
            //Otherwise, prompt the connected peer with an error message
            case PEER_ERROR:
                sendPeer(E, error, temp_sock);
                close(temp_sock);
                state = WAIT_EVENT;
                break;
            //Extract the user input in the terminal
            case USER_INPUT:
                fgets(user_input, 30, stdin);
                user_input[strcspn(user_input, "\n")] = 0;
                if(!extractUserCmd(user_input)){
                    state = USER_ERROR;
                    strcpy(error, "Command is invalid");
                }
                else if(myPDU.command == R) state = R_TYPE;
                else if(myPDU.command == S) state = S_TYPE;
                else if(myPDU.command == T) state = T_TYPE;
                else if(myPDU.command == O) state = O_TYPE;
                else if(myPDU.command == Q) state = Q_TYPE;
                break;
            //Register the content
            case R_TYPE:
                createFileSocket(myPDU.peer, myPDU.content);
                sendServer(R, myPDU.peer, myPDU.content, myPDU.addr, myPDU.sock);
                getServer();
                if(serverPDU.command == A) state = WAIT_EVENT;
                else if(serverPDU.command == E){
                    state = USER_ERROR;
                    strcpy(error, serverPDU.data);
                    destroyRecentFileSocket();
                }
                break;
            //Print an error into the terminal
            case USER_ERROR:
                printf(error);
                printf("\n");
                state = WAIT_EVENT;
                break;
            //Download a content
            case S_TYPE:
                sendServer(S, NULL, myPDU.content, NULL, NULL);
                getServer();
                if(serverPDU.command == E){
                    state = USER_ERROR;
                    strcpy(error, serverPDU.data);
                }
                else if(serverPDU.command  == S){
                    j = 0;
                    while(serverPDU.data[j++] != ' ');
                    serverPDU.data[j-1] = '\0';
                    temp_sock = connectPeerServer(serverPDU.data, &serverPDU.data[j]);

                    sendPeer(D, myPDU.content, temp_sock);
                    getPeer(temp_sock);
                    if(peerPDU.command == E){
                        close(temp_sock);
                        state = USER_ERROR;
                        strcpy(error, peerPDU.data);
                    }
                    else if(peerPDU.command == C){
                        getPeerFile(myPDU.content, atoi(peerPDU.data), temp_sock);
                        close(temp_sock);
                        state = R_TYPE;
                    }
                }
                break;
            //Print the registered contents
            case O_TYPE:
                myList[0] = '\0';
                sendServer(O, NULL, NULL, NULL, NULL);
                getServer();
                getServerList(myList, atoi(serverPDU.data));
                printf(myList);
                state = WAIT_EVENT;
                break;
            //Deregister a content
            case T_TYPE:
                sendServer(T, myPDU.peer, myPDU.content, NULL, NULL);
                getServer();
                if(serverPDU.command == A) state = WAIT_EVENT;
                else if(serverPDU.command == E){
                    state = USER_ERROR;
                    strcpy(error, serverPDU.data);
                }
                break;
            //Deregister all owned content servers
            case Q_TYPE:
                while(files_len > 0){
                    sendServer(T, myFiles[0].peer, myFiles[0].content, NULL, NULL);
                    destroyFileSocket(myFiles[0].peer, myFiles[0].content);
                }
                return 0;

        }
    }
}
