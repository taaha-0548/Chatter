#include "../include/chatter.h"

// Mutex for cleanup operations
pthread_mutex_t cleanup_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_signal(int sig)
{
    if (sig == SIGINT)
    {
        running = 0;
    }
}

/*
 * TCP listener: Accepts incoming TCP connections and handles
 * chat requests, accepts, rejections, messages, and disconnects.
 */
void *tcp_listener(void *arg)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(TCP_PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("[System] TCP listener started on port %d...\n", TCP_PORT);

    // Make server socket non-blocking
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    while (running)
    {
        // Use select with timeout for non-blocking accept
        fd_set read_fds;
        struct timeval tv = {1, 0};
        FD_ZERO(&read_fds);

        pthread_mutex_lock(&cleanup_mutex);
        if (server_fd >= 0)
        {
            FD_SET(server_fd, &read_fds);
        }
        pthread_mutex_unlock(&cleanup_mutex);

        int sel = select(server_fd + 1, &read_fds, NULL, NULL, &tv);
        if (sel <= 0 || !running)
            continue;

        int client = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("[Error] Accept failed");
            }
            continue;
        }

        // Set non-blocking for client socket
        int flags = fcntl(client, F_GETFL, 0);
        fcntl(client, F_SETFL, flags | O_NONBLOCK);

        // Wait for initial message with timeout
        char buffer[BUFFER_SIZE];
        int message_received = 0;
        time_t start_time = time(NULL);

        while (running && !message_received && time(NULL) - start_time < 5)
        {
            fd_set client_fds;
            struct timeval client_tv = {1, 0};
            FD_ZERO(&client_fds);
            FD_SET(client, &client_fds);

            if (select(client + 1, &client_fds, NULL, NULL, &client_tv) > 0)
            {
                ssize_t n = read(client, buffer, BUFFER_SIZE - 1);
                if (n > 0)
                {
                    buffer[n] = '\0';
                    message_received = 1;
                }
            }
        }

        if (!message_received)
        {
            close(client);
            continue;
        }

        // Parse and handle message
        char *type_str = strtok(buffer, ":");
        char *sender = strtok(NULL, ":");
        char *content = strtok(NULL, "\0");

        if (!type_str || !sender || !content)
        {
            close(client);
            continue;
        }

        int type = atoi(type_str);
        switch (type)
        {
        case MSG_TYPE_REQUEST:
        {
            clear_input_line();
            printf("[Request] Chat request from %s: %s\n", sender, content);

            pthread_mutex_lock(&request_list_mutex);
            int found = 0;
            for (int i = 0; i < request_count; i++)
            {
                if (strcmp(pending_requests[i].name, sender) == 0)
                {
                    pending_requests[i].request_time = time(NULL);
                    pending_requests[i].active = 1;
                    found = 1;
                    break;
                }
            }
            if (!found && request_count < MAX_PENDING_REQUESTS)
            {
                safe_strcpy(pending_requests[request_count].name, sender, sizeof(pending_requests[request_count].name));
                pending_requests[request_count].request_time = time(NULL);
                pending_requests[request_count].active = 1;
                request_count++;
            }
            pthread_mutex_unlock(&request_list_mutex);

            printf("[System] Use /accept %s to accept or /reject %s to reject\n", sender, sender);
            print_prompt();
            close(client);
            break;
        }

        case MSG_TYPE_ACCEPT:
        {
            clear_input_line();
            printf("[System] %s accepted your chat request\n", sender);

            pthread_mutex_lock(&chat_sock_mutex);
            if (current_chat_sock >= 0)
            {
                close(client); // already chatting
            }
            else
            {
                current_chat_sock = client;
                safe_strcpy(current_chat_peer, sender, sizeof(current_chat_peer));
                printf("[System] Chat session started with %s\n", sender);
            }
            pthread_mutex_unlock(&chat_sock_mutex);

            print_prompt();

            while (running)
            {
                ssize_t n = read(client, buffer, BUFFER_SIZE - 1);
                if (n <= 0)
                    break;
                buffer[n] = '\0';

                char *msg_type_str = strtok(buffer, ":");
                char *msg_sender = strtok(NULL, ":");
                char *msg_content = strtok(NULL, "\0");

                if (!msg_type_str || !msg_sender || !msg_content)
                    continue;
                int msg_type = atoi(msg_type_str);

                if (msg_type == MSG_TYPE_CHAT)
                {
                    clear_input_line();
                    printf("%s: %s\n", msg_sender, msg_content);
                    print_prompt();
                }
                else if (msg_type == MSG_TYPE_DISCONNECT)
                {
                    clear_input_line();
                    printf("[System] %s ended the chat\n", msg_sender);
                    break;
                }
            }

            pthread_mutex_lock(&chat_sock_mutex);
            if (current_chat_sock == client)
            {
                close(current_chat_sock);
                current_chat_sock = -1;
                current_chat_peer[0] = '\0';
            }
            pthread_mutex_unlock(&chat_sock_mutex);

            print_prompt();
            break;
        }

        case MSG_TYPE_REJECT:
            clear_input_line();
            printf("[System] %s rejected your chat request\n", sender);
            print_prompt();
            close(client);
            break;

        case MSG_TYPE_CHAT:
            close(client); // should never happen as a standalone
            break;

        default:
            close(client);
            break;
        }
    }

    return NULL;
}

