/* Copyright (c) 2011 Wildfire Games
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
	// This function should have no side effects, it should just
	// set these output properties:


	// List of warning strings to display to the user
	// in an ugly GUI dialog box
	var dialog_warnings = [];

	// List of warning strings to log
	var warnings = [];

	var disable_audio = undefined;
	var disable_s3tc = undefined;
	var disable_shadows = undefined;
	var disable_fancywater = undefined;

	// TODO: add some mechanism for setting config values
	// (overriding default.cfg, but overridden by local.cfg)



	// Extract all the settings we might use from the argument:
	// (This is less error-prone than referring to "settings.foo" directly
	// since typos in the matching code will be caught as references to
	// undefined variables.)

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
	var GL_VENDOR = settings.GL_VENDOR;
	var GL_RENDERER = settings.GL_RENDERER;
	var GL_VERSION = settings.GL_VERSION;
	var GL_EXTENSIONS = settings.GL_EXTENSIONS.split(" ");


	// NVIDIA 260.19.* UNIX drivers cause random crashes soon after startup.
	// http://www.wildfiregames.com/forum/index.php?showtopic=13668
	// Fixed in 260.19.21:
	//   "Fixed a race condition in OpenGL that could cause crashes with multithreaded applications."
	if (os_unix && GL_VERSION.match(/NVIDIA 260\.19\.(0[0-9]|1[0-9]|20)$/))
	{
		dialog_warnings.push("You are using 260.19.* series NVIDIA drivers, which may crash the game. Please upgrade to 260.19.21 or later.");
	}

	// http://trac.wildfiregames.com/ticket/685
	if (os_macosx)
	{
		warnings.push("Audio has been disabled, due to problems with OpenAL on OS X.");
		disable_audio = true;
	}

	// http://trac.wildfiregames.com/ticket/684
	// https://bugs.freedesktop.org/show_bug.cgi?id=24047
	// R600 drivers will advertise support for S3TC but not actually support it,
	// and will draw everything in grey instead, so forcibly disable S3TC.
	// (We should add a version check once there's a version that does support it properly.)
	if (os_unix && GL_RENDERER.match(/^Mesa DRI R600 /))
		disable_s3tc = true;

	// http://trac.wildfiregames.com/ticket/623
	// Shadows are reportedly very slow on various drivers:
	//   r300 classic
	//   Intel 945
	// Shadows are also quite slow on some others:
	//   Intel 4500MHD
	// In the interests of performance, we'll disable them on lots of devices
	if (os_unix && (
		GL_RENDERER.match(/^Mesa DRI R300 /) ||
		GL_RENDERER.match(/^Mesa DRI Intel\S* (915|945|965)(G|GM) /) ||
		GL_RENDERER.match(/^Mesa DRI Mobile Intel\S* GM45 /)
	))
	{
		disable_shadows = true;
	}

	// Fragment-shader water is really slow on at least some old Intel devices
	if (os_unix && (
		GL_RENDERER.match(/^Mesa DRI Intel\S* (915|945|965)(G|GM) /) ||
		GL_RENDERER.match(/^Mesa DRI Mobile Intel\S* GM45 /)
	))
	{
		disable_fancywater = true;
	}

	return {
		"dialog_warnings": dialog_warnings,
		"warnings": warnings,
		"disable_audio": disable_audio,
		"disable_s3tc": disable_s3tc,
		"disable_shadows": disable_shadows,
		"disable_fancywater": disable_fancywater,
	};
}

global.RunHardwareDetection = function(settings)
{
	//print(uneval(settings)+"\n");

	var output = RunDetection(settings);

	//print(uneval(output)+"\n");

	for (var i = 0; i < output.warnings.length; ++i)
		warn(output.warnings[i]);

	if (output.dialog_warnings.length)
	{
		var msg = output.dialog_warnings.join("\n\n");
		Engine.DisplayErrorDialog(msg);
	}

	if (output.disable_audio !== undefined)
		Engine.SetDisableAudio(output.disable_audio);

	if (output.disable_s3tc !== undefined)
		Engine.SetDisableS3TC(output.disable_s3tc);

	if (output.disable_shadows !== undefined)
		Engine.SetDisableShadows(output.disable_shadows);

	if (output.disable_fancywater !== undefined)
		Engine.SetDisableFancyWater(output.disable_fancywater);
};
