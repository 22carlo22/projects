#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define PEER_MAX 10
#define CONTENT_MAX 10
#define ADDR_MAX 10

//different commands
#define R 0
#define S 1
#define O 2
#define T 3

//States
#define WAITPEER 0
#define CHECKREG 1
#define ERROR 2
#define REG 3
#define CONTENTEXIST_S 4
#define SENDADDR 5
#define SENDLIST 6
#define CONTENTEXIST_T 7
#define DEREG 8

//Table to store the indexes
struct Table{
    char peer[PEER_MAX];
    char content[CONTENT_MAX];
    char addr[ADDR_MAX];
    char sock[10];
    int recent;
} myTable[20];
int table_len = 0;

//PDU
struct PDU{
    int command;
    char peer[PEER_MAX];
    char content[CONTENT_MAX];
    char addr[ADDR_MAX];
    char sock[10];
} myPDU;


struct  sockaddr_in fsin;	/* the from address of a client	*/
char    *pts;
int	sock;			/* server socket		*/
int	alen;			/* from-address length		*/
struct  sockaddr_in sin; /* an Internet endpoint address         */
int     s, type;        /* socket descriptor and socket type    */
int 	port=3000;


int state = WAITPEER;

//Create the UDP socket
void initServer(int argc, char *argv[]){
    switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
	fprintf(stderr, "can't creat socket\n");
                                                                                
    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	fprintf(stderr, "can't bind to %d port\n",port);
    listen(s, 5);	
	alen = sizeof(fsin);
}

void replaceChar(char text[], char old, char new){
    int i;
    for(i = 0; text[i] != '\0'; i++){
        if(text[i] == old) text[i] = new;
    }
}

//Read the packet from a connected peer
void getPeerInput(){
    int v = 2;
    char data[100];
    
    if (recvfrom(s, data, 100, 0,
	(struct sockaddr *)&fsin, &alen) < 0)
    fprintf(stderr, "recvfrom error\n");
    
    printf("Received: ");
    printf(data);
    printf("\n");

    replaceChar(data, ' ', '\0');
    
    if(strcmp(data, "R") == 0){
        myPDU.command = R;

        strcpy(myPDU.peer, &data[v]);

        while(data[v++] != '\0');

        strcpy(myPDU.content, &data[v]);

        while(data[v++] != '\0');

        strcpy(myPDU.addr, &data[v]);

        while(data[v++] != '\0');

        strcpy(myPDU.sock, &data[v]);
        
    }
    else if(strcmp(data, "S") == 0){
        myPDU.command = S;
        
        strcpy(myPDU.content, &data[v]);
    }
    else if(strcmp(data, "O") == 0){
        myPDU.command = O;
        
    }
    else if(strcmp(data, "T") == 0){
        myPDU.command = T;

        strcpy(myPDU.peer, &data[v]);

        while(data[v++] != '\0');

        strcpy(myPDU.content, &data[v]);
    }

}

//Check if the content to be registered is valid
bool samePeerContent(char peer[], char content[]){
    int i;
    for(i = 0; i < table_len; i++){
        if(strcmp(peer, myTable[i].peer) == 0 && strcmp(content, myTable[i].content) == 0) return true;
    }
    return false;
}

//Check if a given content  exist in the table
bool contentExist(char content[]){
    int i;
    for(i = 0; i < table_len; i++){
        if(strcmp(content, myTable[i].content) == 0) return true;
    }
    return false;
}

//Check if a given peer exist in the table
bool contentPeerExist(char peer[], char content[]){
    int i;
    for(i = 0; i < table_len; i++){
        if(strcmp(content, myTable[i].content) == 0 && strcmp(peer, myTable[i].peer) == 0) return true;
    }
    return false;
}

//Find the least recently used index
int findContentLeastUsed(char content[]){
    int val = 999;
    int index = -1;
    int i;
    for(i = 0; i < table_len; i++){
        if(strcmp(content, myTable[i].content) == 0 && myTable[i].recent < val){
            val = myTable[i].recent;
            index = i;
        }
    }
    return index;
}


//Register a content
void appendContent(char peer[], char content[], char addr[], char sock[]){
    strcpy(myTable[table_len].peer, peer);
    strcpy(myTable[table_len].content, content);
    strcpy(myTable[table_len].addr, addr);
    strcpy(myTable[table_len].sock, sock);
    myTable[table_len].recent = 0;
    table_len++;
}

