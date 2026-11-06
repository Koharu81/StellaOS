/*
 * StellaOS - string.c
 * Implementation of standard string and memory utility functions
 */

#include "types.h"
#include "string.h"

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    uint8_t val = (uint8_t)c;
    for (size_t i = 0; i < n; i++) {
        p[i] = val;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    if (!dest || !src || n == 0 || dest == src) {
        return dest;
    }

    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d < s) {
        /* Copy forwards */
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        /* Copy backwards to prevent overwriting overlapping regions */
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
    }
    return 0;
}

size_t strlen(const char *s) {
    if (!s) return 0;
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (int)(uint8_t)(*s1) - (int)(uint8_t)(*s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (int)(uint8_t)s1[i] - (int)(uint8_t)s2[i];
        }
        if (s1[i] == '\0') {
            break;
        }
    }
    return 0;
}

char *strcpy(char *dest, const char *src) {
    if (!dest) return NULL;
    if (!src) {
        *dest = '\0';
        return dest;
    }

    char *d = dest;
    while ((*d++ = *src++) != '\0') {
        /* Copy loop */
    }
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    if (!dest) return NULL;

    size_t i;
    for (i = 0; i < n && src && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    if (!dest) return NULL;
    if (!src) return dest;

    char *d = dest;
    while (*d != '\0') {
        d++;
    }
    while ((*d++ = *src++) != '\0') {
        /* Append loop */
    }
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    if (!dest) return NULL;
    if (!src || n == 0) return dest;

    char *d = dest;
    while (*d != '\0') {
        d++;
    }
    size_t i = 0;
    while (i < n && *src != '\0') {
        *d++ = *src++;
        i++;
    }
    *d = '\0';
    return dest;
}

char *strchr(const char *s, int c) {
    if (!s) return NULL;
    char target = (char)c;

    while (*s != '\0') {
        if (*s == target) {
            return (char *)s;
        }
        s++;
    }
    if (target == '\0') {
        return (char *)s;
    }
    return NULL;
}

int atoi(const char *s) {
    if (!s) return 0;

    /* Skip leading whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\v' || *s == '\f') {
        s++;
    }

    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    int result = 0;
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    return sign * result;
}

void itoa(int value, char *str, int base) {
    if (!str || base < 2 || base > 36) return;

    char *ptr = str;
    uint32_t uval;
    bool is_negative = false;

    if (base == 10 && value < 0) {
        is_negative = true;
        uval = (uint32_t)(-(uint32_t)value);
    } else {
        uval = (uint32_t)value;
    }

    if (uval == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return;
    }

    char buf[36];
    int i = 0;
    while (uval > 0) {
        uint32_t rem = uval % (uint32_t)base;
        buf[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        uval /= (uint32_t)base;
    }

    if (is_negative) {
        *ptr++ = '-';
    }

    while (i > 0) {
        *ptr++ = buf[--i];
    }
    *ptr = '\0';
}

void utoa(uint32_t value, char *str, int base) {
    if (!str || base < 2 || base > 36) return;

    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    char buf[36];
    int i = 0;
    while (value > 0) {
        uint32_t rem = value % (uint32_t)base;
        buf[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        value /= (uint32_t)base;
    }

    char *ptr = str;
    while (i > 0) {
        *ptr++ = buf[--i];
    }
    *ptr = '\0';
}


char *strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    if (*needle == '\0') return (char *)haystack;

    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (*n == '\0') return (char *)haystack;
        haystack++;
    }
    return NULL;
}
