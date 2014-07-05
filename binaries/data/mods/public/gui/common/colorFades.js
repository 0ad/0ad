/*
	DESCRIPTION	: Some functions to make color fades on GUI elements (f.e. used for hero and group icons)
	NOTES		:
*/

// Used for storing information about color fades
var g_colorFade = {}; 

/**
 * returns the init RGB color setting
 */
function getInitColorFadeRGB()
{
	var rgb = {"r": 0, "g": 0, "b": 0, "o": 100};
	return rgb;
}

/**
 * starts fading a color of a GUI object using the sprite argument
 * name: name of the object which color should be faded
 * tickInterval: interval in ms when the next color change should be made
 * duration: maximal duration of the complete fade (if 0 it runs until it is stopped)
 * fun_colorTransform: function which transform the colors; 
 *					   arguments: [var data] 
 * restartAble [optional: if false, the fade can not be restarted; default: true
 * fun_smoothRestart [optional]: a function, which returns a smooth tick counter, if the fade should be started; 
 *								 arguments: [var data]; must return false, if smooth restart was not possible and true, if it was ok
 */
function startColorFade(name, tickInterval, duration, fun_colorTransform, restartAble = true, fun_smoothRestart = false)
{
	// get the overlay
	var overlay = Engine.GetGUIObjectByName(name);
	if (!overlay)
		return;

	// check, if fade overlay was started just now
	if (!isColorFadeRunning(name))
	{
		overlay.hidden = false;
		// store the values into a var to make it more flexible (can be changed from every function)
		var data = {	"timerId": -1,
						"tickInterval": tickInterval,
						"duration": duration,
						"fun_colorTransform": fun_colorTransform,
						"restartAble": restartAble,
						"fun_smoothRestart": fun_smoothRestart,
						"tickCounter": 0,
						"justStopAtExternCall": duration == 0,
						"stopFade": false,
						"rgb": getInitColorFadeRGB()
					};
		// store it!
		g_colorFade[name] = data;

		// start with fading
		fadeColorTick(name);
	}
	else if (restartAble)
	{
		restartColorFade(name, tickInterval, duration, fun_colorTransform, restartAble, fun_smoothRestart);
		return;
	}
}

/**
 * makes the color changes in a tick
 * name: name of the object which color should be faded
 */
function fadeColorTick(name)
{
	// make some checks
	if (!isColorFadeRunning(name))
		return;
		
	var overlay = Engine.GetGUIObjectByName(name);
	if (!overlay)
		return;
	var data = g_colorFade[name];
		
	// change the color
	data.fun_colorTransform(data);
	
	// set new color
	var rgb = data.rgb;
	overlay.sprite="colour: " + rgb.r + " " + rgb.g + " " + rgb.b + " " + rgb.o;

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
 * checks, if a color fade on that object is running
 * name: name of the object which color fade should be checked
 * return: true a running fade was found
 */
function isColorFadeRunning(name)
{
	return name in g_colorFade;
}

/**
 * stops fading a color
 * name: name of the object which color fade should be stopped
 * hideOverlay [optional]: hides the overlay, if true [default: true]
 * return: true a running fade was stopped
 */
function stopColorFade(name, hideOverlay = true)
{
	// check, if a color fade is running
	if (!isColorFadeRunning(name))
		return false;

	// delete the timer
	clearTimeout(g_colorFade[name].timerId);
	delete g_colorFade[name];

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
 * restarts a color fade
 * see paramter in startColorFade function
 */
function restartColorFade(name)
{
	// check, if a color fade is running
	if (!isColorFadeRunning(name))
		return false;
	
	var data = g_colorFade[name];
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
		startColorFade(name, data.changeInterval, data.duration, data.fun_colorTransform, data.restartAble, data.fun_smoothRestart);
	}
	return true;
}

/**										PREDEFINED FUNCTIONS											*/

//[START] of hero fade functions

var g_fadeAttackUnit = {};
g_fadeAttackUnit.blinkingTicks = 50; // how many ticks should first blinking phase be
g_fadeAttackUnit.blinkingChangeInterval = 5; // how often should the color be changed during the blinking phase
g_fadeAttackUnit.gbcolorChangeRate = 3; // how fast should blue and green part of the color change
g_fadeAttackUnit.fadeOutStart = 100; // when should the fade out start using the opacity
g_fadeAttackUnit.opacityChangeRate = 3; // how fast should opacity change

function colorFade_attackUnit(data)
{
	var rgb = data.rgb;
	
	// init color
	if (data.tickCounter == 0)
		rgb.r = 175;
	// blinking
	if (data.tickCounter < g_fadeAttackUnit.blinkingTicks) 
	{
		// slow that process down
		if (data.tickCounter % g_fadeAttackUnit.blinkingChangeInterval != 0) 
			return;
			
		rgb.g = rgb.g == 0 ? 255 : 0;
	}
	// wait a short time and then color fade from red to grey to nothing
	else if ( data.tickCounter >= g_fadeAttackUnit.blinkingTicks + g_fadeAttackUnit.blinkingChangeInterval) 
	{
		rgb.g += Math.round(g_fadeAttackUnit.gbcolorChangeRate * Math.sqrt(data.tickCounter - g_fadeAttackUnit.blinkingTicks));
		if (rgb.g > 255)
			rgb.g = 255;
		
		// start with fading it out
		if (rgb.g > g_fadeAttackUnit.fadeOutStart) 
			rgb.o = rgb.o > g_fadeAttackUnit.opacityChangeRate ? rgb.o -= g_fadeAttackUnit.opacityChangeRate : 0;
		// check for end
		if (rgb.o == 0)
			data.stopFade = true;
	}
	rgb.b = rgb.g;
}

function smoothColorFadeRestart_attackUnit(data)
{
	// check, if in blinking phase
	if (data.tickCounter < g_fadeAttackUnit.blinkingTicks) 
	{
		data.tickCounter = data.tickCounter % (g_fadeAttackUnit.blinkingChangeInterval * 2);
		return true;
	}
	return false;
}

//[END] of hero fade functions