//Deregister a content
void deleteContent(char peer[], char content[]){
    int i;
    int v;
    for(i = 0; i < table_len; i++){
        if(strcmp(peer, myTable[i].peer) == 0 && strcmp(content, myTable[i].content) == 0){
            for(v = i; v < table_len - 1; v++){
                strcpy(myTable[v].peer, myTable[v+1].peer);
                strcpy(myTable[v].content, myTable[v+1].content);
                strcpy(myTable[v].addr, myTable[v+1].addr);
                strcpy(myTable[v].sock, myTable[v+1].sock);
                myTable[v].recent = myTable[v+1].recent;
            }
            table_len--;
            return;
        }
    }
}

//Send an error PDU
void sendE(char error[]){
    char data[100];
    strcpy(data, "E ");
    strcat(data, error);

    (void) sendto(s, data, 100, 0,
			(struct sockaddr *)&fsin, sizeof(fsin));
    
    printf("Transmit: ");
    printf(data);
    printf("\n");
}

//Send an acknowledge PDU
void sendA(){
    char data[100];
    strcpy(data, "A");

     (void) sendto(s, data, 100, 0,
			(struct sockaddr *)&fsin, sizeof(fsin));
    
    printf("Transmit: ");
    printf(data);
    printf("\n");
}

//Send the index of the requested content
void sendS(int i){
    myTable[i].recent++;

    char data[100];
    strcpy(data, "S ");
    strcat(data, myTable[i].addr);
    strcat(data, " ");
    strcat(data, myTable[i].sock);

     (void) sendto(s, data, 100, 0,
			(struct sockaddr *)&fsin, sizeof(fsin));
    
    printf("Transmit: ");
    printf(data);
    printf("\n");
}

//Send the registered conents
void sendO(){
    char data[100];
    strcpy(data, "O ");

    char temp[10];
    sprintf(temp, "%d", table_len);
    strcat(data, temp);

     (void) sendto(s, data, 100, 0,
			(struct sockaddr *)&fsin, sizeof(fsin));
    
    printf("Transmit: ");
    printf(data);
    printf("\n");
    
    int i;
    for(i = 0; i < table_len; i++){
        strcpy(data, "O ");
        strcat(&data[2],  &myTable[i].peer[0]);
        strcat(&data[2], " ");
        strcat(&data[2], &myTable[i].content[0]);

         (void) sendto(s, data, 100, 0,
			(struct sockaddr *)&fsin, sizeof(fsin));
        
        printf("Transmit: ");
        printf(data);
        printf("\n");
    }
}

int main(int argc, char *argv[]){
    initServer(argc, argv);

    char error[50];

    while(true){
        switch(state){
            //Wait if the UDP socket has a pending connection
            case WAITPEER:
                getPeerInput();
                if(myPDU.command == R) state = CHECKREG;
                else if(myPDU.command == S) state = CONTENTEXIST_S;
                else if(myPDU.command == O) state = SENDLIST;
                else if(myPDU.command == T) state = CONTENTEXIST_T;
                break;
            //Check if the content from a connected peer can be registered
            case CHECKREG:
                if(samePeerContent(myPDU.peer, myPDU.content)){
                    strcpy(error, "Error: Same peer and content");
                    state = ERROR;
                }
                else state = REG;
                break;
            //Prompt an error to the connected peer
            case ERROR:
                sendE(error);
                state = WAITPEER;
                break;
            //Register the content
            case REG:
                appendContent(myPDU.peer, myPDU.content, myPDU.addr, myPDU.sock);
                sendA();
                state = WAITPEER;
                break;
            //Check if the requested content exist
            case CONTENTEXIST_S:
                if(!contentExist(myPDU.content)){
                    strcpy(error, "Error: Content does not exist");
                    state = ERROR;
                }
                else state = SENDADDR;
                break;
            //Send index od the requested content
            case SENDADDR:
                sendS(findContentLeastUsed(myPDU.content));
                state = WAITPEER;
                break;
            //Send the registered index
            case SENDLIST:
                sendO();
                state = WAITPEER;
                break;
            //Prompt an error to the connected peer
            case CONTENTEXIST_T:
                if(!contentPeerExist(myPDU.peer, myPDU.content)){
                    strcpy(error, "Error: Peer and Content does not exist");
                    state = ERROR;
                }
                else  state = DEREG;
                break;
            //Deregister a conent 
            case DEREG:
                deleteContent(myPDU.peer, myPDU.content);
                sendA();
                state = WAITPEER;
                break;
        }
    }
    



    return 0;
}
