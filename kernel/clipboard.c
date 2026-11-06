/*
 * StellaOS - clipboard.c
 * System clipboard buffer implementation
 */

#include "types.h"
#include "clipboard.h"
#include "string.h"
#include "serial.h"

static char clipboard_buffer[CLIPBOARD_MAX_SIZE];
static size_t clipboard_length = 0;

void clipboard_init(void) {
    memset(clipboard_buffer, 0, CLIPBOARD_MAX_SIZE);
    clipboard_length = 0;
    serial_printf("Clipboard: Initialized system clipboard (%d bytes buffer)\n", CLIPBOARD_MAX_SIZE);
}

void clipboard_copy(const char *text, size_t len) {
    if (!text || len == 0) {
        clipboard_clear();
        return;
    }

    size_t to_copy = len;
    if (to_copy >= CLIPBOARD_MAX_SIZE) {
        to_copy = CLIPBOARD_MAX_SIZE - 1;
    }

    memcpy(clipboard_buffer, text, to_copy);
    clipboard_buffer[to_copy] = '\0';
    clipboard_length = to_copy;

    serial_printf("Clipboard: Copied %u bytes to clipboard\n", clipboard_length);
}

size_t clipboard_paste(char *buf, size_t max_len) {
    if (!buf || max_len == 0 || clipboard_length == 0) {
        if (buf && max_len > 0) {
            buf[0] = '\0';
        }
        return 0;
    }

    size_t to_paste = clipboard_length;
    if (to_paste >= max_len) {
        to_paste = max_len - 1;
    }

    memcpy(buf, clipboard_buffer, to_paste);
    buf[to_paste] = '\0';

    return to_paste;
}

bool clipboard_has_content(void) {
    return (clipboard_length > 0);
}

void clipboard_clear(void) {
    memset(clipboard_buffer, 0, CLIPBOARD_MAX_SIZE);
    clipboard_length = 0;
}

size_t clipboard_get_length(void) {
    return clipboard_length;
}
