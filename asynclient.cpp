#include <iostream>
#include <cstring>     // strlen()
#include <cstdlib>     // exit()
#include <netdb.h>     // gethostbyname(), connect(), send(), recv()
#include <unistd.h>    // close(), sleep()
#include <pthread.h>   

using std::cerr;
using std::cout;
using std::endl;
using std::fill;

#define MAXBUFFERSIZE 512
#define SERVERPORT    6000
#define CLIENTPORT    6001

// Globals
int ClientSocketFD;
int errorcheck;
int Yes = 1;

struct hostent *he;
struct sockaddr_in ServerAddress;
struct sockaddr_in ClientAddress;

char Buffer[MAXBUFFERSIZE];
int NumOfBytesSent;
int NumOfBytesReceived;

// Thread function for receiving messages
void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    while (true) {
        // Blocking receive to get data from server
        NumOfBytesReceived = recv(client_socket, Buffer, MAXBUFFERSIZE-1, 0);
        if (NumOfBytesReceived <= 0) {
            if (NumOfBytesReceived == 0) {
                cout << "Server disconnected." << endl;
            } else {
                perror("recv() failed");
            }
            break;
        }
        Buffer[NumOfBytesReceived] = '\0';  // Null-terminate the received message
        cout << "Server says: " << Buffer << endl;
    }
    return NULL;
}

// Thread function for sending messages
void *send_messages(void *arg) {
    int client_socket = *((int *)arg);
    char message[MAXBUFFERSIZE];
    while (true) {
        cout << "Enter message to send to server: ";
        std::cin.getline(message, MAXBUFFERSIZE);

        // Send user input to server
        NumOfBytesSent = send(client_socket, message, strlen(message), 0);
        if (NumOfBytesSent < 0) {
            perror("send() failed");
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // Error checking: Must provide server name/IP and port to connect
    if (argc < 3) {
        cout << "ERROR000: Usage: 'Client [server name or IP] [server port]'" << endl;
        exit(-1);
    }

    // Getting server's name/IP
    he = gethostbyname(argv[1]);

    // Creating socket for client
    ClientSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (ClientSocketFD < 0) {
        perror("Failed to create socket");
        exit(-1);
    }

    setsockopt(ClientSocketFD, SOL_SOCKET, SO_REUSEADDR, &Yes, sizeof(int));

    // Initializing client address for binding
    ClientAddress.sin_family = AF_INET;
    ClientAddress.sin_addr.s_addr = INADDR_ANY;
    ClientAddress.sin_port = htons(CLIENTPORT);
    fill((char*)&(ClientAddress.sin_zero), (char*)&(ClientAddress.sin_zero) + 8, '\0');

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

    // Create threads for receiving and sending messages
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

    // Wait for threads to finish
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    // Close the client socket
    close(ClientSocketFD);
    return 0;
}
