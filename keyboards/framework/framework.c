// Copyright 2022 Framework Computer
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "framework.h"
#include "os_detection.h"
#include "usb_main.h"

bool is_suspended = false;

#if defined(RGB_MATRIX_ENABLE) && defined(IS31FL3743A_CUSTOM_PWM_FREQ)
// Undocumented IS31FL3743A registers carried over from Framework's fork:
// enter test mode, raise the LED PWM frequency from the default 29 kHz to
// 32.25 kHz, then leave test mode.
#    define IS31FL3743A_REG_TEST 0xE0
#    define IS31FL3743A_REG_CUSTOM_PWM 0xE2
#    define IS31FL3743A_CUSTOM_PWM_32K 0xE0

static void set_led_pwm_frequency(void) {
    for (uint8_t i = 0; i < IS31FL3743A_DRIVER_COUNT; i++) {
        is31fl3743a_select_page(i, IS31FL3743A_COMMAND_FUNCTION);
        is31fl3743a_write_register(i, IS31FL3743A_REG_TEST, 0x01);
        is31fl3743a_write_register(i, IS31FL3743A_REG_CUSTOM_PWM, IS31FL3743A_CUSTOM_PWM_32K);
        is31fl3743a_write_register(i, IS31FL3743A_REG_TEST, 0x00);
    }
}
#endif

void keyboard_post_init_kb(void) {
#if defined(RGB_MATRIX_ENABLE) && defined(IS31FL3743A_CUSTOM_PWM_FREQ)
    // The LED drivers were initialized by rgb_matrix_init() before this runs.
    set_led_pwm_frequency();
#endif

    // Enable debug output
    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;

    keyboard_post_init_user();
}

/**
 * Hook into early keyboard initialization
 */
void keyboard_pre_init_kb(void) {
    // Mark boot as done.
    // Before this, when holding down both alt keys QSPI_SS is pulled low to put
    // the RP2040 in bootloader mode during reset.
    gpio_set_pin_output(BOOT_DONE_GPIO);
    gpio_write_pin_low(BOOT_DONE_GPIO);

    //// TODO: Do we ever need to disable it to save power?
    gpio_set_pin_output(MUX_ENABLE_GPIO);
    gpio_write_pin_high(MUX_ENABLE_GPIO);

    gpio_set_pin_output(IS31FL3743A_ENABLE_GPIO);
#if defined(RGB_MATRIX_ENABLE)
    gpio_write_pin_high(IS31FL3743A_ENABLE_GPIO);
#else
    gpio_write_pin_low(IS31FL3743A_ENABLE_GPIO);
#endif
    gpio_set_pin_input(SLEEP_GPIO);

    keyboard_pre_init_user();
}

/**
 * Called by QMK when the keyboard suspends. While the host keeps the keyboard
 * suspended, QMK's suspend loop calls this on every pass.
 */
void suspend_power_down_kb(void) {
    // QMK's suspend loop doesn't process queued USB events. Framework found
    // that this races with selective suspend on the RP2040 (their commit
    // 385ed1a291), so handle pending events on every pass.
    usb_event_queue_task();

    is_suspended = true;

#if defined(RGB_MATRIX_ENABLE) && !defined(NO_SUSPEND_POWER_DOWN)
    // Put the LED controllers into hardware shutdown
    gpio_write_pin_low(IS31FL3743A_ENABLE_GPIO);
#endif

    suspend_power_down_user();
}

/**
 * Called by QMK when the keyboard wakes up from suspend
 */
void suspend_wakeup_init_kb(void) {
    is_suspended = false;

#if defined(RGB_MATRIX_ENABLE) && !defined(NO_SUSPEND_POWER_DOWN)
    gpio_write_pin_high(IS31FL3743A_ENABLE_GPIO);
#endif

    suspend_wakeup_init_user();
}

// If in BIOS mode, no matter what the keys have been remapped to, always send them as the F keys
bool bios_mode = true;
void set_bios_mode(bool val) {
    bios_mode = val;

#ifdef BIOS_HOTKEY_DEBUG
    if (bios_mode) {
        // Red
#    if defined(RGB_MATRIX_ENABLE)
        rgb_matrix_sethsv_noeeprom(0, 0xFF, 0xFF);
        rgb_matrix_mode_noeeprom(1);
#    endif
        gpio_write_pin(GP24, 1);
    } else {
        // White
#    if defined(RGB_MATRIX_ENABLE)
        rgb_matrix_sethsv_noeeprom(0, 0, 0xFF);
        rgb_matrix_mode_noeeprom(1);
#    endif
        gpio_write_pin(GP24, 0);
    }
#endif
}

/**
 * Turn BIOS mode off only once the host has been identified as Windows or
 * Linux. In UEFI (and while the OS is still unknown) the F-row keys always
 * send F1-F12 so firmware hotkeys work.
 */
