/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*

This script is for adjusting the game's default settings based on the
user's system configuration details.

The game engine itself does some detection of capabilities, so it will
enable certain graphical features only when they are supported.
This script is for the messier task of avoiding performance problems
and driver bugs based on experience of particular system configurations.

*/

function RunDetection(settings)
{
	// List of warning strings to display to the user
	var warnings = [];

	// TODO: add some mechanism for setting config values
	// (overriding default.cfg, but overridden by local.cfg)


	// Extract all the settings we might use from the argument:
	// (This is less error-prone than referring to "settings.foo" directly
	// since typos will be caught)

	// OS flags (0 or 1)
	var os_unix = settings.os_unix;
	var os_linux = settings.os_linux;
	var os_macosx = settings.os_macosx;
	var os_win = settings.os_win;

	// Should avoid using these, since they're disabled in quickstart mode
	var gfx_card = settings.gfx_card;
	var gfx_drv_ver = settings.gfx_drv_ver;
	var gfx_mem = settings.gfx_mem;

	// Values from glGetString
	var gl_vendor = settings.gl_vendor;
	var gl_renderer = settings.gl_renderer;
	var gl_version = settings.gl_version;
	var gl_extensions = settings.gl_extensions.split(" "); // split on spaces

	var video_xres = settings.video_xres;
	var video_yres = settings.video_yres;
	var video_bpp = settings.video_bpp;

	var uname_sysname = settings.uname_sysname;
	var uname_release = settings.uname_release;
	var uname_version = settings.uname_version;
	var uname_machine = settings.uname_machine;

	var cpu_identifier = settings.cpu_identifier;
	var cpu_frequency = settings.cpu_frequency; // -1 if unknown

	var ram_total = settings.ram_total; // megabytes
	var ram_free = settings.ram_free; // megabytes


	// NVIDIA 260.19.* UNIX drivers cause random crashes soon after startup.
	// http://www.wildfiregames.com/forum/index.php?showtopic=13668
	// Fixed in 260.19.21:
	//   "Fixed a race condition in OpenGL that could cause crashes with multithreaded applications."
	if (os_unix && gl_version.match(/NVIDIA 260\.19\.(0[0-9]|1[0-9]|20)$/))
	{
		warnings.push("You are using 260.19.* series NVIDIA drivers, which may crash the game. Please upgrade to 260.19.21 or later.");
	}


	return { "warnings": warnings };
}

global.RunHardwareDetection = function(settings)
{
	//print(uneval(settings)+"\n");

	var output = RunDetection(settings);

	//print(uneval(output)+"\n");

	if (output.warnings.length)
	{
		var msg = output.warnings.join("\n\n");
		Engine.DisplayErrorDialog(msg);
	}
};
