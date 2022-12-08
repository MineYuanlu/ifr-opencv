#include "serial_port.h"

__EXPORT_C_START

#if __OS__ == __OS_Windows__

int IOflush_serial_port(serial_port_t device, uint8_t flush_mode)
{
	if ((flush_mode & FLUSH_I) && (flush_mode & FLUSH_O))
		return -(!PurgeComm(device, PURGE_RXCLEAR | PURGE_TXCLEAR));
	else if (flush_mode & FLUSH_I)
		return -(!PurgeComm(device, PURGE_RXCLEAR));
	else if (flush_mode & FLUSH_O)
		return -(!PurgeComm(device, PURGE_TXCLEAR));
	return SUCCESS;
}

int require_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device)
{
	_Bool status_main = GetCommState(device, &(attr_device->__basic_state));
	_Bool status_sec = GetCommTimeouts(device, &(attr_device->__timeouts));
	if (LIKELY(status_main && status_sec))
		return SUCCESS;
	else if (status_main && !status_sec)
		return ERR_STATE_SEC;
	else if (!status_main && status_sec)
		return ERR_STATE_MAIN;
	else return ERR_STATE_BOTH;
}

int apply_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device, int optional_actions)
{
	switch (optional_actions){
	case TCSANOW:
		if (!PurgeComm(device, PURGE_RXCLEAR | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_TXABORT))
			return ERR_OPTIONAL_ACTIONS;
		break;
	case TCSADRAIN:
		if (!PurgeComm(device, PURGE_TXCLEAR | PURGE_TXABORT))
			return ERR_OPTIONAL_ACTIONS;
		break;
	case TCSAFLUSH:
		break;
	default:
		return ERR_INVALID_OPT_ACTION;
	}
	_Bool status_main = SetCommState(device, &(attr_device->__basic_state));
	_Bool status_sec = SetCommTimeouts(device, &(attr_device->__timeouts));
	if (LIKELY(status_main && status_sec))
		return SUCCESS;
	if (status_main && !status_sec)
		return ERR_STATE_SEC;
	if (!status_main && status_sec)
		return ERR_STATE_MAIN;
	return ERR_STATE_BOTH;
}

int get_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t* parity)
{
	if (LIKELY(!(device_attr->__basic_state).fParity)) {
		*parity = OPT_PARITY_NONE;
		return SUCCESS;
	}
	switch ((device_attr->__basic_state).Parity) {
	case EVENPARITY:
		*parity = OPT_PARITY_EVEN;
		break;
	case ODDPARITY:
		*parity = OPT_PARITY_ODD;
		break;
	case NOPARITY:
		*parity = OPT_PARITY_NONE;
		break;
	default:
		*parity = OPT_PARITY_NOT_IMPLEMENTED;
		break;
	}
	return SUCCESS;
}

int set_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t parity)
{
	switch (parity) {
	case OPT_PARITY_NONE:
		(device_attr->__basic_state).Parity = NOPARITY;
		(device_attr->__basic_state).fParity = 0;
		break;
	case OPT_PARITY_ODD:
		(device_attr->__basic_state).Parity = ODDPARITY;
		(device_attr->__basic_state).fParity = 1;
		break;
	case OPT_PARITY_EVEN:
		(device_attr->__basic_state).Parity = EVENPARITY;
		(device_attr->__basic_state).fParity = 1;
		break;
	case OPT_PARITY_NOT_IMPLEMENTED:
		return ERR_NOT_IMPLEMENTED;
	default:
		return ERR_INVALID_PARITY;
	}
	return SUCCESS;
}

int get_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t* stop_bits)
{
	switch ((device_attr->__basic_state).StopBits) {
	case ONESTOPBIT:
		*stop_bits = OPT_STOP_BITS_1;
		break;
	case TWOSTOPBITS:
		*stop_bits = OPT_STOP_BITS_2;
		break;
	default:
		*stop_bits = OPT_STOP_BITS_NOT_IMPLEMENTED;
		break;
	}
	return SUCCESS;
}

