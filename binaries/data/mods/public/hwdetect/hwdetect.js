/* Copyright (c) 2013 Wildfire Games
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

var g_IntelMesaChipsets = [
	"Intel(R) 845G",
	"Intel(R) 830M",
	"Intel(R) 852GM/855GM",
	"Intel(R) 865G",
	"Intel(R) 915G",
	"Intel (R) E7221G (i915)",
	"Intel(R) 915GM",
	"Intel(R) 945G",
	"Intel(R) 945GM",
	"Intel(R) 945GME",
	"Intel(R) G33",
	"Intel(R) Q35",
	"Intel(R) Q33",
	"Intel(R) IGD",
	"Intel(R) 965Q",
	"Intel(R) 965G",
	"Intel(R) 946GZ",
	"Intel(R) GMA500", // not in current Mesa
	"Intel(R) 965GM",
	"Intel(R) 965GME/GLE",
	"Mobile Intel\xC2\xAE GM45 Express Chipset", // utf-8 decoded as iso-8859-1
	"Intel(R) Integrated Graphics Device",
	"Intel(R) G45/G43",
	"Intel(R) Q45/Q43",
	"Intel(R) G41",
	"Intel(R) B43",
	"Intel(R) IGDNG_D", // not in current Mesa; looks somewhat like Ironlake
	"Intel(R) IGDNG_M", // not in current Mesa; looks somewhat like Ironlake
	"Intel(R) Ironlake Desktop",
	"Intel(R) Ironlake Mobile",
	"Intel(R) Sandybridge Desktop",
	"Intel(R) Sandybridge Mobile",
	"Intel(R) Sandybridge Server",
	"Intel(R) Ivybridge Desktop",
	"Intel(R) Ivybridge Mobile",
	"Intel(R) Ivybridge Server",
	"Intel(R) Haswell Desktop",
	"Intel(R) Haswell Mobile",
	"Intel(R) Haswell Server",
	"Unknown Intel Chipset",
	"*", // dummy value to support IsWorseThanIntelMesa("*") to detect all Intel Mesa devices
];
// Originally generated from Mesa with
//   perl -lne'print "\t$1," if /chipset = (".*")/' src/mesa/drivers/dri/intel/intel_context.c
// Assumed to be roughly ordered by performance.

var g_IntelWindowsChipsets = [
	"Intel 845G",
	"Intel 855GM",
	"Intel 865G",
	"Intel 915G",
	"Intel 915GM",
	"Intel 945G",
	"Intel 945GM",
	"Intel 965/963 Graphics Media Accelerator",
	"Intel Broadwater G",
	"Intel Bear Lake B",
	"Intel Pineview Platform",
	"Intel Eaglelake",
	"Intel(R) G41 Express Chipset", // Eaglelake
	"Intel(R) G45/G43 Express Chipset", // Eaglelake
	"Intel Cantiga",
	"Mobile Intel(R) 4 Series Express Chipset Family", // probably Cantiga
	"Intel(R) HD Graphics", // probably Ironlake
	"Intel(R) Graphics Media Accelerator HD", // no idea
	"*",
];
// Determined manually from data reports.
// See http://en.wikipedia.org/wiki/Intel_GMA for useful listing.

var g_IntelMacChipsets = [
	"Intel GMA 950",
	"Intel GMA X3100",
	"Intel HD Graphics",
	"Intel HD Graphics 3000",
	"Unknown Intel Chipset",
	"*",
];
// Determined manually from data reports.
// See http://support.apple.com/kb/HT3246 for useful listing.

function IsWorseThanIntelMesa(renderer, chipset)
{
	var target = g_IntelMesaChipsets.indexOf(chipset);
	if (target == -1)
		error("Invalid chipset "+chipset);

	// GL_RENDERER is "Mesa DRI $chipset" or "Mesa DRI $chipset $otherstuff"
	for (var i = 0; i < target; ++i)
	{
		var str = "Mesa DRI " + g_IntelMesaChipsets[i];
		if (renderer == str || renderer.substr(0, str.length+1) == str+" ")
			return true;
	}

	return false;
}

function IsWorseThanIntelWindows(renderer, chipset)
{
	var target = g_IntelWindowsChipsets.indexOf(chipset);
	if (target == -1)
		error("Invalid chipset "+chipset);

	var match = g_IntelWindowsChipsets.indexOf(renderer);
	if (match != -1 && match < target)
		return true;

	return false;
}

function IsWorseThanIntelMac(renderer, chipset)
{
	var target = g_IntelMacChipsets.indexOf(chipset);
	if (target == -1)
		error("Invalid chipset "+chipset);

	// GL_RENDERER is "$chipset OpenGL Engine"
	for (var i = 0; i < target; ++i)
	{
		var str = g_IntelMacChipsets[i]+" OpenGL Engine";
		if (renderer == str)
			return true;
	}

	return false;
}

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
	var disable_shadowpcf = undefined;
	var disable_allwater = undefined;
	var disable_fancywater = undefined;
	var override_renderpath = undefined;

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
	// Shadows are also known to be quite slow on some others:
	//   Intel 4500MHD
	// In the interests of performance, we'll disable them on lots of devices
	// (with a fairly arbitrary cutoff for Intels)
	if ((os_unix && GL_RENDERER.match(/^(Software Rasterizer|Gallium \S* on llvmpipe|Mesa X11|Apple Software Renderer)$/)) ||
		(os_unix && GL_RENDERER.match(/^Mesa DRI R[123]00 /)) ||
		(os_macosx && IsWorseThanIntelMac(GL_RENDERER, "Intel HD Graphics 3000")) ||
		(os_unix && IsWorseThanIntelMesa(GL_RENDERER, "Intel(R) Ironlake Desktop")) ||
		(os_win && IsWorseThanIntelWindows(GL_RENDERER, "Intel(R) HD Graphics"))
	)
	{
		disable_shadows = true;
		disable_shadowpcf = true;
	}

	// Fragment-shader water is really slow on most Intel devices (especially the
	// "use actual depth" option) and on software renderers, so disable it on all of them
	if ((os_unix && GL_RENDERER.match(/^(Software Rasterizer|Gallium \S* on llvmpipe|Apple Software Renderer)$/)) ||
		(os_macosx && IsWorseThanIntelMac(GL_RENDERER, "*")) ||
		(os_unix && IsWorseThanIntelMesa(GL_RENDERER, "*")) ||
		(os_win && IsWorseThanIntelWindows(GL_RENDERER, "*"))
	)
	{
		disable_allwater = false;
		disable_fancywater = true;
		disable_shadowpcf = true;
	}

	// http://trac.wildfiregames.com/ticket/780
	// r300 classic has problems with shader mode, so fall back to non-shader
	if (os_unix && GL_RENDERER.match(/^Mesa DRI R[123]00 /))
	{
		override_renderpath = "fixed";
		warnings.push("Some graphics features are disabled, due to bugs in old graphics drivers. Upgrading to a Gallium-based driver might help.");
	}

	// http://www.wildfiregames.com/forum/index.php?showtopic=15058
	// GF FX has poor shader performance, so fall back to non-shader
	if (GL_RENDERER.match(/^GeForce FX /))
	{
		override_renderpath = "fixed";
		disable_allwater = true;
	}

	// http://trac.wildfiregames.com/ticket/964
	// SiS Mirage 3 drivers apparently crash with shaders, so fall back to non-shader
	// (The other known SiS cards don't advertise GL_ARB_fragment_program so we
	// don't need to do anything special for them)
	if (os_win && GL_RENDERER.match(/^Mirage Graphics3$/))
	{
		override_renderpath = "fixed";
	}

	return {
		"dialog_warnings": dialog_warnings,
		"warnings": warnings,
		"disable_audio": disable_audio,
		"disable_s3tc": disable_s3tc,
		"disable_shadows": disable_shadows,
		"disable_shadowpcf": disable_shadowpcf,
		"disable_allwater": disable_allwater,
		"disable_fancywater": disable_fancywater,
		"override_renderpath": override_renderpath,
	};
}

global.RunHardwareDetection = function(settings)
{
	//print(JSON.stringify(settings, null, 1)+"\n");

	var output = RunDetection(settings);

	//print(JSON.stringify(output, null, 1)+"\n");

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

	if (output.disable_shadowpcf !== undefined)
		Engine.SetDisableShadowPCF(output.disable_shadowpcf);

	if (output.disable_allwater !== undefined)
		Engine.SetDisableFancyWater(output.disable_allwater);
	
	if (output.disable_fancywater !== undefined)
		Engine.SetDisableFancyWater(output.disable_fancywater);

	if (output.override_renderpath !== undefined)
		Engine.SetRenderPath(output.override_renderpath);
};