static void update_bios_mode(void) {
    os_variant_t os = detected_host_os();
    set_bios_mode(!(os == OS_LINUX || os == OS_WINDOWS));
}

// Matrix positions of the keys that print F1-F12 on the keycaps
static const struct {
    uint8_t  row;
    uint8_t  col;
    uint16_t keycode;
} bios_hotkeys[] = {
    {3, 5, KC_F1}, {2, 5, KC_F2},  {6, 4, KC_F3},  {3, 4, KC_F4},  {4, 10, KC_F5},  {3, 10, KC_F6},
    {2, 10, KC_F7}, {1, 15, KC_F8}, {3, 11, KC_F9}, {4, 8, KC_F10}, {6, 8, KC_F11}, {3, 13, KC_F12},
};

static bool handle_bios_hotkeys(uint16_t keycode, keyrecord_t *record) {
    // Not in bios mode, no special handling, handle as normal
    if (!bios_mode) return true;

    for (uint8_t i = 0; i < ARRAY_SIZE(bios_hotkeys); i++) {
        if (record->event.key.row == bios_hotkeys[i].row && record->event.key.col == bios_hotkeys[i].col) {
            if (record->event.pressed) {
                register_code(bios_hotkeys[i].keycode);
            } else {
                unregister_code(bios_hotkeys[i].keycode);
            }
            return false;
        }
    }
    return true;
}

#ifdef RADIO_CONTROLS_ENABLE
static void send_airplane_mode_toggle(void) {
    host_driver_t *driver = host_get_driver();
    if (!driver || !driver->send_extra) return;

    // The radio button is a relative input: each report with the bit set
    // toggles airplane mode, so only send on key press.
    report_extra_t report = {
        .report_id = REPORT_ID_RADIO,
        .usage     = 1,
    };
    driver->send_extra(&report);
}
#endif

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // Framework's fork called process_record_user() first and ignored its
    // return value. Here the laptop-specific handling runs first, so it still
    // applies when a keymap consumes a key, and the keymap's return value is
    // honored.
    update_bios_mode();

    // Enable NKRO only in OS. Can cause issues with some pre-boot environments
    // Framework UEFI is ok, but I think grub can't handle it properly
    if (!bios_mode) {
        keymap_config.nkro = 1;
    }

    if (!handle_bios_hotkeys(keycode, record)) {
        return false;
    }

    if (!process_record_user(keycode, record)) {
        return false;
    }

    switch (keycode) {
#ifdef RGB_MATRIX_ENABLE
        // Allow next/prev effect keys to turn RGB on
        case QK_RGB_MATRIX_MODE_NEXT:
        case QK_RGB_MATRIX_MODE_PREVIOUS:
        case QK_UNDERGLOW_MODE_NEXT:
        case QK_UNDERGLOW_MODE_PREVIOUS:
            if (!rgb_matrix_is_enabled()) {
                rgb_matrix_enable();
            }
            return true;
        // Implement step brightness for RGB backlight
        case BL_STEP:
            if (record->event.pressed) {
                uint8_t h = rgb_matrix_get_hue();
                uint8_t s = rgb_matrix_get_sat();
                uint8_t v = rgb_matrix_get_val();
                uint8_t new_v;
                switch (v) {
                    default: // Default when user set to a different level
                    case 0:
                        new_v = 85;
                        break;
                    case 85:
                        new_v = 170;
                        break;
                    case 170:
                        new_v = 255;
                        break;
                    case 255:
                        new_v = 0;
                        break;
                }
                rgb_matrix_sethsv(h, s, new_v);
            }
            return true;
#endif
        case KC_SCRN:
            // Simulate press WIN+P
            // Works (at least) on Windows and GNOME
            if (record->event.pressed) {
                register_code(KC_LGUI);
                register_code(KC_P);
            } else {
                unregister_code(KC_P);
                unregister_code(KC_LGUI);
            }
            return false;
        case KC_CPLT:
            // Copilot key: simulate press WIN+SHIFT+F23
            if (record->event.pressed) {
                register_code(KC_LGUI);
                register_code(KC_LSFT);
                register_code(KC_F23);
            } else {
                unregister_code(KC_F23);
                unregister_code(KC_LSFT);
                unregister_code(KC_LGUI);
            }
            return false;
        case KC_AIRPLANE_MODE:
#ifdef RADIO_CONTROLS_ENABLE
            if (record->event.pressed) {
                send_airplane_mode_toggle();
            }
#endif
            return false;
        default:
            return true;
    }
}

#ifdef BIOS_HOTKEY_DEBUG
// Ignore capslock updates in order to manually control it
bool led_update_kb(led_t led_state) {
    return false;
}
#endif
