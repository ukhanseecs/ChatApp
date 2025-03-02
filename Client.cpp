// The Client ( Client.cpp )

// ***************************************** Header Files ***********************************************
#include <iostream>
#include <cstring>		// strlen()
#include <cstdlib>		// exit()
#include <netdb.h>		// gethostbyname(), connect(), send(), recv()
#include <unistd.h>
#include <pthread.h>   


using std::cerr;
using std::cout;
using std::endl;
using std::fill;
// ****************************************** #Defintions ***********************************************
#define  MAXBUFFERSIZE		512		// Maximum default buffersize.
#define  SERVERPORT		   6000		// Server will be listening on this port by default.
#define  CLIENTPORT        6001		// Client will running on this port.
// ******************************************************************************************************

// *********************************************** Globals ************************************************
int AddressLength;
int ClientSocketFD;
int errorcheck;
int Yes = 1;

struct hostent *he;
struct sockaddr_in ServerAddress;
struct sockaddr_in ClientAddress;

// Client's Buffer.
char Buffer[MAXBUFFERSIZE];
int NumOfBytesSent;
int NumOfBytesReceived;
// ********************************************************************************************************


void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    while (true) {
        NumOfBytesReceived = recv(client_socket, Buffer, MAXBUFFERSIZE-1, 0);
        if (NumOfBytesReceived <= 0) {
            if (NumOfBytesReceived == 0) {
                cout << "server disconnected." << endl;
            } else {
                perror("recv() failed");
            }
            break;
        }
        Buffer[NumOfBytesReceived] = '\0';  
        cout << "server says: " << Buffer << endl;
    }
    return NULL;
}

void *send_messages(void *arg) {
    int client_socket = *((int *)arg);
    char message[MAXBUFFERSIZE];
    while (true) {
        cout << "enter message to send to server: ";
        std::cin.getline(message, MAXBUFFERSIZE);

        NumOfBytesSent = send(client_socket, message, strlen(message), 0);
        if (NumOfBytesSent < 0) {
            perror("send() failed");
            break;
        }
    }
    return NULL;
}


int main (int argc, char *argv[])
{
	// Standard error checking. Must provide server name/IP and port to connect.
	if ( argc < 3 )
	{
		cout << "ERROR000: Usage: 'Client [server name or IP] [server port]'" << endl;
		exit (-1);
	}

	// Getting server's name/IP.
	he = gethostbyname (argv[1]);

	// Creating a socket for the client.
	ClientSocketFD = socket ( AF_INET, SOCK_STREAM, 0 );


	setsockopt (ClientSocketFD, SOL_SOCKET, SO_REUSEADDR, &Yes, sizeof (int));

	// Initializing Client address for binding.
	ClientAddress.sin_family = AF_INET;					// Socket family.
	ClientAddress.sin_addr.s_addr = INADDR_ANY;			// Assigninig client IP.
	ClientAddress.sin_port = htons (CLIENTPORT);		// Client port.
	fill ((char*)&(ClientAddress.sin_zero), (char*)&(ClientAddress.sin_zero)+8, '\0');


	// bind()
    if (bind(ClientSocketFD, (sockaddr *)&ClientAddress, sizeof(ClientAddress)) < 0) {
        perror("Failed to bind");
        close(ClientSocketFD);
        exit(-1);
    }

    // Initializing server address to connect to
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_addr = *((in_addr *)(*he).h_addr);
    ServerAddress.sin_port = htons(atoi(argv[2]));  // Server port provided as argument
    fill((char*)&(ServerAddress.sin_zero), (char*)&(ServerAddress.sin_zero) + 8, '\0');

    // Connecting to server
    if (connect(ClientSocketFD, (sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0) {
        perror("Failed to connect to server");
        close(ClientSocketFD);
        exit(-1);
    }

    cout << "Connected to server at " << argv[1] << ":" << argv[2] << endl;

    pthread_t receive_thread, send_thread;

    if (pthread_create(&receive_thread, NULL, receive_messages, (void*)&ClientSocketFD) != 0) {
        perror("Failed to create receive thread");
        close(ClientSocketFD);
        exit(-1);
    }

    if (pthread_create(&send_thread, NULL, send_messages, (void*)&ClientSocketFD) != 0) {
        perror("Failed to create send thread");
        close(ClientSocketFD);
        exit(-1);
    }

    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    close(ClientSocketFD);
    return 0;
}
// ********************************************************************************************************