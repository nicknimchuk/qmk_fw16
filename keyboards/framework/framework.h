// Copyright 2022 Framework Computer
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

// Keyboard-level keycodes live in the QK_KB range so keymaps and userspace
// can keep using SAFE_RANGE for their own keycodes.
enum framework_keycodes {
    // Change screen modes (e.g. enable external screen), sends WIN+P
    KC_SCRN = QK_KB_0,
    FN_LOCK,
    // Copilot key, sends WIN+SHIFT+F23
    KC_CPLT,
    // Toggle airplane mode (HID Wireless Radio Button)
    KC_AIRPLANE_MODE,
};
#define KC_AIRP KC_AIRPLANE_MODE

extern bool bios_mode;
extern bool is_suspended;

// Uncomment this to show hotkey mode with capslock LED and backlight
// hotkey mode on:  capslock ON,  RGB red
// hotkey mode off: capslock OFF, RGB white
//#define BIOS_HOTKEY_DEBUG
void set_bios_mode(bool val);

#define SLEEP_GPIO GP0
#define MUX_ENABLE_GPIO GP4
#define BOOT_DONE_GPIO GP5
// Pin SDB to enable the RGB controller
#ifdef PICO_FL16
// 22 only on RPi Pico because it doesn't have GP29
#    define IS31FL3743A_ENABLE_GPIO GP22
#else
#    define IS31FL3743A_ENABLE_GPIO GP29
#endif
