#ifndef SIMPLE_SERIAL_PORT_H
#define SIMPLE_SERIAL_PORT_H 1

#include "serial_port.h"

#define DEVICE_NAME_MAX_LENGTH 50

#define SUCCESS                        0
#define ERR_NAME_TOO_LONG             -1
#define ERR_OPEN_PORT_FAILED          -2
#define ERR_CANNOT_CLOSE_OPENED_PORT  -3
#define ERR_REQUIRE_ATTR_FAILED       -4
#define ERR_BAUD_RATE_FAILED          -5
#define ERR_DATA_BITS_FAILED          -6
#define ERR_PARITY_FAILED             -7
#define ERR_STOP_BITS_FAILED          -8
#define ERR_NO_OPENING_PORT           -9
#define ERR_CANNOT_APPLY_ATTR        -10
#define ERR_CANNOT_STANDARDIZE_ATTR  -11
#define ERR_FLUSH_ON_SETTING         -12
#define ERR_BASIC_ATTR_FAILED        -13
#define ERR_IOFLUSH                  -14
#define ERR_SET_NONBLOCK_READING     -15

//__EXPORT_C_START

int reset_error(void);

int get_last_error(void);

int enable_serial_port(char *device_str, _Bool force_open);

int disable_serial_port(void);

int get_serial_port_attributes(speed_t *baud_rate, uint8_t *data_bits, uint8_t *parity, uint8_t *stop_bits);

int
set_serial_port_attributes(speed_t baud_rate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits, _Bool force_set);

int set_serial_port_block_reading(void);

int set_serial_port_nonblock_reading(void);

int set_serial_port_to_default(_Bool force_set);

int __receive(uint8_t *dst, size_t size);

int __transmit(uint8_t *dst, size_t size);

//__EXPORT_C_END

#endif

