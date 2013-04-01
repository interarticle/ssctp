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
 * SimpleSerialCommandTransferProtocol C# Implementation
 *
 * @author Zhao Yichen <max.zhaoyichen@gmail.com>
 * @copyright Copyright (c) 2013 赵迤晨 (Zhao, Yichen) <max.zhaoyichen@gmail.com>
 * @license MIT License
 * @version 0.1
 */

using System;
using System.IO;
using System.Collections;

namespace SimpleSerialCommandTransferProtocol {
	public enum Commands: ushort {
		DataHalt            = 0,
		DataByte1           = 1,
		DataByte2           = 2,
		DataByte3           = 3,
		DataByte4           = 4,
		DataByte5           = 5,
		DataFloat           = 6,
		DataDouble          = 7,
		DataUInt64          = 8,
		BufferClear         = 9,
		BufferVerify        = 10,
		ExtendedCommand     = 11,
		BeginAsyncCommand   = 12,
		EndAsyncCommand     = 13,
		ErrorInvalidCommand = 127,
		ErrorUnknownCommand = 126,
		ErrorDataCorrupt    = 125,
		ErrorBufferOverflow = 124,
		ErrorDataTruncated  = 123,
		ErrorBufferOverrun  = 122,
		ErrorOK             = 118
	}
	public static class Extensions {
		public static bool IsDataCommand(this Commands command) {
			ushort cmd = (ushort) command;
			return (cmd >= 0 && cmd <= 8);
		}

		public static bool IsErrorCommand(this Commands command) {
			ushort cmd = (ushort) command;
			return (cmd >= 118 && cmd <= 127);
		}

		public static int GetDataSize(this Commands command) {
			ushort cmd = (ushort) command;
			if (cmd >= 1 && cmd <= 5)
				return cmd;
			else if (command == Commands.DataFloat)
				return 5;
			else if (command == Commands.DataDouble || command == Commands.DataUInt64)
				return 10;
			else 
				return -1;
		}

		public static bool IsXorClearCommand(this Commands command) {
			return !command.IsDataCommand() && !command.IsErrorCommand();
		}
	}

	public class CustomCommandEventArgs : EventArgs {
		public CustomCommandEventArgs(ushort commandID, object[] parameters) {
			this.commandID = commandID;
			this.parameters = parameters;
		}
		private ushort commandID;
		public ushort CommandID {
			get {
				return commandID;
			}
		}

		private object[] parameters;
		public object[] Parameters {
			get {
				return parameters;
			}
		}
	}

	public class ErrorEventArgs : EventArgs {
		public ErrorEventArgs(Commands error) {
			this.error = error;
		}

		private Commands error;
		public Commands Error {
			get {
				return error;
			}
		}
	}

	//This is thread unsafe!
	public class SSCTP : IDisposable {
		private Stream baseStream;
		private const int asyncReadBufferSize = 10;

		private UInt64 inputBuffer = 0UL;
		private int inputBufferReceived = 0;
		private byte inputXor = 0;
		private Commands inputMode = Commands.DataHalt;

		private ArrayList stack = new ArrayList();

		public Stream BaseStream {
			get {
				return baseStream;
			}
		}

		public SSCTP(Stream baseStream) {
			this.baseStream = baseStream;
			SendErrors = true;
		}

		public event EventHandler<CustomCommandEventArgs> CustomCommand;

		protected void OnCustomCommand(CustomCommandEventArgs e) {
			if (CustomCommand != null) {
				CustomCommand(this, e);
			}
		}

		public event EventHandler<ErrorEventArgs> Error;

		protected void OnError(ErrorEventArgs e) {
			if (Error != null) {
				Error(this, e);
			}
		}

		public bool SendErrors { get; set; }

		private void ClearStack() {
			stack.Clear();
		}

		private void ProcessCommand(Commands command, object[] args) {
			if (command == Commands.ExtendedCommand) {
				if (args.Length == 0)
				{
					SendError(Commands.ErrorInvalidCommand);
					return;
				}
				object[] parameters = new object[args.Length - 1];
				Array.Copy(args, parameters, args.Length - 1);
				ProcessCommand((Commands) args[args.Length - 1], parameters);
			} else if (command == Commands.BufferVerify) {
				if (args.Length == 0)
				{
					SendError(Commands.ErrorInvalidCommand);
					return;
				}
				if (inputXor != (byte) args[0]) {
					SendError(Commands.ErrorDataCorrupt);
				} else {
					SendError(Commands.ErrorOK);
				}
			} else if (command == Commands.BufferClear) {

			} else if (command == Commands.BeginAsyncCommand || command == Commands.EndAsyncCommand) {
				SendError(Commands.ErrorUnknownCommand);
			} else {
				OnCustomCommand(new CustomCommandEventArgs((ushort) command, args));
			}
		}

		private void ReceiveCommand(byte cmd) {
			Commands command = (Commands) cmd;
			if (command.IsErrorCommand()) {
				OnError(new ErrorEventArgs(command));
				return;
			}

			inputBuffer = 0;
			inputBufferReceived = 0;

			if (command.IsDataCommand()) {
				inputMode = command;
			} else {
				ProcessCommand(command, stack.ToArray());
				ClearStack();
			}

			if (command.IsXorClearCommand())
				inputXor = 0;
		}

