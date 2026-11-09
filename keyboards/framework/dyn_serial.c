// Copyright 2022 Framework Computer
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "usb_descriptor.h"
#include "dyn_serial.h"

_Static_assert(SERIAL_NUMBER_LENGTH == SERIALNUM_LEN, "SERIAL_NUMBER_LENGTH must match the factory serial length");
// Wear leveling must never touch the factory serial number block
_Static_assert(WEAR_LEVELING_RP2040_FLASH_BASE + WEAR_LEVELING_BACKING_SIZE <= LAST_4K_BLOCK, "Wear leveling overlaps the serial number block");

// Defined in tmk_core/protocol/usb_descriptor.c when SERIAL_NUMBER_USE_HARDWARE_ID is TRUE
extern uint8_t SerialNumberString[];

char ascii_serialnum[SERIALNUM_LEN + 1];

struct __attribute__((__packed__)) serialnum_raw {
    uint8_t sn_rev;
    uint8_t serialnum[SERIALNUM_LEN];
    uint8_t crc32[4];
};

static uint32_t crc32b(const uint8_t *message, size_t msg_len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < msg_len; i++) {
        crc = crc ^ message[i];
        for (int j = 0; j < 8; j++) {
            uint32_t mask = -(crc & 1);
            crc           = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}

/**
 * Read the factory serial number from memory-mapped flash into
 * ascii_serialnum. Returns false if it's missing or corrupt.
 */
static bool read_factory_serial(void) {
    const uint8_t              *serialnum_ptr = (const uint8_t *)(FLASH_OFFSET + LAST_4K_BLOCK);
    const struct serialnum_raw *serialnum_raw = (const struct serialnum_raw *)serialnum_ptr;

    // Only rev 1 supported
    if (serialnum_raw->sn_rev != 1) {
        return false;
    }

    uint32_t crc32 = crc32b(serialnum_ptr, SERIALNUM_LEN + 1);
    // Make sure it's little-endian
    uint32_t crc32_read = ((uint32_t)serialnum_raw->crc32[3] << 24) + ((uint32_t)serialnum_raw->crc32[2] << 16) + ((uint32_t)serialnum_raw->crc32[1] << 8) + serialnum_raw->crc32[0];
    if (crc32_read != crc32) {
        return false;
    }

    memcpy(ascii_serialnum, serialnum_raw->serialnum, SERIALNUM_LEN);
    ascii_serialnum[SERIALNUM_LEN] = '\0';
    return true;
}

/**
 * Overrides QMK's hardware-ID serial number with the factory serial number,
 * falling back to a fixed one like Framework's firmware does.
 */
void set_serial_number_descriptor(void) {
    static bool is_set = false;
    if (is_set) {
        return;
    }
    is_set = true;

    const char *serial = read_factory_serial() ? ascii_serialnum : FALLBACK_SERIAL_NUMBER;

    USB_Descriptor_String_t *desc = (USB_Descriptor_String_t *)SerialNumberString;

    // Convert to UCS-2, which is equivalent to UTF-16, if the input is ASCII
    uint8_t length = 0;
    for (; length < SERIALNUM_LEN; length++) {
        if ((uint8_t)serial[length] > 128) {
            dprintf("Serial number character %d is not valid ASCII.", serial[length]);
            break;
        }
        desc->UnicodeString[length] = serial[length];
    }

    desc->Header.Size = sizeof(USB_Descriptor_Header_t) + (length * sizeof(wchar_t));
    desc->Header.Type = DTYPE_String;
}
