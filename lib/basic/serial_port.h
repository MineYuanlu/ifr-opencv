#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#if defined(__KERNEL__)
#  error This library cannot be ran under kernel mode.
#endif

#include "ext_types.h"
#if !defined(__KERNEL__)
#  include <stdint.h>
#endif

#if __OS__ == __OS_Windows__
#  include <Windows.h>
   typedef HANDLE serial_port_t;
   typedef DWORD speed_t;
   typedef DWORD read_setting_t;
#elif __OS__ == __OS_Linux__
#  include <fcntl.h>
#  include <unistd.h>
#  include <termios.h>
   typedef int serial_port_t;
   typedef cc_t read_setting_t;
#else
#  error This serial port library is only available for __OS_Linux__ and __OS_Windows__.
#endif

#if __OS__ == __OS_Windows__
#  define B0        0
#  define B50       50
#  define B75       75
#  if defined(CBR_110)
#    define B110    CBR110
#  else
#    define B110    110
#  endif
#  define B134      134
#  define B150      150
#  define B200      200
#  if defined(CBR_300)
#    define B300    CBR_300
#  else
#    define B300    300
#  endif
#  if defined(CBR_600)
#    define B600    CBR_600
#  else
#    define B600    600
#  endif
#  if defined(CBR_1200)
#    define B1200   CBR_1200
#  else
#    define B1200   1200
#  endif
#  define B1800     1800
#  if defined(CBR_2400)
#    define B2400   CBR_2400
#  else
#    define B2400   2400
#  endif
#  if defined(CBR_4800)
#    define B4800   CBR_4800
#  else
#    define B4800   4800
#  endif
#  if defined(CBR_9600)
#    define B9600   CBR_9600
#  else
#    define B9600   CBR_9600
#  endif
#  if defined(CBR_14400)
#    define B14400  CBR_14400
#  else
#    define B14400  14400
#  endif
//Seriously my ubuntu doesn't has this one
#  if defined(CBR_19200)
#    define B19200  CBR_19200
#  else
#    define B19200  19200
#  endif
#  if defined(CBR_38400)
#    define B38400  CBR_38400
#  else
#    define B38400  38400
#  endif
#  if defined(CBR_57600)
#    define B57600  CBR_57600
#  else
#    define B57600  57600
#  endif
#  if defined(CBR_115200)
#    define B115200 CBR_115200
#  else
#    define B115200 115200
#  endif
#  if defined(CBR_128000)
#    define B128000 CBR_128000
#  else
#    define B128000 128000
#  endif
//And this one, not in ubuntu
#  define B230400   230400
#  if defined(CBR_256000)
#    define B256000 CBR_256000
#  else
#    define B256000 256000
#  endif
//And this one
#  define B460800   460800
#  define B500000   500000
#  define B576000   576000
#  define B921600   921600
#  define B1000000  1000000
#  define B1152000  1152000
#  define B1500000  1500000
#  define B2000000  2000000
#  define B2500000  2500000
#  define B3000000  3000000
#  define B3500000  3500000
#  define B4000000  4000000
//All CBR_ macros are found from MSDN
#elif __OS__ == __OS_Linux__
// static int __valid_baud_rates[]={ 0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400,
//    4800, 9600, 19200, 38400, 57600, 115200, 230400};
   static int __valid_baud_rates[] = {B0, B50, B75, B110, B134, B150, B200,
     B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600,
     B115200, B230400};
//Valid baud rates for ubuntu are listed in the manual page of tcsetattr(3) by `man tcsetattr`
#endif 
#define BAUD_RATE(__value__) B##__value__

#if __OS__ == __OS_Windows__
   typedef struct{
     struct _DCB __basic_state;
     struct _COMMTIMEOUTS __timeouts;
   }serial_port_attr_t;
#elif __OS__ == __OS_Linux__
   typedef struct termios serial_port_attr_t;
#endif

#if __OS__ == __OS_Windows__
#  define TCSANOW   0
#  define TCSADRAIN 1
#  define TCSAFLUSH 2
#endif

#if __OS__ == __OS_Linux__
#  if !defined(CS5)
#    pragma message ("Macro 'CS5' not defined for this __OS_Linux__. Using settings from x86.")
#    define CS5 0x00
#    define CS6 0x20
#    define CS7 0x40
#    define CS8 0x60
#  endif
#  define CS(__value__) CS##__value__
#endif

#define FLUSH_I 0x00
#define FLUSH_O 0x01

#define OPT_DATA_BITS_5 5
#define OPT_DATA_BITS_6 6
#define OPT_DATA_BITS_7 7
#define OPT_DATA_BITS_8 8
#define OPT_DATA_BITS(__value__) OPT_DATA_BITS_##__value__
//These values are ensured

#define OPT_PARITY_NONE              0
#define OPT_PARITY_ODD               1
#define OPT_PARITY_EVEN              2
#define OPT_PARITY_NOT_IMPLEMENTED 255