/*
 * TCP client connector for all request types
 */
int connect_to_peer(const char *target_name, int request_type)
{
    const char *target_ip = get_peer_ip(target_name);
    if (!target_ip)
    {
        printf("[System] Peer '%s' not found or offline.\n", target_name);
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in peer_addr = {0};
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, target_ip, &peer_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0)
    {
        perror("[Error] TCP Client: Connection failed");
        close(sock);
        return -1;
    }

    char message[BUFFER_SIZE];
    const char *content = "";

    switch (request_type)
    {
    case MSG_TYPE_REQUEST:
        content = "Would you like to chat?";
        break;
    case MSG_TYPE_ACCEPT:
        content = "I accept your chat request";
        break;
    case MSG_TYPE_REJECT:
        content = "I'm not available right now";
        break;
    case MSG_TYPE_DISCONNECT:
        content = "Goodbye";
        break;
    }

    snprintf(message, sizeof(message), "%d:%s:%s", request_type, my_username, content);
    send(sock, message, strlen(message), 0);
    return sock;
}

/*
 * Broadcast presence via UDP every 5 seconds
 */
void *udp_broadcast(void *arg)
{
    char *username = (char *)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;

    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in broadcast_addr = {0};
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(UDP_PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    char message[BUFFER_SIZE];
    while (running)
    {
        snprintf(message, sizeof(message), "NAME:%s:TIME:%ld", username, time(NULL));
        sendto(sock, message, strlen(message), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
        sleep(5);
    }

    close(sock);
    return NULL;
}

/*
 * Listen to UDP broadcasts and update peer list,
 * check for peer timeout and stale requests.
 */
void *udp_listener(void *arg)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    struct sockaddr_in sender_addr;
    socklen_t addrlen = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];

    while (running)
    {
        fd_set read_fds;
        struct timeval timeout = {1, 0};
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        int ready = select(sock + 1, &read_fds, NULL, NULL, &timeout);
        time_t now = time(NULL);

        // Check peer timeouts
        pthread_mutex_lock(&peer_list_mutex);
        for (int i = 0; i < peer_count; i++)
        {
            if (peer_list[i].active && now - peer_list[i].last_seen > PEER_TIMEOUT)
            {
                peer_list[i].active = 0;

                if (strcmp(peer_list[i].name, current_chat_peer) == 0)
                {
                    pthread_mutex_lock(&chat_sock_mutex);
                    if (current_chat_sock >= 0)
                    {
                        clear_input_line();
                        printf("[System] %s went offline. Chat ended.\n", current_chat_peer);
                        close(current_chat_sock);
                        current_chat_sock = -1;
                        current_chat_peer[0] = '\0';
                        print_prompt();
                    }
                    pthread_mutex_unlock(&chat_sock_mutex);
                }
                else
                {
                    clear_input_line();
                    printf("[System] Peer '%s' is now offline\n", peer_list[i].name);
                    print_prompt();
                }
            }
        }
        pthread_mutex_unlock(&peer_list_mutex);

        // Expire old pending requests
        pthread_mutex_lock(&request_list_mutex);
        for (int i = 0; i < request_count; i++)
        {
            if (pending_requests[i].active && now - pending_requests[i].request_time > REQUEST_EXPIRY_TIME)
            {
                pending_requests[i].active = 0;
            }
        }
        pthread_mutex_unlock(&request_list_mutex);

        if (ready > 0 && FD_ISSET(sock, &read_fds))
        {
            int len = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0,
                               (struct sockaddr *)&sender_addr, &addrlen);
            if (len < 0)
                continue;

            buffer[len] = '\0';
            if (strncmp(buffer, "NAME:", 5) != 0)
                continue;

            char *name_end = strstr(buffer + 5, ":TIME:");
            if (!name_end)
                continue;

            char received_name[50];
            int name_len = name_end - (buffer + 5);
            snprintf(received_name, sizeof(received_name), "%.*s", name_len, buffer + 5);
            if (strcmp(received_name, my_username) == 0)
                continue;

            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, sizeof(sender_ip));

            pthread_mutex_lock(&peer_list_mutex);
            int exists = 0;
            for (int i = 0; i < peer_count; i++)
            {
                if (strcmp(peer_list[i].name, received_name) == 0)
                {
                    peer_list[i].last_seen = now;
                    if (!peer_list[i].active)
                    {
                        peer_list[i].active = 1;
                        clear_input_line();
                        printf("[System] Peer '%s' is back online\n", received_name);
                        print_prompt();
                    }
                    exists = 1;
                    break;
                }
            }
            if (!exists && peer_count < MAX_PEERS)
            {
                safe_strcpy(peer_list[peer_count].name, received_name, sizeof(peer_list[peer_count].name));
                safe_strcpy(peer_list[peer_count].ip, sender_ip, sizeof(peer_list[peer_count].ip));
                peer_list[peer_count].last_seen = now;
                peer_list[peer_count].active = 1;
                peer_count++;

                clear_input_line();
                printf("[Discovery] New peer: %s\n", received_name);
                print_prompt();
            }
            pthread_mutex_unlock(&peer_list_mutex);
        }
    }

    close(sock);
    return NULL;
}

const char *get_peer_ip(const char *name)
{
    static char ip[INET_ADDRSTRLEN];
    pthread_mutex_lock(&peer_list_mutex);
    for (int i = 0; i < peer_count; i++)
    {
        if (strcmp(peer_list[i].name, name) == 0 && peer_list[i].active)
        {
            safe_strcpy(ip, peer_list[i].ip, INET_ADDRSTRLEN);
            pthread_mutex_unlock(&peer_list_mutex);
            return ip;
        }
    }
    pthread_mutex_unlock(&peer_list_mutex);
    return NULL;
}

/*
 * Clean up sockets and notify peer if in chat
 */
void cleanup_resources(void)
{
    running = 0;
    pthread_mutex_lock(&chat_sock_mutex);
    if (current_chat_sock >= 0)
    {
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "%d:%s:%s",
                 MSG_TYPE_DISCONNECT, my_username, "Ending chat session");
        send(current_chat_sock, message, strlen(message), 0);
        close(current_chat_sock);
        current_chat_sock = -1;
    }
    pthread_mutex_unlock(&chat_sock_mutex);

    if (server_fd >= 0)
    {
        close(server_fd);
        server_fd = -1;
    }
}
