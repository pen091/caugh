#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int client_socket;
char username[50];

void *receive_handler(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int receive = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (receive > 0) {
            printf("\r%s\npen-->> ", buffer);
            fflush(stdout);
        } else if (receive == 0) {
            break;
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    pthread_t recv_thread;
    char buffer[BUFFER_SIZE];

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change to server IP if needed

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return EXIT_FAILURE;
    }

    // Get username from user
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // Remove newline

    // Send username to server
    send(client_socket, username, strlen(username), 0);

    printf("Connected to chat server. You can start messaging.\n");
    printf("Use '@username message' for private messages\n");
    printf("pen-->> ");

    // Create thread for receiving messages
    pthread_create(&recv_thread, NULL, receive_handler, NULL);

    // Send messages
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        if (strlen(buffer) > 0) {
            send(client_socket, buffer, strlen(buffer), 0);
        }

        printf("pen-->> ");
        fflush(stdout);
    }

    close(client_socket);
    return EXIT_SUCCESS;
}
