/*
 * StellaOS - string.h
 * Memory and string manipulation utilities
 */

#ifndef STELLA_STRING_H
#define STELLA_STRING_H

#include "types.h"

/* Memory operations */
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

/* String operations */
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
char *strchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

/* Conversion functions */
int atoi(const char *s);
void itoa(int value, char *str, int base);
void utoa(uint32_t value, char *str, int base);

#endif /* STELLA_STRING_H */
