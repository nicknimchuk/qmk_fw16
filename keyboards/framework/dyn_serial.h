// Copyright 2022 Framework Computer
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// The factory writes the serial number into the last 4K block of the 1MB flash
#define FLASH_OFFSET 0x10000000
#define LAST_4K_BLOCK 0xff000
#define SERIALNUM_LEN 18

#define FALLBACK_SERIAL_NUMBER "FRAKDKEN0100000000"
_Static_assert(sizeof(FALLBACK_SERIAL_NUMBER) == SERIALNUM_LEN + 1, "Fallback serial number must be same length");

// ASCII copy of the serial number, empty if the factory data is invalid
extern char ascii_serialnum[SERIALNUM_LEN + 1];
