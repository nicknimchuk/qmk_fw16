// Copyright 2022 Framework Computer
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define MATRIX_COLS 16
#define MATRIX_ROWS 8

// Two IS31FL3743A LED controllers: 48 + 49 LEDs
#define IS31FL3743A_I2C_ADDRESS_1 IS31FL3743A_I2C_ADDRESS_GND_GND
#define IS31FL3743A_I2C_ADDRESS_2 IS31FL3743A_I2C_ADDRESS_GND_VCC
#define RGB_MATRIX_LED_COUNT (48 + 49)

// Settings carried over from Framework's fork of the older ISSI driver
// Enable only the first 9 SW and disable software shutdown
#define IS31FL3743A_CONFIGURATION 0x21
#define IS31FL3743A_GLOBAL_CURRENT 104
// The old driver ran the first controller as sync master, the second as slave
#define IS31FL3743A_SYNC_1 IS31FL3743A_SYNC_MASTER
#define IS31FL3743A_SYNC_2 IS31FL3743A_SYNC_SLAVE
