/**
 * Some functions to make color fades on GUI elements (f.e. used for hero and group icons)
 */

/**
 * Used for storing information about color fades
 */
var g_ColorFade = {};

var g_FadeAttackUnit = {

	// How many ticks should first blinking phase be
	"blinkingTicks": 50,

	// How often should the color be changed during the blinking phase
	"blinkingChangeInterval": 5,

	// How fast should blue and green part of the color change
	"gbcolorChangeRate": 3,

	// When should the fade out start using the opacity
	"fadeOutStart": 100,

	// How fast should opacity change
	"opacityChangeRate": 3
};

function getInitColorFadeRGB()
{
	return { "r": 0, "g": 0, "b": 0, "o": 100 };
}

/**
 * Starts fading a color of a GUI object using the sprite argument.
 *
 * @param {string} name - name of the object which color should be faded
 * @param {number} tickInterval - interval in ms when the next color change should be made
 * @param {number} duration - maximum duration of the complete fade (if 0 it runs until it is stopped)
 * @param {function} fun_colorTransform - function which transform the colors
 * @param {boolean} [restartable] - if false, the fade can not be restarted; default: true
 * @param {function} [fun_smoothRestart] - a function, which returns a smooth tick counter, if the fade should be started;
 *                   arguments: [var data]; must return false, if smooth restart was not possible and true, if it was ok
 */
function startColorFade(name, tickInterval, duration, fun_colorTransform, restartable = true, fun_smoothRestart = false)
{
	var overlay = Engine.GetGUIObjectByName(name);
	if (!overlay)
		return;

	// check, if fade overlay was started just now
	if (!isColorFadeRunning(name))
	{
		overlay.hidden = false;

		// store the values into a var to make it more flexible (can be changed from every function)
		g_ColorFade[name] = {
			"timerId": -1,
			"tickInterval": tickInterval,
			"duration": duration,
			"fun_colorTransform": fun_colorTransform,
			"restartable": restartable,
			"fun_smoothRestart": fun_smoothRestart,
			"tickCounter": 0,
			"justStopAtExternCall": duration == 0,
			"stopFade": false,
			"rgb": getInitColorFadeRGB()
		};

		// start with fading
		fadeColorTick(name);
	}
	else if (restartable)
	{
		restartColorFade(name, tickInterval, duration, fun_colorTransform, restartable, fun_smoothRestart);
		return;
	}
}

/**
 * Changes the color on tick.
 *
 * @param {string} name - name of the object which color should be faded
 */
function fadeColorTick(name)
{
	if (!isColorFadeRunning(name))
		return;

	var overlay = Engine.GetGUIObjectByName(name);
	if (!overlay)
		return;
	var data = g_ColorFade[name];

	// change the color
	data.fun_colorTransform(data);

	overlay.sprite = "color:" + rgbToGuiColor(data.rgb, data.rgb.o);

	// recusive call, if duration is positive
	if (!data.stopFade && (data.justStopAtExternCall || data.duration - (data.tickInterval * data.tickCounter) > 0))
	{
		var id = setTimeout(function() { fadeColorTick(name); }, data.tickInterval);
		data.timerId = id;
		data.tickCounter++;
	}
	else
	{
		overlay.hidden = true;
		stopColorFade(name);
	}
}

/**
 * Checks, if a color fade on that object is running.
 *
 * @param {string} name - name of the object which color fade should be checked
 * @returns {boolean} - true a running fade was found
 */
function isColorFadeRunning(name)
{
	return name in g_ColorFade;
}

/**
 * Stops fading a color.
 *
 * @param {string} name - name of the object which color fade should be stopped
 * @param {boolean} hideOverlay - hides the overlay, if true [default: true]
 * @returns {boolean} true a running fade was stopped
 */
function stopColorFade(name, hideOverlay = true)
{
	// check, if a color fade is running
	if (!isColorFadeRunning(name))
		return false;

	// delete the timer
	clearTimeout(g_ColorFade[name].timerId);
	delete g_ColorFade[name];

	// get the overlay and hide it
	if (hideOverlay)
	{
		var overlay = Engine.GetGUIObjectByName(name);
		if (overlay)
			overlay.hidden = true;
	}
	return true;
}

/**
 * Restarts a color fade,
 * see paramter in startColorFade function.
 */
function restartColorFade(name)
{
	// check, if a color fade is running
	if (!isColorFadeRunning(name))
		return false;

	var data = g_ColorFade[name];
	// check, if fade can be restarted smoothly
	if (data.fun_smoothRestart)
	{
		// if call was too late
		if (!data.fun_smoothRestart(data))
		{
			data.rgb = getInitColorFadeRGB(); // set RGB start values
			data.tickCounter = 0;
		}
	}
	// stop it and restart it
	else
	{
		stopColorFade(name, false);
		startColorFade(name, data.changeInterval, data.duration, data.fun_colorTransform, data.restartable, data.fun_smoothRestart);
	}
	return true;
}

function colorFade_attackUnit(data)
{
	var rgb = data.rgb;

	// init color
	if (data.tickCounter == 0)
		rgb.r = 175;

	// blinking
	if (data.tickCounter < g_FadeAttackUnit.blinkingTicks)
	{
		// slow that process down
		if (data.tickCounter % g_FadeAttackUnit.blinkingChangeInterval != 0)
			return;

		rgb.g = rgb.g == 0 ? 255 : 0;
	}
	// wait a short time and then color fade from red to grey to nothing
	else if (data.tickCounter >= g_FadeAttackUnit.blinkingTicks + g_FadeAttackUnit.blinkingChangeInterval)
	{
		rgb.g += Math.round(g_FadeAttackUnit.gbcolorChangeRate * Math.sqrt(data.tickCounter - g_FadeAttackUnit.blinkingTicks));
		if (rgb.g > 255)
			rgb.g = 255;

		// start with fading it out
		if (rgb.g > g_FadeAttackUnit.fadeOutStart)
			rgb.o = rgb.o > g_FadeAttackUnit.opacityChangeRate ? rgb.o -= g_FadeAttackUnit.opacityChangeRate : 0;
		// check for end
		if (rgb.o == 0)
			data.stopFade = true;
	}
	rgb.b = rgb.g;
}

function smoothColorFadeRestart_attackUnit(data)
{
	// check, if in blinking phase
	if (data.tickCounter < g_FadeAttackUnit.blinkingTicks)
	{
		data.tickCounter %= g_FadeAttackUnit.blinkingChangeInterval * 2;
		return true;
	}
	return false;
}