int set_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t stop_bits)
{
	switch (stop_bits) {
	case OPT_STOP_BITS_1:
		(device_attr->__basic_state).StopBits = ONESTOPBIT;
		break;
	case OPT_STOP_BITS_2:
		(device_attr->__basic_state).StopBits = TWOSTOPBITS;
		break;
	case OPT_STOP_BITS_NOT_IMPLEMENTED:
		return ERR_NOT_IMPLEMENTED;
	default:
		return ERR_INVALID_STOP_BITS;
	}
	return SUCCESS;
}

#elif __OS__ == __OS_Linux__

int IOflush_serial_port(serial_port_t device, uint8_t flush_mode)
{
	//Execute bit operations multiple times should be faster than conditional jump
	if ((flush_mode & FLUSH_I) && (flush_mode & FLUSH_O))
		return tcflush(device, TCIOFLUSH);
	else if (flush_mode & FLUSH_I)
		return tcflush(device, TCIFLUSH);
	else if (flush_mode & FLUSH_O)
		return tcflush(device, TCOFLUSH);
    return SUCCESS;
}

int set_serial_port_attr_baud_rate(serial_port_attr_t* attr_device, speed_t baud_rate)
{
	size_t i = 0;
	_Bool is_arg_valid = 0;
	for (; i < sizeof(__valid_baud_rates); ++i) {
		if (__valid_baud_rates[i] == baud_rate) {
			is_arg_valid = 1;
			break;
		}
	}
	if (!is_arg_valid)return ERR_INVALID_BAUD_RATE;
	if (cfsetispeed(attr_device, baud_rate) < 0)
		return ERR_FAILED;
	return cfsetospeed(attr_device, baud_rate);
}

int get_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t* data_bits)
{
	switch (device_attr->c_cflag & CSIZE) {
	case CS5:
		*data_bits = OPT_DATA_BITS_5;
		break;
	case CS6:
		*data_bits = OPT_DATA_BITS_6;
		break;
	case CS7:
		*data_bits = OPT_DATA_BITS_7;
		break;
	case CS8:
		*data_bits = OPT_DATA_BITS_8;
		break;
	default:
		return ERR_FAILED;
	}
	return SUCCESS;
}

int set_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t data_bits)
{
	if (data_bits < OPT_DATA_BITS(5) || data_bits > OPT_DATA_BITS(8))
		return ERR_INVALID_DATA_BITS;
	device_attr->c_cflag &= ~CSIZE;
	switch (data_bits) {
	case OPT_DATA_BITS_5:
		device_attr->c_cflag |= CS5;
		break;
	case OPT_DATA_BITS_6:
		device_attr->c_cflag |= CS6;
		break;
	case OPT_DATA_BITS_7:
		device_attr->c_cflag |= CS7;
		break;
	case OPT_DATA_BITS_8:
		device_attr->c_cflag |= CS8;
		break;
	}
	return SUCCESS;
}

int get_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t* parity)
{
	if (UNLIKELY(device_attr->c_cflag & PARENB)) {
		if (device_attr->c_cflag & PARODD)
			*parity = OPT_PARITY_ODD;
		else
			*parity = OPT_PARITY_EVEN;
	} else
		*parity = OPT_PARITY_NONE;
	return SUCCESS;
}

int set_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t parity)
{
	switch (parity) {
	case OPT_PARITY_NONE:
		device_attr->c_cflag &= ~PARENB;
		device_attr->c_iflag &= ~INPCK;
		break;
	case OPT_PARITY_ODD:
		device_attr->c_cflag |= PARENB;
		device_attr->c_cflag |= PARODD;
		device_attr->c_iflag |= INPCK;
		break;
	case OPT_PARITY_EVEN:
		device_attr->c_cflag |= PARENB;
		device_attr->c_cflag &= ~PARODD;
		device_attr->c_iflag |= INPCK;
		break;
	case OPT_PARITY_NOT_IMPLEMENTED:
		return ERR_NOT_IMPLEMENTED;
	default:
		return ERR_INVALID_PARITY;
	}
	return SUCCESS;
}

int set_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t stop_bits)
{
	switch (stop_bits) {
	case OPT_STOP_BITS_1:
		device_attr->c_cflag &= ~CSTOPB;
		break;
	case OPT_STOP_BITS_2:
		device_attr->c_cflag |= CSTOPB;
		break;
	default:
		return ERR_INVALID_STOP_BITS;
	}
	return SUCCESS;
}

#else

#endif

__EXPORT_C_END

