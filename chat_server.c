#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define PORT 8080

typedef struct {
    int socket;
    struct sockaddr_in address;
    char username[50];
    int uid;
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

time_t rawtime;
struct tm *timeinfo;

void log_message(const char *username, const char *message, const char *recipient) {
    FILE *log_file = fopen("chat_log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    if (recipient == NULL || strcmp(recipient, "ALL") == 0) {
        fprintf(log_file, "[%s] [PUBLIC] %s: %s\n", timestamp, username, message);
    } else {
        fprintf(log_file, "[%s] [PRIVATE %s->%s] %s\n", timestamp, username, recipient, message);
    }
    
    fclose(log_file);
}

void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    client_count++;
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->uid == uid) {
            clients[i] = NULL;
            break;
        }
    }
    client_count--;
    pthread_mutex_unlock(&clients_mutex);
}

void send_message_to_client(const char *message, int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->uid == uid) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Send failed");
            }
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message_to_all(const char *message, int sender_uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->uid != sender_uid) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Send failed");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int leave_flag = 0;

    // Receive username
    if (recv(cli->socket, buffer, BUFFER_SIZE, 0) <= 0 || strlen(buffer) < 2) {
        printf("Didn't receive username from client\n");
        leave_flag = 1;
    } else {
        strcpy(cli->username, buffer);
        printf("%s has joined\n", cli->username);
        
        sprintf(buffer, "%s has joined", cli->username);
        send_message_to_all(buffer, cli->uid);
        
        // Log join message
        log_message(cli->username, "joined the chat", "ALL");
    }

    bzero(buffer, BUFFER_SIZE);

    while (!leave_flag) {
        int receive = recv(cli->socket, buffer, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buffer) > 0) {
                printf("%s: %s\n", cli->username, buffer);
                
                // Check if it's a private message
                if (buffer[0] == '@') {
                    char *recipient = strtok(buffer + 1, " ");
                    char *message = strtok(NULL, "\n");
                    
                    if (recipient && message) {
                        char private_msg[BUFFER_SIZE];
                        sprintf(private_msg, "[PRIVATE from %s] %s", cli->username, message);
                        
                        pthread_mutex_lock(&clients_mutex);
                        int found = 0;
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (clients[i] && strcmp(clients[i]->username, recipient) == 0) {
                                send(clients[i]->socket, private_msg, strlen(private_msg), 0);
                                found = 1;
                                
                                // Send confirmation to sender
                                char confirm_msg[BUFFER_SIZE];
                                sprintf(confirm_msg, "[Private message sent to %s]", recipient);
                                send(cli->socket, confirm_msg, strlen(confirm_msg), 0);
                                
                                // Log private message
                                log_message(cli->username, message, recipient);
                                break;
                            }
                        }
                        
                        if (!found) {
                            char error_msg[BUFFER_SIZE];
                            sprintf(error_msg, "User %s not found or offline", recipient);
                            send(cli->socket, error_msg, strlen(error_msg), 0);
                        }
                        pthread_mutex_unlock(&clients_mutex);
                    }
                } else {
                    // Public message
                    char public_msg[BUFFER_SIZE];
                    sprintf(public_msg, "%s: %s", cli->username, buffer);
                    send_message_to_all(public_msg, cli->uid);
                    
                    // Log public message
                    log_message(cli->username, buffer, "ALL");
                }
            }
        } else {
            printf("%s has left\n", cli->username);
            sprintf(buffer, "%s has left", cli->username);
            send_message_to_all(buffer, cli->uid);
            
            // Log leave message
            log_message(cli->username, "left the chat", "ALL");
            
            leave_flag = 1;
        }

        bzero(buffer, BUFFER_SIZE);
    }

    close(cli->socket);
    remove_client(cli->uid);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    pthread_t tid;
    int opt = 1;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        return EXIT_FAILURE;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return EXIT_FAILURE;
    }

    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        return EXIT_FAILURE;
    }

    printf("Server started on port %d. Waiting for connections...\n", PORT);

    // Accept clients
    while (1) {
        socklen_t clilen = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &clilen);

        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Check if maximum clients reached
        if (client_count >= MAX_CLIENTS) {
            printf("Maximum clients reached. Rejected connection from %s\n", 
                   inet_ntoa(client_addr.sin_addr));
            close(client_socket);
            continue;
        }

        // Create client structure
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = client_addr;
        cli->socket = client_socket;
        cli->uid = rand() % 10000;
        strcpy(cli->username, "");

        // Add client to queue and create thread
        add_client(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        // Reduce CPU usage
        sleep(1);
    }

    return EXIT_SUCCESS;
}