#define OPT_STOP_BITS_1                 1
#define OPT_STOP_BITS_2                 2
#define OPT_STOP_BITS_NOT_IMPLEMENTED 255
#define OPT_STOP_BITS(__value__) OPT_STOP_BITS_##__value__

#define SERIAL_PORT_OPT(__name__,__value__) OPT_##__name__(__value__)
#define OPT_PARITY(__value__) OPT_PARITY_##__value__
#define OPT_BAUD_RATE(__value__) B(__value__)

#define SUCCESS                 0
#define ERR_FAILED             -1 //Nothing special. For APIs without details
//ERR_FAILED must be -1
#define ERR_NOT_IMPLEMENTED    -2
#define ERR_STATE_MAIN         -3
#define ERR_STATE_SEC          -4
#define ERR_STATE_BOTH         -5
#define ERR_FLUSH              -6
#define ERR_INVALID_BAUD_RATE  -7
#define ERR_OPTIONAL_ACTIONS   -8
#define ERR_INVALID_OPT_ACTION -9
#define ERR_INVALID_DATA_BITS -10
#define ERR_INVALID_PARITY    -11
#define ERR_INVALID_STOP_BITS -12
#define ERR_FCNTL_FAILED      -13
//#define ERR_OPEN_FAILED       -13

__EXPORT_C_START

#if __OS__ == __OS_Windows__

FORCE_INLINE _Bool is_device_valid(serial_port_t device)
{
	return device != INVALID_HANDLE_VALUE;
}

int IOflush_serial_port(serial_port_t device, uint8_t flush_mode);

//Warning: The parameter 'device_config' behaves really different for different systems. 
//Do not call this or modify this unless you REALLY know what you are doing.
//Note that for ubuntu, baud_rate refers to the output speed.
int require_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device);

//Note that different operating systems may behave differently for optional actions
int apply_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device, int optional_actions);

FORCE_INLINE int get_serial_port_attr_baud_rate(serial_port_attr_t* attr_device, speed_t* baud_rate)
{
	*baud_rate = (attr_device->__basic_state).BaudRate;
	return SUCCESS;
}

FORCE_INLINE int set_serial_port_attr_baud_rate(serial_port_attr_t* attr_device, speed_t baud_rate)
{
	(attr_device->__basic_state).BaudRate = baud_rate;
	return SUCCESS;
}

FORCE_INLINE int get_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t* data_bits)
{
	*data_bits = (device_attr->__basic_state).ByteSize;
	return SUCCESS;
}

FORCE_INLINE int set_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t data_bits)
{
	if (data_bits < OPT_DATA_BITS(5) || data_bits > OPT_DATA_BITS(8))
		return ERR_INVALID_DATA_BITS;
//All call them data bits and you call them ByteSize???
//Go to hell Microsoft I really have had enough of your f**king damn strange APIs
	(device_attr->__basic_state).ByteSize = data_bits;
	return SUCCESS;
}

int get_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t* parity);

int set_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t parity);

int get_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t* stop_bits);

int set_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t stop_bits);

//Note that this function behaves really different under different operating systems.
//Be careful calling this
FORCE_INLINE int get_serial_port_attr_read_settings(serial_port_attr_t* device_attr, read_setting_t* arg1, read_setting_t* arg2, read_setting_t* arg3)
{
	*arg1 = (device_attr->__timeouts).ReadIntervalTimeout;
	*arg2 = (device_attr->__timeouts).ReadTotalTimeoutMultiplier;
	*arg3 = (device_attr->__timeouts).ReadTotalTimeoutConstant;
	return SUCCESS;
}

FORCE_INLINE int set_serial_port_attr_read_settings(serial_port_attr_t* device_attr, read_setting_t arg1, read_setting_t arg2, read_setting_t arg3)
{
	(device_attr->__timeouts).ReadIntervalTimeout = arg1;
	(device_attr->__timeouts).ReadTotalTimeoutMultiplier = arg2;
	(device_attr->__timeouts).ReadTotalTimeoutConstant = arg3;
	return SUCCESS;
}

FORCE_INLINE int standardize_serial_port_attr(serial_port_attr_t* device_attr)
{
	(device_attr->__timeouts).WriteTotalTimeoutMultiplier = 0;
	(device_attr->__timeouts).WriteTotalTimeoutConstant = 0;
	return SUCCESS;
}

FORCE_INLINE serial_port_t open_serial_port(char* device_str)
{
	return CreateFileA(device_str, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
}

FORCE_INLINE int close_serial_port(serial_port_t device)
{
	return -(!CloseHandle(device));
}

FORCE_INLINE int serial_port_read(serial_port_t device, uint8_t* dst, size_t size)
{
	DWORD size_received;
	if (!ReadFile(device, dst, size, &size_received, NULL))
		return ERR_FAILED;
	return size_received;
}

FORCE_INLINE int serial_port_write(serial_port_t device, uint8_t* src, size_t size)
{
	DWORD size_sent;
	if (!WriteFile(device, src, size, &size_sent, NULL))
		return ERR_FAILED;
	return size_sent;
}

#elif __OS__ == __OS_Linux__

FORCE_INLINE _Bool is_device_valid(serial_port_t device)
{
	return device >= 0;
}

int IOflush_serial_port(serial_port_t device, uint8_t flush_mode);

//Warning: The parameter 'device_config' behaves really different for different systems. 
//Do not call this or modify this unless you REALLY know what you are doing.
//Note that for ubuntu, baud_rate refers to the output speed.
FORCE_INLINE int require_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device)
{
	if (tcgetattr(device, attr_device) < 0)
		return ERR_STATE_MAIN;
}