		private void ReceiveData(byte data) {
			if (inputMode.GetDataSize() > 0) {
				inputXor ^= data;
				if (inputBufferReceived < inputMode.GetDataSize()) {
					inputBuffer |= ((UInt64) data) << (7 * inputBufferReceived ++);
				}

				if (inputBufferReceived >= inputMode.GetDataSize()) {
					switch (inputMode) {
						case Commands.DataByte1:
							stack.Add((byte) inputBuffer);
							break;
						case Commands.DataByte2:
							stack.Add((UInt16) inputBuffer);
							break;
						case Commands.DataByte3: 
						case Commands.DataByte4:
							stack.Add((UInt32) inputBuffer);
							break;
						case Commands.DataByte5:
						case Commands.DataUInt64:
							stack.Add(inputBuffer);
							break;
						case Commands.DataFloat:
							stack.Add(BitConverter.ToSingle(BitConverter.GetBytes((UInt32) inputBuffer), 0));
							break;
						case Commands.DataDouble:
							stack.Add(BitConverter.ToDouble(BitConverter.GetBytes(inputBuffer), 0));
							break;
					}
					inputBuffer = 0UL;
					inputBufferReceived = 0;
				}
			}
		}

		public void FeedByte(byte data) {
			if ((data & (1 << 7)) != 0) {
				ReceiveCommand((byte) (data & ~(1 << 7)));
			} else {
				ReceiveData(data);
			}
		}

		public bool ReadByte() {
			int b = baseStream.ReadByte();
			if (b == -1)
				return false;
			FeedByte((byte) b);
			return true;
		}

		//Note: there is no way to exist this function unless stream reachs an end, or you close it from somewhere else
		public void RunReadByte() {
			while (ReadByte());
		}

		public void SendError(Commands error) {
			if (SendErrors) {
				SendCommand(error);
			}
		}

		private Commands sendDataMode = Commands.DataHalt;

		public void SendCommand(Commands command) {
			command = (Commands) ((ushort) command & ((1 << 14) - 1));
			if (command.IsDataCommand()) {
				sendDataMode = command; 
			} else if (!command.IsErrorCommand()) {
				sendDataMode = 0;
			}

			if ((ushort) command > 127) {
				SendData((ushort) command, 2);
				command = Commands.ExtendedCommand;
			}
			baseStream.WriteByte((byte) ((byte) command | (1 << 7)));
			baseStream.Flush();
		}

		public void SendCommand(ushort command) {
			SendCommand((Commands) command);
		}

		private void SendDataInternal(UInt64 data, int length) {
			while (length -- > 0)
			{
				baseStream.WriteByte((byte) (data & 127));
				data >>= 7;
			}
		}

		public void SendData(UInt64 data, int length) {
			if (!(length >= 1 && length <= 5 || length == 10))
				throw new InvalidOperationException("Invalid Data Length");
			if (sendDataMode != ((Commands) length)) {
				SendCommand((Commands) length);
			}
			SendDataInternal(data, length);
		}

		public void SendData(float data) {
			if (sendDataMode != Commands.DataFloat)
				SendCommand(Commands.DataFloat);
			SendDataInternal(BitConverter.ToUInt32(BitConverter.GetBytes(data), 0), Commands.DataFloat.GetDataSize());
		}


		public void SendData(double data) {
			SendDataInternal(BitConverter.ToUInt64(BitConverter.GetBytes(data), 0), Commands.DataDouble.GetDataSize());
		}

		public void SendData(object data) {
			if (data is sbyte){
				data = (byte) (sbyte) data;
			} else if (data is Int16) {
				data = (UInt16) (Int16) data;
			} else if (data is Int32) {
				data = (UInt32) (Int32) data;
			} else if (data is Int64) {
				data = (UInt64) (Int64) data;
			}
			if (data is byte) {
				if ((byte) data < 128)
					SendUInt7((byte) data);
				else
					SendUInt14((byte) data);
			} else if (data is UInt16) {
				if ((UInt16) data < (1 << 14))
					SendUInt14((UInt16) data);
				else
					SendUInt21((UInt16) data);
			} else if (data is UInt32) {
				if (((UInt32) data) < (1 << 21))
					SendUInt21((UInt32) data);
				else if ((UInt32) data < (1 << 28))
					SendUInt28((UInt32) data);
				else 
					SendUInt35((UInt32) data);
			} else if (data is UInt64) {
				if ((UInt64) data < (1 << 35)) 
					SendUInt35((UInt64) data);
				else 
					SendUInt64((UInt64) data);
			} else if (data is float) {
				SendData((float) data);
			} else if (data is double) {
				SendData((double) data);
			} else {
				throw new InvalidOperationException("Invalid Data type given to SendData(object)");
			}
		}

		public void SendUInt7(byte data) {
			SendData(data, 1);
		}

		public void SendUInt14(ushort data) {
			SendData(data, 2);
		}

		public void SendUInt21(uint data) {
			SendData(data, 3);
		}

		public void SendUInt28(uint data) {
			SendData(data, 4);
		}

		public void SendUInt35(ulong data) {
			SendData(data, 5);
		}

		public void SendUInt64(ulong data) {
			SendData(data, 10);
		}

		public void SendCommand(ushort command, params object[] parameters) {
			SendCommand((Commands) command, parameters);
		}

		public void SendCommand(Commands command, params object[] parameters) {
			foreach (object param in parameters) {
				SendData(param);
			}
			SendCommand(command);
		}

		public void Flush() {
			baseStream.Flush();
		}

		public void Close() {
            Error = null;
            CustomCommand = null;
		}

		public void Dispose() {
			Close();
		}
	}
}