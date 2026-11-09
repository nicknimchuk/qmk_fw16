# VIA support uses raw HID, don't need to enable it extra.
# Only when disabling VIA but still wanting to use RAW, need to enable it here.
VIA_ENABLE = yes
RAW_ENABLE = no

# Custom matrix scanning code via ADC
CUSTOM_MATRIX = lite
ANALOG_DRIVER_REQUIRED = yes
SRC += matrix.c

SRC += dyn_serial.c factory.c

# Let the idle thread sleep the CPU (WFI) when there's nothing to do
OPT_DEFS += -DCORTEX_ENABLE_WFI_IDLE=TRUE
