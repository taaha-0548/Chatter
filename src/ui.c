#include "../include/chatter.h"

/*
 * Clears the current input line visually from the terminal.
 * Useful when printing something mid-input (like incoming message).
 */
void clear_input_line(void) {
    printf("\r\033[K");
    fflush(stdout);
}

/*
 * Displays the prompt depending on context.
 * Shows peer chat prompt or generic command prompt.
 */
void print_prompt(void) {
    pthread_mutex_lock(&chat_sock_mutex);
    if (strlen(current_chat_peer) > 0) {
        printf("[%s <-> %s] %s: ", my_username, current_chat_peer, my_username);
    } else {
        printf("Command: ");
    }
    fflush(stdout);
    pthread_mutex_unlock(&chat_sock_mutex);
}
