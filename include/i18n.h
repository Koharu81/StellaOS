/*
 * StellaOS - i18n.h
 * Internationalization subsystem supporting 30+ languages
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_I18N_H
#define STELLA_I18N_H

#include "types.h"

typedef enum {
    STR_WELCOME, STR_SHUTDOWN, STR_RESTART, STR_SETTINGS,
    STR_TERMINAL, STR_FILES, STR_ABOUT, STR_LANGUAGE,
    STR_CLOSE, STR_MINIMIZE, STR_MAXIMIZE, STR_SEARCH,
    STR_HELP, STR_VERSION, STR_MEMORY_USAGE, STR_CPU_USAGE,
    STR_DISK_USAGE, STR_NETWORK, STR_DATETIME, STR_INSTALL,
    STR_INSTALLING, STR_COMPLETE, STR_ERROR, STR_OK,
    STR_CANCEL, STR_YES, STR_NO, STR_NEXT, STR_BACK,
    STR_FINISH, STR_SELECT_LANG, STR_SELECT_TIMEZONE,
    STR_USERNAME, STR_PASSWORD, STR_HOSTNAME,
    STR_WELCOME_INSTALLER, STR_INSTALL_COMPLETE,
    STR_COUNT
} string_id_t;

typedef struct {
    const char *code;               /* e.g. "en", "ko", "ja" */
    const char *name;               /* Romanized ASCII name e.g. "English", "Hangugeo", "Nihongo" */
    const char *strings[STR_COUNT]; /* Translations for each string_id_t */
} language_t;

void i18n_init(void);
void i18n_set_language(const char *code);
const char *i18n_get(string_id_t id);
const char *i18n_get_lang_name(void);
const char *i18n_get_lang_code(void);
int i18n_get_language_count(void);
const language_t *i18n_get_language(int index);

#endif /* STELLA_I18N_H */
