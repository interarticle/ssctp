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
 * Sample SSCTP Implementation
 * 
 * Offers two buffers: unsigned int and float. Adjustable buffer sizes
 *
 * @author Zhao Yichen <max.zhaoyichen@gmail.com>
 * @copyright Copyright (c) 2013 赵迤晨 (Zhao, Yichen) <max.zhaoyichen@gmail.com>
 * @license MIT License
 * @version 0.1
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ssctp.h"

/* Receiver Code */
static uint8_t              data_mode                = 0;
static ssctp_shift_buffer_t in_shift_buffer          = 0;
static int                  in_shift_buffer_received = 0;
static int                  in_shift_buffer_size     = 0;
static uint8_t              in_xor                   = 0;

static ssctp_int_t        in_int_buffer[SSCTP_INT_BUFFER_SIZE];
static int                in_int_buffer_position   = 0;
#ifdef SSCTP_FLOAT
static ssctp_float_t      in_float_buffer[SSCTP_FLOAT_BUFFER_SIZE];
static int                in_float_buffer_position = 0;
#endif
static ssctp_stack_t      in_stack[SSCTP_STACK_SIZE];
static int                in_stack_position        = 0;

static _Bool              clear_buffer             = true;

#ifdef SSCTP_FLOAT
	#define SSCTP_BUFFER_FULL() (in_int_buffer_position >= SSCTP_INT_BUFFER_SIZE || \
									in_float_buffer_position >= SSCTP_FLOAT_BUFFER_SIZE || in_stack_position >= SSCTP_STACK_SIZE)
#else
	#define SSCTP_BUFFER_FULL() (in_int_buffer_position >= SSCTP_INT_BUFFER_SIZE || in_stack_position >= SSCTP_STACK_SIZE)
#endif
#ifdef SSCTP_FLOAT
#define SSCTP_BUFFER_CLEAR() {in_int_buffer_position = 0; in_float_buffer_position = 0; in_stack_position = 0;}
#else
#define SSCTP_BUFFER_CLEAR() {in_int_buffer_position = 0; in_stack_position = 0;}
#endif

void ssctp_init() {

}

