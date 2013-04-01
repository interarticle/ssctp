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
 * Sample SSCTP Test
 * 
 * @author Zhao Yichen <max.zhaoyichen@gmail.com>
 * @copyright Copyright (c) 2013 赵迤晨 (Zhao, Yichen) <max.zhaoyichen@gmail.com>
 * @license MIT License
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <rsb.h>
#include "ssctp.h"
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>

rsb_buffer buffer;
unsigned char data_buffer[1000];

DWORD WINAPI ReaderThread(LPVOID dummy) {
	while (true) {
		int result = rsb_read(&buffer);
		uint8_t b = result;
		if (result >= 0) {
			fwrite(&b, 1, 1, stdout);
			fflush(stdout);
		}
		Sleep(1);
	}
	return 0;
}

DWORD WINAPI InputThread(LPVOID dummy) {
	int count;
	unsigned char buffer;
	while ((count = fread(&buffer, 1, 1, stdin)) > 0) {
		ssctp_receive_byte(buffer);
	}
	return 0;
}

void ssctp_command_handler(int commandNo, int argn, ssctp_stack_t* argv) {
	switch (commandNo) {
		case 20:
		if (argn < 2) {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		if (argv[0].float_val == NULL || argv[1].float_val == NULL) {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		ssctp_send_float((*argv[0].float_val) * (*argv[1].float_val));
		ssctp_send_command(21);
		break;
		case 22:
		if (argn < 2) {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		if (argv[0].int_val == NULL || argv[1].int_val == NULL) {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		ssctp_send_int21((*argv[0].int_val) * (*argv[1].int_val));
		ssctp_send_command(23);
		break;
		case 220:
		if (argn < 2) {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		if (argv[0].float_val == NULL || argv[1].float_val == NULL) {
			ssctp_send_error(SSCTP_ERR_INVALIDCMD);
			return;
		}
		ssctp_send_float((*argv[0].float_val) / (*argv[1].float_val));
		ssctp_send_command(221);
		break;
	}
	return;
}

int main(int argc, char* argv) {
	int i;

	_setmode( _fileno( stdout ), _O_BINARY );
	_setmode( _fileno( stdin ), _O_BINARY );

	if (argc > 1) {
		system("pause");
	}
	rsb_init_array(&buffer, data_buffer);
	CreateThread(NULL, 0, ReaderThread, NULL, 0, NULL);

	ssctp_init();

	CreateThread(NULL, 0, InputThread, NULL, 0, NULL);
	while (true) {
		Sleep(500);
	}

	return 0;
}