//Note that different operating systems may behave differently for optional actions
FORCE_INLINE int apply_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device, int optional_actions)
{
	if (tcsetattr(device, optional_actions, attr_device) < 0)
		return ERR_STATE_MAIN;
}

FORCE_INLINE int get_serial_port_attr_baud_rate(serial_port_attr_t* attr_device, speed_t* baud_rate)
{
	*baud_rate = cfgetospeed(attr_device);
	return SUCCESS;
}

//Note that for ubuntu, valid baud rates are lmited. Use B(e) can fix some bugs.
int set_serial_port_attr_baud_rate(serial_port_attr_t* attr_device, speed_t baud_rate);

int get_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t* data_bits);

int set_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t data_bits);

int get_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t* parity);

int set_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t parity);

FORCE_INLINE int get_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t* stop_bits)
{
	if (device_attr->c_cflag & CSTOPB)
		*stop_bits = OPT_STOP_BITS_2;
	else
		*stop_bits = OPT_STOP_BITS_1;
	return SUCCESS;
}

int set_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t stop_bits);

//Note that this function behaves really different under different operating systems.
//Be careful calling this
FORCE_INLINE int get_serial_port_attr_read_settings(serial_port_attr_t* device_attr, read_setting_t* arg1, read_setting_t* arg2, read_setting_t* arg3)
{
	*arg1 = device_attr->c_cc[VMIN];
	*arg2 = device_attr->c_cc[VTIME];
	*arg3 = 0;
	return SUCCESS;
}

FORCE_INLINE int set_serial_port_attr_read_settings(serial_port_attr_t* device_attr, read_setting_t arg1, read_setting_t arg2, read_setting_t arg3)
{
	device_attr->c_cc[VMIN] = arg1;
	device_attr->c_cc[VTIME] = arg2;
	return SUCCESS;
}

FORCE_INLINE int standardize_serial_port_attr(serial_port_attr_t* device_attr)
{
	device_attr->c_oflag &= ~OPOST;
	device_attr->c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	return SUCCESS;
}

FORCE_INLINE serial_port_t open_serial_port(char* device_str)
{
	serial_port_t device = open(device_str, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if(fcntl(device, F_SETFL, 0) < 0)
		return ERR_FCNTL_FAILED;
	//Remember that for Linux, serial_port_t is an alias of int
	return device;
}

FORCE_INLINE int close_serial_port(serial_port_t device)
{
	return close(device);
}

FORCE_INLINE int serial_port_read(serial_port_t device, uint8_t* dst, size_t size)
{
	return read(device, dst, size);
}

FORCE_INLINE int serial_port_write(serial_port_t device, uint8_t* src, size_t size)
{
	return write(device, src, size);
}

#else

FORCE_INLINE _Bool is_device_valid(serial_port_t device)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int IOflush_serial_port(serial_port_t device, uint8_t flush_mode)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int require_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int apply_serial_port_attr(serial_port_t device, serial_port_attr_t* attr_device, int optional_actions)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int get_serial_port_attr_baud_rate(serial_port_attr_t* attr_device, speed_t* baud_rate)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int set_serial_port_attr_baud_rate(serial_port_attr_t* attr_device, speed_t baud_rate)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int get_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t* data_bits)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int set_serial_port_attr_data_bits(serial_port_attr_t* device_attr, uint8_t data_bits)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int get_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t* parity)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int set_serial_port_attr_parity(serial_port_attr_t* device_attr, uint8_t parity)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int get_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t* stop_bits)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int set_serial_port_attr_stop_bits(serial_port_attr_t* device_attr, uint8_t stop_bits)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int get_serial_port_attr_read_settings(serial_port_attr_t* device_attr, read_setting_t* arg1, read_setting_t* arg2, read_setting_t* arg3)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int set_serial_port_attr_read_settings(serial_port_attr_t* device_attr, read_setting_t arg1, read_setting_t arg2, read_setting_t arg3)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int standardize_serial_port_attr(serial_port_attr_t* device_attr)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE serial_port_t open_serial_port(char* device_str)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int close_serial_port(serial_port_t device)
{
	return ERR_NOT_IMPLEMENTED;
}

FORCE_INLINE int serial_port_read(serial_port_t device, uint8_t* dst, size_t size)
{
	return ERR_NOT_IMPEMENTED;
}

FORCE_INLINE int serial_port_write(serial_port_t device, uint8_t* src, size_t size)
{
	return ERR_NOT_IMPLEMENTED;
}

#endif

__EXPORT_C_END

#endif

