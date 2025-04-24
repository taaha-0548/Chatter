#include "../include/chatter.h"

// Global State Definitions
Peer peer_list[MAX_PEERS];
int peer_count = 0;
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

ChatRequest pending_requests[MAX_PENDING_REQUESTS];
int request_count = 0;
pthread_mutex_t request_list_mutex = PTHREAD_MUTEX_INITIALIZER;

char my_username[50] = {0};

int current_chat_sock = -1;
char current_chat_peer[50] = {0};
pthread_mutex_t chat_sock_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_fd = -1;
volatile int running = 1;

int main()
{
    signal(SIGINT, handle_signal);

    printf("Welcome to Chatter 2.1\n");
    printf("Enter your display name: ");
    fflush(stdout);

    if (!fgets(my_username, sizeof(my_username), stdin))
    {
        fprintf(stderr, "[Error] Failed to read username.\n");
        exit(EXIT_FAILURE);
    }

    size_t len = strlen(my_username);
    if (len > 0 && my_username[len - 1] == '\n')
        my_username[len - 1] = '\0';
    trim_string(my_username);

    if (strlen(my_username) == 0)
    {
        fprintf(stderr, "[Error] Username cannot be empty.\n");
        exit(EXIT_FAILURE);
    }

    pthread_t tid_tcp, tid_udp_broadcast, tid_udp_listener;
    pthread_create(&tid_tcp, NULL, tcp_listener, NULL);
    pthread_create(&tid_udp_broadcast, NULL, udp_broadcast, my_username);
    pthread_create(&tid_udp_listener, NULL, udp_listener, NULL);

    printf("\n[System] Chat application started. You are '%s'.\n", my_username);
    printf("[System] Available commands:\n");
    printf("  /list                - Show available peers\n");
    printf("  /pending             - Show pending chat requests\n");
    printf("  /connect <name>      - Send a chat request to a peer\n");
    printf("  /accept <name>       - Accept a chat request\n");
    printf("  /reject <name>       - Reject a chat request\n");
    printf("  /disconnect          - Disconnect from current chat\n");
    printf("  /quit                - Exit the program\n\n");

    char input_buffer[BUFFER_SIZE];
    while (running)
    {
        print_prompt();
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL)
            break;

        size_t len = strlen(input_buffer);
        if (len > 0 && input_buffer[len - 1] == '\n')
            input_buffer[len - 1] = '\0';

        if (input_buffer[0] == '/')
        {
            handle_command(input_buffer);
        }
        else if (strlen(input_buffer) > 0)
        {
            pthread_mutex_lock(&chat_sock_mutex);
            if (current_chat_sock >= 0)
            {
                char message[BUFFER_SIZE];
                snprintf(message, sizeof(message), "%d:%.63s:%.954s",
                         MSG_TYPE_CHAT, my_username, input_buffer);

                if (send(current_chat_sock, message, strlen(message), 0) < 0)
                {
                    perror("[Error] Sending message");
                    printf("[System] Connection to %s lost.\n", current_chat_peer);
                    close(current_chat_sock);
                    current_chat_sock = -1;
                    memset(current_chat_peer, 0, sizeof(current_chat_peer));
                }
                else
                {
                    clear_input_line();
                    printf("%s: %s\n", my_username, input_buffer);
                }
            }
            else
            {
                printf("[System] Not connected to any peer. Use /connect <name> to connect.\n");
            }
            pthread_mutex_unlock(&chat_sock_mutex);
        }
    }

    // Add proper cleanup sequence
    cleanup_resources();

    // Wait for threads to finish
    pthread_join(tid_tcp, NULL);
    pthread_join(tid_udp_broadcast, NULL);
    pthread_join(tid_udp_listener, NULL);

    printf("[System] Chatter has been shut down. Goodbye!\n");
    return 0;
}
