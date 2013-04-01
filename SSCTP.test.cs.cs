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

using System;
using System.IO;
using System.Diagnostics;
using SimpleSerialCommandTransferProtocol;
using System.Threading;

public class m{
	private static Process p;
	public static void Main() {
		ProcessStartInfo psi = new ProcessStartInfo("ssctp.exe");
		psi.RedirectStandardInput = true;
		psi.RedirectStandardOutput = true;
		psi.UseShellExecute = false;

		p = Process.Start(psi);

		SSCTP prot = new SSCTP(p.StandardOutput.BaseStream);

		prot.CustomCommand += (sender, e) => {
			Console.Write(e.CommandID);
			foreach (object o in e.Parameters)
			{
				Console.Write("\t");
				Console.Write(o);
			}
			Console.WriteLine();
		};

		prot.Error += (sender, e) => {
			Console.Write("Error: {0}\n", e.Error);
		};

		Thread ts = new Thread(Reader);
		ts.Start();
		prot.RunReadByte();
	}

	public static void Reader() {
		SSCTP prot = new SSCTP(p.StandardInput.BaseStream);
		for(;;) {
			prot.SendCommand(20, 3.0f, 2.5f);
			prot.SendCommand(22, (short) 3, (short) 5);
			prot.SendCommand(220, 3.0f, 5.0f);
			Thread.Sleep(500);
		}
	}
}