SSCTP_PRIVATE_METHOD void ssctp_process_command(int command, int argc, ssctp_stack_t* argv) {
	if (command == SSCTP_CMD_EXTCMD) {
		//Note: New definition, the last integer is the command. This is to simplify command sending from the MCU
		if (argc <= 0 || argv[argc - 1].int_val == NULL)  {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		ssctp_process_command(*argv[argc - 1].int_val, argc - 1, argv);
	} else if (command == SSCTP_CMD_VERIFY) {
		if (argc <= 0 || argv[0].int_val == NULL)  {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		if (in_xor != *argv[0].int_val)
		{
			ssctp_send_error(SSCTP_ERR_DATACORRUPT);
		} else {
			ssctp_send_ok();
		}
	} else if (command == SSCTP_CMD_CLEAR) {
		//Do nothing, buffer is cleared by default
	} else if (command == SSCTP_CMD_ASYNC || command == SSCTP_CMD_ASYNCEND) {
		//Not implemented
		ssctp_send_error(SSCTP_ERR_UNKNOWNCMD);
	} else {
		ssctp_command_handler(command, argc, argv);
	}
}

SSCTP_PRIVATE_METHOD void ssctp_receive_command(int command) {
	if (SSCTP_CMD_ERROR(command)) {
		//Ignore all errors
		//return is mandatory to prevent unexpected clearing of in_shift_buffer
		return;
	}
	//Any command other than error should clear input shift buffer
	in_shift_buffer          = 0;
	in_shift_buffer_received = 0;
	if (SSCTP_CMD_DATA(command)) {
		if (SSCTP_BUFFER_FULL() && command != SSCTP_CMD_HALT) {
			command = SSCTP_CMD_HALT;
			ssctp_send_error(SSCTP_ERR_OVERFLOW);
		}
		if (command == SSCTP_CMD_BYTE4 || command == SSCTP_CMD_BYTE5 || command == SSCTP_CMD_DOUBLE || command == SSCTP_CMD_LONGLONG){
			command = SSCTP_CMD_HALT;
			ssctp_send_error(SSCTP_ERR_UNKNOWNCMD);
		}
		#ifndef SSCTP_FLOAT
		if (command == SSCTP_CMD_FLOAT){
			command = SSCTP_CMD_HALT;
			ssctp_send_error(SSCTP_ERR_UNKNOWNCMD);
		}
		#endif
		data_mode = command;
		in_shift_buffer_size = SSCTP_CMD_DATA_SIZE(command);
	} else {
		clear_buffer = true;
		ssctp_process_command(command, in_stack_position, in_stack);
		if (clear_buffer)
			SSCTP_BUFFER_CLEAR();
	}

	if (SSCTP_CMD_CLEARXOR(command))
		in_xor = 0;
}

SSCTP_PRIVATE_METHOD void ssctp_receive_data(uint8_t data) {
	if (data_mode != SSCTP_CMD_HALT && in_shift_buffer_size > 0) {
		if (SSCTP_BUFFER_FULL())
		{
			//This will automatically halt the receiver and send overflow errors
			ssctp_receive_command(data_mode);
			return;
		}
		in_xor ^= data;
		if (in_shift_buffer_received < in_shift_buffer_size)
		{
			in_shift_buffer |= (((ssctp_shift_buffer_t) data) << (7 * in_shift_buffer_received++));
		}
		if (in_shift_buffer_received >= in_shift_buffer_size) {
			if (data_mode >= 1 && data_mode <= 5) {
				in_int_buffer[in_int_buffer_position] = (ssctp_int_t) in_shift_buffer;
				in_stack[in_stack_position].int_val = &in_int_buffer[in_int_buffer_position++];
				#ifdef SSCTP_FLOAT
				in_stack[in_stack_position++].float_val = NULL;
				#endif
			}
			#ifdef SSCTP_FLOAT
			if (data_mode == SSCTP_CMD_FLOAT) {
				ssctp_float_equiv_t float_buffer;
				float_buffer = in_shift_buffer;
				in_float_buffer[in_float_buffer_position] = *((ssctp_float_t*) ((intptr_t) &float_buffer));
				in_stack[in_stack_position].float_val = &in_float_buffer[in_float_buffer_position++];
				in_stack[in_stack_position++].int_val = NULL;
			}
			#endif

			//Clear in_shift_buffer
			in_shift_buffer_received = 0;
			in_shift_buffer          = 0;
		}
	}
}


void ssctp_buffer_preserve(void) {
	clear_buffer = false;
}

/**
 * Clear the current buffer immediately
 */
void ssctp_buffer_clear(void) {
	SSCTP_BUFFER_CLEAR();
}

void ssctp_receive_byte(uint8_t byte) {
	if ((byte & (1 << 7)) != 0) {
		ssctp_receive_command(byte & ~(1 << 7));
	} else {
		ssctp_receive_data(byte);
	}
}

/* /End Receiver Code */

/* Transmitter Code */

#ifdef SSCTP_RSB
static _Bool out_buffer_dirty   = false;
static _Bool out_buffer_corrupt = false;
#endif
static int out_data_mode        = 0;

void ssctp_send_command(int command) {
	command = command & ((1 << 14) - 1); //Limit command size to 14 bits
	if (SSCTP_CMD_DATA(command)) {
		out_data_mode = command;
	} else if (!SSCTP_CMD_ERROR(command)) {
		//Makes sure that the next batch of data is preceded by a datatype declaration
		out_data_mode = 0;
	}
	if (command > 127) {
		ssctp_send_int14(command);
		command = SSCTP_CMD_EXTCMD;
	}
	#ifndef SSCTP_RSB
	ssctp_send_byte_handler((1 << 7) | command);
	#else
	if (out_buffer_corrupt && !SSCTP_CMD_ERROR(command)) { //Corrupt data buffer, discard all commands/data
		if (!SSCTP_CMD_DATA(command)) { //Standard command, reset corrupt flag
			out_buffer_corrupt = false;
		}
		return;
	}

	if (!rsb_is_free(SSCTP_RSB_BUFFER_PTR)) {
		//Buffer overrun
		if (!SSCTP_CMD_ERROR(command)) { //Errors are silently discarded
			if (out_buffer_dirty) { //Sending data upon full, retract and corrupt
				rsb_restore_marker(SSCTP_RSB_BUFFER_PTR);
				out_buffer_dirty = false;
				rsb_clear_marker(SSCTP_RSB_BUFFER_PTR);
				ssctp_send_command(SSCTP_CMD_CLEAR);
				ssctp_send_error(SSCTP_ERR_OVERRUN);

			} //If out_buffer_corrupt, then it shouldn't be dirty, so data commands are silently discarded
			if (SSCTP_CMD_DATA(command)) {
				out_buffer_corrupt = true;
			}
		}
	} else {
		if (!SSCTP_CMD_ERROR(command) && !SSCTP_CMD_DATA(command)) { //Sending command, clear dirty
			out_buffer_dirty = false;
			rsb_clear_marker(SSCTP_RSB_BUFFER_PTR);
		} else if (SSCTP_CMD_DATA(command)) {
			if (!out_buffer_dirty) {
				rsb_set_marker(SSCTP_RSB_BUFFER_PTR);
			}
			out_buffer_dirty = true;
		}
		rsb_write(SSCTP_RSB_BUFFER_PTR, (1 << 7) | command);
	}
	#endif
}

SSCTP_PRIVATE_METHOD void ssctp_send_data(uint8_t data) {
	//data &= ~(1 << 7); //Clears high bit just to be safe;
	#ifndef SSCTP_RSB
	ssctp_send_byte_handler(data);
	#else
	if (out_buffer_corrupt) {//Discard all data if corrupt
		return;
	}
	if (!rsb_is_free(SSCTP_RSB_BUFFER_PTR)) {
		if (out_buffer_dirty) { //Sending data upon full, retract and corrupt
			rsb_restore_marker(SSCTP_RSB_BUFFER_PTR);
			out_buffer_dirty = false;
			rsb_clear_marker(SSCTP_RSB_BUFFER_PTR);
			ssctp_send_command(SSCTP_CMD_CLEAR);
			ssctp_send_error(SSCTP_ERR_OVERRUN);

		} //If out_buffer_corrupt, then it shouldn't be dirty, so data commands are silently discarded
		out_buffer_corrupt = true;
	} else {
		if (!out_buffer_dirty) {
			rsb_set_marker(SSCTP_RSB_BUFFER_PTR);
		}
		out_buffer_dirty = true;
		rsb_write(SSCTP_RSB_BUFFER_PTR, data);
	}
	#endif
}


void ssctp_send_int(int length, ssctp_int_t data) {
	if (length < 1 || length > 5) return; //Prevent invalid modes
	if (out_data_mode != length)
		ssctp_send_command(length);
	while (length -- > 0){
		ssctp_send_data(data & 127);
		data >>= 7;
	}
}


void ssctp_send_float(ssctp_float_t data) {
	static ssctp_float_equiv_t shift_buffer;
	int length = SSCTP_CMD_DATA_SIZE(SSCTP_CMD_FLOAT);
	if (out_data_mode != SSCTP_CMD_FLOAT)
		ssctp_send_command(SSCTP_CMD_FLOAT);
	shift_buffer = *((ssctp_float_equiv_t*) ((intptr_t) &data));

	while (length -- > 0) {
		ssctp_send_data(shift_buffer & 127);
		shift_buffer >>= 7;
	}
}