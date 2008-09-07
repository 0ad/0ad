using System;
using System.IO;

using NUnit.Framework;

namespace RunFColladaTests
{
	/// <summary>
	/// Test class for FColladaTest
	/// </summary>
	/// 
	[TestFixture]
	public class RunFColladaTests
	{
		public RunFColladaTests()
		{
			// constructor needed by NUnit
		}

		[Test][STAThread]
		public void DoTest()
		{
			String path = System.Configuration.ConfigurationSettings.AppSettings["path"];
			String exe = System.Configuration.ConfigurationSettings.AppSettings["exe"];
			String outputName = System.Configuration.ConfigurationSettings.AppSettings["output"];

			path.TrimEnd( new char[]{'\\'} );
			path = path + @"\";

			// test for file existence
			String fullName = path + exe;
			FileInfo file = new FileInfo( fullName );
			
			Assert.IsTrue( file.Exists, "Unable to find executable : " + fullName );

			System.Diagnostics.Process proc; // Declare New Process
			System.Diagnostics.ProcessStartInfo procInfo = new System.Diagnostics.ProcessStartInfo(); // Declare New Process Starting Information

			procInfo.UseShellExecute = false;  //If this is false, only .exe's can be run.
			procInfo.WorkingDirectory = path; //execute notepad from the C: Drive
			procInfo.FileName = fullName; // Program or Command to Execute.
			procInfo.Arguments = "-v 0 " + outputName; // no verbose + name of the file to output
			procInfo.CreateNoWindow = true;
			procInfo.RedirectStandardError = true;

			proc = System.Diagnostics.Process.Start( procInfo );
			proc.WaitForExit(); // Waits for the process to end.

			// test for errors in the called procedure
			String procErr = proc.StandardError.ReadToEnd();
			Assert.IsTrue( (procErr.Length == 0), procErr );

			FileInfo outFile = new FileInfo( path + outputName );

			// test for outFile existence
			Assert.IsTrue( outFile.Exists, exe + " was unable to create output file." );

			StreamReader sr = new StreamReader( outFile.OpenRead() );
			String strFile = sr.ReadToEnd();
			sr.Close();

			// if file non-empty then print report
			Assert.IsTrue( (outFile.Length == 0), strFile );
		}
	}
}
