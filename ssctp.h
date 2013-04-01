//
// SimpleSerialCommandTransferProtocol
//	
//	Copyright (c) 2013 赵迤晨 (Zhao, Yichen) <max.zhaoyichen@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/**
 * @file
 * Sample SSCTP Implementation Headers
 * 
 * Offers two buffers: unsigned int and float. Adjustable buffer sizes
 *
 * @needs stdint.h
 * @needs stdbool.h
 * @needs stddef.h
 * @author Zhao Yichen <max.zhaoyichen@gmail.com>
 * @copyright Copyright (c) 2013 赵迤晨 (Zhao, Yichen) <max.zhaoyichen@gmail.com>
 * @license MIT License
 * @version 0.1
 */
#ifndef SSCTP_H
#define SSCTP_H

//Options
/**
 * Enables float point buffers
 */
// /* Remove the leading // to disable
#define SSCTP_FLOAT
//*/
/**
 * float data type
 */
typedef float    ssctp_float_t;

/**
 * int data type
 */
typedef uint16_t ssctp_int_t;

/**
 * Size of the float buffer
 */
#define SSCTP_FLOAT_BUFFER_SIZE 10

/**
 * Size of the int buffer
 */
#define SSCTP_INT_BUFFER_SIZE   10

/**
 * Size of the stack
 */
#define SSCTP_STACK_SIZE        10
/**
 * Internal used shift buffer type. Please keep it the size of the maximum size of data types defined above
 */
typedef uint32_t ssctp_shift_buffer_t;

/**
 * Int datatype with size equal to the float datatype defined above
 * Warning: This must be defined exactly equal to the size of float
 */
typedef uint32_t ssctp_float_equiv_t;


/**
 * Enable sending errors
 */
// /* Remove the leading // to disable
#define SSCTP_ERROR_ENABLE
//*/

/**
 * Enables inline functions (if not defined, functions are defined as static)
 */
 /* Remove the leading // to disable
#define SSCTP_INLINE_AVAILABLE  
//*/

/**
 * Output is using RetractableSerialBuffer (rsb)
 */
// /* Remove the leading // to disable
#define SSCTP_RSB
//*/

#ifdef SSCTP_RSB
 #include <rsb.h>
 extern rsb_buffer buffer;
#endif


#define SSCTP_RSB_BUFFER_PTR &buffer

//System Data Type Definitions
typedef struct {
	ssctp_int_t*   int_val;
	#ifdef SSCTP_FLOAT
	ssctp_float_t* float_val;
	#endif
} ssctp_stack_t;

//System Command Definitions
#define SSCTP_CMD_HALT     0
#define SSCTP_CMD_BYTE1    1
#define SSCTP_CMD_BYTE2    2
#define SSCTP_CMD_BYTE3    3
#define SSCTP_CMD_BYTE4    4
#define SSCTP_CMD_BYTE5    5
#define SSCTP_CMD_FLOAT    6
#define SSCTP_CMD_DOUBLE   7
#define SSCTP_CMD_LONGLONG 8
#define SSCTP_CMD_CLEAR    9
#define SSCTP_CMD_VERIFY   10
#define SSCTP_CMD_EXTCMD   11
#define SSCTP_CMD_ASYNC    12
#define SSCTP_CMD_ASYNCEND 13
#define SSCTP_CMD_DATA(cmd)      (cmd >= 0 && cmd <= 8)
#define SSCTP_CMD_ERROR(cmd)     (cmd >= 118 && cmd <= 127)
#define SSCTP_CMD_CLEARXOR(cmd)  (!(SSCTP_CMD_DATA(cmd) || SSCTP_CMD_ERROR(cmd)))
#define SSCTP_CMD_DATA_SIZE(cmd) ((cmd >= 1 && cmd <= 5) ? cmd : ((cmd == 6) ? 5 : ((cmd >= 7 && cmd <= 8) ? 10 : -1)))

//System Errors
#define SSCTP_ERR_INVALIDCMD  127
#define SSCTP_ERR_UNKNOWNCMD  126
#define SSCTP_ERR_DATACORRUPT 125
#define SSCTP_ERR_OVERFLOW    124
#define SSCTP_ERR_TRUNCATED   123
#define SSCTP_ERR_OVERRUN     122
#define SSCTP_ERR_OK          118

//Parameters
#ifdef SSCTP_INLINE_AVAILABLE
 #define SSCTP_PRIVATE_METHOD inline
#else
 #define SSCTP_PRIVATE_METHOD static
#endif

/**
 * Initialize SSCTP
 */
void ssctp_init(void);

/**
 * Send an int
 * @param length [description]
 * @param data   [description]
 */
void ssctp_send_int(int length, ssctp_int_t data);

#define ssctp_send_int7(data)  ssctp_send_int(1, data)

#define ssctp_send_int14(data) ssctp_send_int(2, data)

#define ssctp_send_int21(data) ssctp_send_int(3, data)

#ifdef SSCTP_FLOAT
/**
 * Send a float
 * @param data [description]
 */
void ssctp_send_float(ssctp_float_t data);
#endif

/**
 * Send a command
 * @param command [description]
 */
void ssctp_send_command(int command);

/**
 * Send an error (same as command)
 * @param  err_code [description]
 * @return          [description]
 */
#ifdef SSCTP_ERROR_ENABLE
#define ssctp_send_error(err_code) ssctp_send_command(err_code)
#else
#define ssctp_send_error(err_code)
#endif

/**
 * Reply ok
 * @param  SSCTP_ERR_OK [description]
 * @return              [description]
 */
#define ssctp_send_ok() ssctp_send_error(SSCTP_ERR_OK)


/**
 * Preserve the current buffer during command call
 */
void ssctp_buffer_preserve(void);

/**
 * Clear the current buffer immediately
 */
void ssctp_buffer_clear(void);

/**
 * Input a byte to SSCTP
 * @param byte [description]
 */
void ssctp_receive_byte(uint8_t byte);

#ifndef SSCTP_RSB
/**
 * Byte output handler prototype
 * @param byte [description]
 */
void ssctp_send_byte_handler(uint8_t byte);
#endif

/**
 * Command handler prototype
 * @param commandNo [description]
 * @param argn      [description]
 * @param argv      [description]
 */
void ssctp_command_handler(int commandNo, int argn, ssctp_stack_t* argv);

#endif