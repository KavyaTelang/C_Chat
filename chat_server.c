#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define USERNAME_SIZE 32

typedef struct {
    int socket_fd;
    char username[USERNAME_SIZE];
    struct sockaddr_in address;
    int uid;
} client_t;

client_t *clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int uid = 0;

void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void print_client_addr(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = cl;
            client_count++;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->uid == uid) {
            clients[i] = NULL;
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(char *message, int sender_uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->uid != sender_uid) {
            if (send(clients[i]->socket_fd, message, strlen(message), 0) < 0) {
                perror("Send failed");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_to_client(char *message, int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->uid == uid) {
            if (send(clients[i]->socket_fd, message, strlen(message), 0) < 0) {
                perror("Send failed");
            }
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_active_users(int uid) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "\n=== Active Users ===\n");
    send_to_client(buffer, uid);
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            snprintf(buffer, sizeof(buffer), "  - %s\n", clients[i]->username);
            send_to_client(buffer, uid);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    snprintf(buffer, sizeof(buffer), "===================\n");
    send_to_client(buffer, uid);
}

void *handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + USERNAME_SIZE + 20];
    int leave_flag = 0;
    
    client_t *cli = (client_t *)arg;
    
    // Receive username
    if (recv(cli->socket_fd, cli->username, USERNAME_SIZE, 0) <= 0) {
        leave_flag = 1;
    } else {
        str_trim_lf(cli->username, USERNAME_SIZE);
        if (strlen(cli->username) < 2 || strlen(cli->username) >= USERNAME_SIZE - 1) {
            printf("Invalid username from ");
            print_client_addr(cli->address);
            printf("\n");
            leave_flag = 1;
        } else {
            sprintf(message, ">>> %s has joined the chat\n", cli->username);
            printf("%s", message);
            broadcast_message(message, cli->uid);
        }
    }
    
    memset(buffer, 0, BUFFER_SIZE);
    
    while (!leave_flag) {
        int receive = recv(cli->socket_fd, buffer, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buffer) > 0) {
                str_trim_lf(buffer, BUFFER_SIZE);
                
                // Handle commands
                if (strcmp(buffer, "/users") == 0) {
                    send_active_users(cli->uid);
                } else if (strcmp(buffer, "/quit") == 0) {
                    sprintf(message, ">>> %s has left the chat\n", cli->username);
                    printf("%s", message);
                    broadcast_message(message, cli->uid);
                    leave_flag = 1;
                } else {
                    // Get timestamp
                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);
                    
                    snprintf(message, sizeof(message), "[%02d:%02d] %s: %s\n", 
                             t->tm_hour, t->tm_min, cli->username, buffer);
                    printf("%s", message);
                    broadcast_message(message, cli->uid);
                }
            }
        } else if (receive == 0) {
            sprintf(message, ">>> %s has left the chat\n", cli->username);
            printf("%s", message);
            broadcast_message(message, cli->uid);
            leave_flag = 1;
        } else {
            perror("Receive error");
            leave_flag = 1;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
    }
    
    close(cli->socket_fd);
    remove_client(cli->uid);
    free(cli);
    pthread_detach(pthread_self());
    
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int port = atoi(argv[1]);
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_t tid;
    
    // Socket settings
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }
    
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        return EXIT_FAILURE;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return EXIT_FAILURE;
    }
    
    // Listen
    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        return EXIT_FAILURE;
    }
    
    printf("=== CHAT SERVER ===\n");
    printf("Listening on port %d...\n\n", port);
    
    while (1) {
        socklen_t client_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }
        
        if (client_count >= MAX_CLIENTS) {
            printf("Max clients reached. Connection rejected.\n");
            close(client_sock);
            continue;
        }
        
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = client_addr;
        cli->socket_fd = client_sock;
        cli->uid = uid++;
        
        add_client(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);
        
        printf("New connection from ");
        print_client_addr(client_addr);
        printf("\n");
    }
    
    close(server_sock);
    return EXIT_SUCCESS;
}