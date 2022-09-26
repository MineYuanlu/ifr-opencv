#include "simple_serial_port.h"
#include <string.h>

#if (__OS__ != __OS_Windows__) && (__OS__ != __OS_Linux__)
#  error This library is only available for Windows and Linux.
#endif

static char device_name[DEVICE_NAME_MAX_LENGTH + 1];
static serial_port_t cnt_device;
static _Bool is_device_opened = 0;
static serial_port_attr_t cnt_device_attr;
static int serial_port_err = 0;

__EXPORT_C_START

int reset_error(void) {
    serial_port_err = 0;
    return SUCCESS;
}

int get_last_error(void) {
    return serial_port_err;
}

int enable_serial_port(char *device_str, _Bool force_open) {
    if (is_device_opened) {
        serial_port_err = close_serial_port(cnt_device);
        if (serial_port_err < 0 && !force_open)
            return ERR_CANNOT_CLOSE_OPENED_PORT;
    }
    if (strlen(device_str) > DEVICE_NAME_MAX_LENGTH)
        return serial_port_err = ERR_NAME_TOO_LONG;
    cnt_device = open_serial_port(device_str);
    if (!is_device_valid(cnt_device))
        return serial_port_err = ERR_OPEN_PORT_FAILED;
    strcpy(device_name, device_str);
    // OK C4996. That's why I hate microsoft. Go to hell you suck.
    if ((serial_port_err = require_serial_port_attr(cnt_device, &cnt_device_attr)) < 0)
        return ERR_REQUIRE_ATTR_FAILED;
    is_device_opened = 1;
    return SUCCESS;
}

int disable_serial_port(void) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
    if (!is_device_opened)
        return ERR_NO_OPENING_PORT;
    serial_port_err = close_serial_port(cnt_device);
    if (serial_port_err == 0)
        is_device_opened = 0;
    return serial_port_err;
}

int get_serial_port_attributes(speed_t *baud_rate, uint8_t *data_bits, uint8_t *parity, uint8_t *stop_bits) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
    serial_port_err = get_serial_port_attr_baud_rate(&cnt_device_attr, baud_rate);
    if (serial_port_err < 0)
        return ERR_BAUD_RATE_FAILED;
    serial_port_err = get_serial_port_attr_data_bits(&cnt_device_attr, data_bits);
    if (serial_port_err < 0)
        return ERR_DATA_BITS_FAILED;
    serial_port_err = get_serial_port_attr_parity(&cnt_device_attr, parity);
    if (serial_port_err < 0)
        return ERR_PARITY_FAILED;
    serial_port_err = get_serial_port_attr_stop_bits(&cnt_device_attr, stop_bits);
    if (serial_port_err < 0)
        return ERR_STOP_BITS_FAILED;
    return SUCCESS;
}

int
set_serial_port_attributes(speed_t baud_rate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits, _Bool force_set) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
    serial_port_err = IOflush_serial_port(cnt_device, FLUSH_I | FLUSH_O);
    if (serial_port_err < 0 && !force_set)
        return ERR_FLUSH_ON_SETTING;
    serial_port_err = standardize_serial_port_attr(&cnt_device_attr);
    if (serial_port_err < 0 && !force_set)
        return ERR_CANNOT_STANDARDIZE_ATTR;
    serial_port_err = set_serial_port_attr_baud_rate(&cnt_device_attr, baud_rate);
    if (serial_port_err < 0)
        return ERR_BAUD_RATE_FAILED;
    serial_port_err = set_serial_port_attr_data_bits(&cnt_device_attr, data_bits);
    if (serial_port_err < 0)
        return ERR_DATA_BITS_FAILED;
    serial_port_err = set_serial_port_attr_parity(&cnt_device_attr, parity);
    if (serial_port_err < 0)
        return ERR_PARITY_FAILED;
    serial_port_err = set_serial_port_attr_stop_bits(&cnt_device_attr, stop_bits);
    if (serial_port_err < 0)
        return ERR_STOP_BITS_FAILED;
    serial_port_err = apply_serial_port_attr(cnt_device, &cnt_device_attr, TCSANOW);
    if (serial_port_err < 0)
        return ERR_CANNOT_APPLY_ATTR;
    return SUCCESS;
}

int set_serial_port_block_reading(void) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
#if __OS__ == __OS_Windows__
    return serial_port_err = set_serial_port_attr_read_settings(&cnt_device_attr, 0, 0, 0);
#else
    return serial_port_err = set_serial_port_attr_read_settings(&cnt_device_attr, 0, 1, 0);
#endif
}

int set_serial_port_nonblock_reading(void) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
#if __OS__ == __OS_Windows__
    return serial_port_err = set_serial_port_attr_read_settings(&cnt_device_attr, 100, 100, 10);
#else
    return serial_port_err = set_serial_port_attr_read_settings(&cnt_device_attr, 0, 0, 0);
#endif
}

int set_serial_port_to_default(_Bool force_set) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
    //In the called function, 'serial_port_err' is set. No need to overwrite
    if (set_serial_port_attributes(BAUD_RATE(115200), OPT_DATA_BITS_8, OPT_PARITY_NONE, OPT_STOP_BITS_1, force_set) < 0)
        return ERR_BASIC_ATTR_FAILED;
    if (serial_port_err = set_serial_port_nonblock_reading() < 0)
        return ERR_SET_NONBLOCK_READING;
    if (serial_port_err = IOflush_serial_port(cnt_device, FLUSH_I | FLUSH_O) < 0)
        return ERR_IOFLUSH;
    if (serial_port_err = apply_serial_port_attr(cnt_device, &cnt_device_attr, TCSANOW) < 0)
        return ERR_CANNOT_APPLY_ATTR;
    return SUCCESS;
}

int __receive(uint8_t *dst, size_t size) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
    return serial_port_err = serial_port_read(cnt_device, dst, size);
}

int __transmit(uint8_t *dst, size_t size) {
    if (!is_device_opened)
        return serial_port_err = ERR_NO_OPENING_PORT;
    return serial_port_err = serial_port_write(cnt_device, dst, size);
}

__EXPORT_C_END

