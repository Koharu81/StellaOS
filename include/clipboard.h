/*
 * StellaOS - clipboard.h
 * System clipboard buffer interface
 */

#ifndef STELLA_CLIPBOARD_H
#define STELLA_CLIPBOARD_H

#include "types.h"

#define CLIPBOARD_MAX_SIZE 4096

/* Clipboard Public API */
void clipboard_init(void);
void clipboard_copy(const char *text, size_t len);
size_t clipboard_paste(char *buf, size_t max_len);
bool clipboard_has_content(void);
void clipboard_clear(void);
size_t clipboard_get_length(void);

#endif /* STELLA_CLIPBOARD_H */
