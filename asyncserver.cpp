#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

using std::cout;
using std::endl;

// Definitions
#define MAXBUFFERSIZE 512
#define SERVERPORT 6000

// Globals
struct sockaddr_in ServerAddress;
struct sockaddr_in ClientAddress;

int ServerSocketFD;
int ClientSocketFD;
char Buffer[MAXBUFFERSIZE];
int NumOfBytesReceived;
int NumOfBytesSent;

int ServerPort;
int Yes = 1;
socklen_t sin_size;

// Function to handle receiving messages
void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    while (true) {
        NumOfBytesReceived = recv(client_socket, Buffer, MAXBUFFERSIZE-1, 0);
        if (NumOfBytesReceived <= 0) {
            if (NumOfBytesReceived == 0) {
                cout << "Client disconnected." << endl;
            } else {
                perror("recv() failed");
            }
            break;
        }
        Buffer[NumOfBytesReceived] = '\0';
        cout << "Client says: " << Buffer << endl;
    }
    return NULL;
}

// Function to handle sending messages
void *send_messages(void *arg) {
    int client_socket = *((int *)arg);
    char message[MAXBUFFERSIZE];
    while (true) {
        cout << "Enter message to send to client: ";
        std::cin.getline(message, MAXBUFFERSIZE);

        // Send user input to client
        NumOfBytesSent = send(client_socket, message, strlen(message), 0);
        if (NumOfBytesSent < 0) {
            perror("send() failed");
            break;
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    // Setting Server's Listen Port
    ServerPort = SERVERPORT;
    cout << "Server listening on default port " << SERVERPORT << endl;

    // Server socket creation
    ServerSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocketFD < 0) {
        perror("Failed to create socket");
        return -1;
    }

    // Set socket options: SO_REUSEADDR to prevent "socket in use" errors after shutdown
    setsockopt(ServerSocketFD, SOL_SOCKET, SO_REUSEADDR, &Yes, sizeof(int));

    // Server address initialization for binding
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_addr.s_addr = INADDR_ANY;
    ServerAddress.sin_port = htons(ServerPort);
    memset((char*)&(ServerAddress.sin_zero), '\0', sizeof(ServerAddress.sin_zero));

    // bind()
    if (bind(ServerSocketFD, (sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0) {
        perror("Failed to bind");
        return -1;
    }

    // listen()
    if (listen(ServerSocketFD, 5) < 0) {
        perror("Failed to listen");
        return -1;
    }

    // Accept will block and wait for connections
    sin_size = sizeof(ClientAddress);
    ClientSocketFD = accept(ServerSocketFD, (sockaddr *)&ClientAddress, &sin_size);
    if (ClientSocketFD < 0) {
        perror("Failed to accept client connection");
        return -1;
    }

    cout << "*** Server got connection from " << inet_ntoa(ClientAddress.sin_addr) << " on socket '" << ClientSocketFD << "' ***" << endl;

    // Create threads for receiving and sending messages
    pthread_t receive_thread, send_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void*)&ClientSocketFD) != 0) {
        perror("Failed to create receive thread");
        close(ClientSocketFD);
        close(ServerSocketFD);
        return -1;
    }

    if (pthread_create(&send_thread, NULL, send_messages, (void*)&ClientSocketFD) != 0) {
        perror("Failed to create send thread");
        close(ClientSocketFD);
        close(ServerSocketFD);
        return -1;
    }

    // Wait for threads to finish
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    // Close connection
    close(ClientSocketFD);
    close(ServerSocketFD);
    return 0;
}
