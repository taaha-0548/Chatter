#ifndef CHATTER_H
#define CHATTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/select.h>
#include <ctype.h>

#define TCP_PORT            12345
#define UDP_PORT            54321
#define BUFFER_SIZE         1024
#define MAX_PEERS           100
#define MAX_PENDING_REQUESTS 10
#define PEER_TIMEOUT        15     // in seconds
#define REQUEST_EXPIRY_TIME 300    // 5 minutes

// Message Types
#define MSG_TYPE_CHAT        1
#define MSG_TYPE_REQUEST     2
#define MSG_TYPE_ACCEPT      3
#define MSG_TYPE_REJECT      4
#define MSG_TYPE_DISCONNECT  5

// Peer Representation
typedef struct {
    char name[50];
    char ip[INET_ADDRSTRLEN];
    time_t last_seen;
    int active;
} Peer;

// Chat Request
typedef struct {
    char name[50];
    time_t request_time;
    int active;
} ChatRequest;

// Shared Globals
extern Peer peer_list[MAX_PEERS];
extern int peer_count;
extern pthread_mutex_t peer_list_mutex;

extern ChatRequest pending_requests[MAX_PENDING_REQUESTS];
extern int request_count;
extern pthread_mutex_t request_list_mutex;

extern char my_username[50];
extern volatile int running;
extern int current_chat_sock;
extern char current_chat_peer[50];
extern pthread_mutex_t chat_sock_mutex;
extern int server_fd;

// Core Functional Areas

// --- Network ---
void *tcp_listener(void *arg);
void *udp_broadcast(void *arg);
void *udp_listener(void *arg);
int connect_to_peer(const char *target_name, int request_type);
const char *get_peer_ip(const char *name);
void cleanup_resources(void);
void handle_signal(int sig);

// --- Commands ---
void handle_command(const char *cmd);
void print_peer_list(void);
void print_pending_requests(void);

// --- UI ---
void clear_input_line(void);
void print_prompt(void);

// --- Utility ---
void safe_strcpy(char *dest, const char *src, size_t size);
void trim_string(char *str);

#endif
