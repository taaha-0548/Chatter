#include "../include/chatter.h"

/*
 * Safely copies a string with null-termination and size protection.
 */
void safe_strcpy(char *dest, const char *src, size_t size) {
    if (!dest || !src || size == 0) return;

    size_t i;
    for (i = 0; i < size - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/*
 * Trims leading and trailing whitespace from a string in-place.
 */
void trim_string(char *str) {
    if (!str) return;

    // Trim leading
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return;

    // Copy back into original pointer if needed
    char *start = str;
    char *end = str + strlen(str) - 1;

    // Trim trailing
    while (end > start && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    // Move trimmed string to the beginning
    memmove(str - (str - start), start, strlen(start) + 1);
}
