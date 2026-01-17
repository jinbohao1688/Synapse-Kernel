#include <string.h>

static char* strtok_saveptr = NULL;

size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strcpy(char* dest, const char* src)
{
    char* d = dest;
    while ((*d++ = *src++) != '\0') {
    }
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    char* d = dest;
    while (n && (*d++ = *src++) != '\0') {
        n--;
    }
    while (n--) {
        *d++ = '\0';
    }
    return dest;
}

char* strcat(char* dest, const char* src)
{
    char* d = dest;
    while (*d) {
        d++;
    }
    while ((*d++ = *src++) != '\0') {
    }
    return dest;
}

char* strchr(const char* str, int c)
{
    while (*str != '\0') {
        if (*str == (char)c) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

char* strstr(const char* haystack, const char* needle)
{
    if (!*needle) {
        return (char*)haystack;
    }
    const char* h = haystack;
    while (*h) {
        const char* h_ptr = h;
        const char* n_ptr = needle;
        while (*n_ptr && *h_ptr == *n_ptr) {
            h_ptr++;
            n_ptr++;
        }
        if (!*n_ptr) {
            return (char*)h;
        }
        h++;
    }
    return NULL;
}

char* strtok(char* str, const char* delim)
{
    if (str != NULL) {
        strtok_saveptr = str;
    }
    
    if (strtok_saveptr == NULL) {
        return NULL;
    }
    
    char* token_start = strtok_saveptr;
    
    while (*strtok_saveptr != '\0') {
        const char* d = delim;
        while (*d != '\0') {
            if (*strtok_saveptr == *d) {
                *strtok_saveptr = '\0';
                strtok_saveptr++;
                return token_start;
            }
            d++;
        }
        strtok_saveptr++;
    }
    
    if (token_start == strtok_saveptr) {
        return NULL;
    }
    
    char* result = token_start;
    strtok_saveptr = NULL;
    return result;
}
