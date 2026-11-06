/*
 * StellaOS - installer.h
 * Interactive OS Installer Wizard
 * Copyright (c) 2024 StellaOS Project
 */

#ifndef STELLA_INSTALLER_H
#define STELLA_INSTALLER_H

#include "types.h"

void installer_init(void);
void installer_run(void);  // runs the installer flow
bool installer_is_first_boot(void);

#endif /* STELLA_INSTALLER_H */
