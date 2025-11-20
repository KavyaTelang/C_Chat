#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#define BUFFER_SIZE 2048
#define USERNAME_SIZE 32

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char username[USERNAME_SIZE];

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void catch_ctrl_c(int sig) {
    flag = 1;
}

void *send_msg_handler(void *arg) {
    char message[BUFFER_SIZE] = {};
    
    while (1) {
        str_overwrite_stdout();
        fgets(message, BUFFER_SIZE, stdin);
        str_trim_lf(message, BUFFER_SIZE);
        
        if (strlen(message) == 0) {
            continue;
        }
        
        if (strcmp(message, "/quit") == 0) {
            break;
        }
        
        send(sockfd, message, strlen(message), 0);
        
        memset(message, 0, BUFFER_SIZE);
    }
    
    catch_ctrl_c(2);
    return NULL;
}

void *recv_msg_handler(void *arg) {
    char message[BUFFER_SIZE] = {};
    
    while (1) {
        int receive = recv(sockfd, message, BUFFER_SIZE, 0);
        if (receive > 0) {
            printf("%s", message);
            str_overwrite_stdout();
        } else if (receive == 0) {
            break;
        }
        memset(message, 0, BUFFER_SIZE);
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char *ip = argv[1];
    int port = atoi(argv[2]);
    
    signal(SIGINT, catch_ctrl_c);
    
    printf("Enter your username: ");
    fgets(username, USERNAME_SIZE, stdin);
    str_trim_lf(username, USERNAME_SIZE);
    
    if (strlen(username) < 2 || strlen(username) >= USERNAME_SIZE - 1) {
        printf("Username must be between 2 and 30 characters.\n");
        return EXIT_FAILURE;
    }
    
    struct sockaddr_in server_addr;
    
    // Socket settings
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return EXIT_FAILURE;
    }
    
    // Send username
    send(sockfd, username, USERNAME_SIZE, 0);
    
    printf("\n=== WELCOME TO THE CHAT ===\n");
    printf("Commands:\n");
    printf("  /users - Show active users\n");
    printf("  /quit  - Leave the chat\n");
    printf("===========================\n\n");
    
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, send_msg_handler, NULL) != 0) {
        perror("Thread creation failed");
        return EXIT_FAILURE;
    }
    
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, recv_msg_handler, NULL) != 0) {
        perror("Thread creation failed");
        return EXIT_FAILURE;
    }
    
    while (1) {
        if (flag) {
            printf("\nGoodbye!\n");
            break;
        }
    }
    
    close(sockfd);
    
    return EXIT_SUCCESS;
}