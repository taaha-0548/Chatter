#include "../include/chatter.h"

void handle_command(const char *cmd)
{
    if (strncmp(cmd, "/list", 5) == 0)
    {
        print_peer_list();
    }
    else if (strncmp(cmd, "/pending", 8) == 0)
    {
        print_pending_requests();
    }
    else if (strncmp(cmd, "/connect ", 9) == 0)
    {
        const char *target_name = cmd + 9;
        pthread_mutex_lock(&chat_sock_mutex);
        if (current_chat_sock >= 0)
        {
            printf("[System] You are already in a chat with %s. Disconnect first with /disconnect.\n",
                   current_chat_peer);
            pthread_mutex_unlock(&chat_sock_mutex);
            return;
        }
        pthread_mutex_unlock(&chat_sock_mutex);

        int sock = connect_to_peer(target_name, MSG_TYPE_REQUEST);
        if (sock >= 0)
        {
            close(sock);
        }
    }
    else if (strncmp(cmd, "/accept ", 8) == 0)
    {
        const char *target = cmd + 8;
        // Verify pending request
        int found = 0;
        pthread_mutex_lock(&request_list_mutex);
        for (int i = 0; i < request_count; i++)
        {
            if (pending_requests[i].active &&
                strcmp(pending_requests[i].name, target) == 0)
            {
                pending_requests[i].active = 0;
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&request_list_mutex);

        if (!found)
        {
            printf("[System] No pending request from %s\n", target);
            return;
        }

        int sock = connect_to_peer(target, MSG_TYPE_ACCEPT);
        if (sock >= 0)
        {
            pthread_mutex_lock(&chat_sock_mutex);
            current_chat_sock = sock;
            strncpy(current_chat_peer, target, sizeof(current_chat_peer) - 1);
            pthread_mutex_unlock(&chat_sock_mutex);
            printf("[System] Chat session established with %s\n", target);
        }
    }
    else if (strncmp(cmd, "/disconnect", 11) == 0)
    {
        pthread_mutex_lock(&chat_sock_mutex);
        if (current_chat_sock >= 0)
        {
            char temp_peer[50];
            strncpy(temp_peer, current_chat_peer, sizeof(temp_peer));

            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "%d:%s:%s",
                     MSG_TYPE_DISCONNECT, my_username, "Ending chat session");
            send(current_chat_sock, message, strlen(message), 0);

            close(current_chat_sock);
            current_chat_sock = -1;
            memset(current_chat_peer, 0, sizeof(current_chat_peer));
            printf("[System] Disconnected from %s\n", temp_peer);
        }
        else
        {
            printf("[System] Not connected to any peer\n");
        }
        pthread_mutex_unlock(&chat_sock_mutex);
    }
    else if (strncmp(cmd, "/quit", 5) == 0)
    {
        running = 0; // This will trigger cleanup in main()
    }
    else if (strlen(cmd) > 0)
    {
        printf("[System] Unknown command: %s\n", cmd);
        printf("[System] Available commands: /list, /pending, /connect <name>,\n");
        printf("  /accept <name>, /reject <name>, /disconnect, /quit\n");
    }
}

void print_peer_list(void)
{
    pthread_mutex_lock(&peer_list_mutex);
    printf("---- Online Peers ----\n");
    int count = 0;
    for (int i = 0; i < peer_count; i++)
    {
        if (peer_list[i].active)
        {
            printf(" - %s\n", peer_list[i].name);
            count++;
        }
    }
    if (count == 0)
        printf("No peers online.\n");
    printf("----------------------\n");
    pthread_mutex_unlock(&peer_list_mutex);
}

void print_pending_requests(void)
{
    pthread_mutex_lock(&request_list_mutex);
    printf("---- Pending Requests ----\n");
    int count = 0;
    for (int i = 0; i < request_count; i++)
    {
        if (pending_requests[i].active)
        {
            printf(" - %s (at %s)", pending_requests[i].name,
                   ctime(&pending_requests[i].request_time)); // includes newline
            count++;
        }
    }
    if (count == 0)
        printf("No pending requests.\n");
    printf("--------------------------\n");
    pthread_mutex_unlock(&request_list_mutex);
}
