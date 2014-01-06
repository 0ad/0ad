/*
	DESCRIPTION	: Some functions to make colour fades on GUI elements (f.e. used for hero and group icons)
	NOTES		:
*/

// Used for storing object names of running color fades in order to stop them, if the fade is restarted before the old ended
var g_colorFade = {};
g_colorFade["id"] = {};
g_colorFade["tick"] = {};

/**
 * starts fading a colour of a GUI object using the sprite argument
 * name: name of the object which colour should be faded
 * changeInterval: interval in ms when the next colour change should be made
 * duration: maximal duration of the complete fade
 * colour: RGB + opacity object with keys r,g,b and o
 * fun_colorTransform: function which transform the colors; 
 *					  arguments: [colour object, tickCounter] 
 * fun_smoothRestart [optional]: a function, which returns a smooth tick counter, if the fade should be started; 
 *								arguments: [tickCounter of current fade; not smaller than 1 or it restarts at 0] returns: smooth tick counter value
 * tickCounter [optional]: should not be set by hand! - how often the function was called recursively
 */
function fadeColour(name, changeInterval, duration, colour, fun_colorTransform, fun_smoothRestart, tickCounter)
{
	// get the overlay
	var overlay = Engine.GetGUIObjectByName(name);
	if (!overlay)
		return;

	// check, if fade overlay was started just now
	if (!tickCounter)
	{
		tickCounter = 1;
		overlay.hidden = false;
					
		// check, if another animation is running and restart it, if it's the case
		if (isColourFadeRunning(name)) 
		{
			restartColourFade(name, changeInterval, duration, colour, fun_colorTransform, fun_smoothRestart, g_colorFade.tick[name]);
			return;
		}
	}
	
	// get colors
	fun_colorTransform(colour, tickCounter);
	
	// set new colour
	overlay.sprite="colour: "+colour.r+" "+colour.g+" "+colour.b+" "+colour.o;

	// recusive call, if duration is positive
	duration-= changeInterval; 
	if (duration > 0 && colour.o > 0)
	{
		var id = setTimeout(function() { fadeColour(name, changeInterval, duration, colour, fun_colorTransform, fun_smoothRestart, ++tickCounter); }, changeInterval);
		g_colorFade.id[name] = id;
		g_colorFade.tick[name] = tickCounter;
	}
	else 
	{
		overlay.hidden = true;
		stopColourFade(name);
	}
}


/**
 * checks, if a colour fade on that object is running
 * name: name of the object which colour fade should be checked
 * return: true a running fade was found
 */
function isColourFadeRunning(name)
{
	return name in g_colorFade.id;
}

/**
 * stops fading a colour
 * name: name of the object which colour fade should be stopped
 * hideOverlay: hides the overlay, if true
 * return: true a running fade was stopped
 */
function stopColourFade(name, hideOverlay)
{
	// check, if a colour fade is running
	if (!isColourFadeRunning(name))
		return false;

	// delete the timer
	clearTimeout(g_colorFade.id[name]);
	delete g_colorFade.id[name];
	delete g_colorFade.tick[name];
	
	// get the overlay and hide it
	if (hideOverlay)
	{
		var overlay = Engine.GetGUIObjectByName(name);
		if(overlay) 
			overlay.hidden = true;
	}
	return true;
}

/**
 * restarts a colour fade
 * see paramter in fadeColour function
 */
function restartColourFade(name, changeInterval, duration, colour, fun_colorTransform, fun_smoothRestart, tickCounter)
{
	// check, if a colour fade is running
	if (!isColourFadeRunning(name))
		return false;
	
	// check, if fade can be restarted smoothly
	if (fun_smoothRestart)
	{
		tickCounter = fun_smoothRestart(colour, tickCounter);
		// set new function to existing timer
		var fun = function() { fadeColour(name, changeInterval, duration, colour, fun_colorTransform, fun_smoothRestart, tickCounter); };
		setNewTimerFunction(g_colorFade.id[name], fun);
	}
	// stop it and restart it
	else
	{
		stopColourFade(name, true);
		fadeColour(name, changeInterval, duration, colour, fun_colorTransform);
	}
	return true;
}

/**										PREDEFINED FUNCTIONS											*/

//[START] of hero fade functions

var g_fadeAttackUnit = {};
g_fadeAttackUnit.blinkingTicks = 50; // how many ticks should first blinking phase be
g_fadeAttackUnit.blinkingChangeInterval = 5; // how often should the colour be changed during the blinking phase
g_fadeAttackUnit.gbColourChangeRate = 3; // how fast should blue and green part of the colour change
g_fadeAttackUnit.fadeOutStart = 100; // when should the fade out start using the opacity
g_fadeAttackUnit.opacityChangeRate = 3; // how fast should opacity change

/**
 * rgb: colour object with keys r,g,b and o
 * tickCounter: how often the fade was executed
 */
function colourFade_attackUnit(rgb, tickCounter)
{
	// blinking
	if (tickCounter < g_fadeAttackUnit.blinkingTicks) 
	{
		// slow that process down
		if (tickCounter % g_fadeAttackUnit.blinkingChangeInterval != 0) 
			return;
			
		rgb.g = rgb.g == 0 ? 255 : rgb.g = 0;
		rgb.b = rgb.b == 0 ? 255 : rgb.b = 0;
	}
	// wait a short time and then colour fade from red to grey to nothing
	else if ( tickCounter >= g_fadeAttackUnit.blinkingTicks + g_fadeAttackUnit.blinkingChangeInterval) 
	{
		rgb.g = rgb.g < 255 ? rgb.g += g_fadeAttackUnit.gbColourChangeRate * Math.sqrt(tickCounter - g_fadeAttackUnit.blinkingTicks) : 255;
		rgb.b = rgb.g;
		
		// start with fading it out
		if (rgb.g > g_fadeAttackUnit.fadeOutStart) 
			rgb.o = rgb.o > g_fadeAttackUnit.opacityChangeRate ? rgb.o -= g_fadeAttackUnit.opacityChangeRate : 0;
	}
}

/**
 * makes a smooth fade, if the attack on the unit has not stopped yet
 * rgb: colour object with keys r,g,b and o
 * tickCounter: how often the fade was executed
 */
function smoothColourFadeRestart_attackUnit(rgb, tickCounter)
{
	// check, if in blinking phase
	if (tickCounter < g_fadeAttackUnit.blinkingTicks) 
	{
		// get rgb to current state
		for (var i = 1; i <= tickCounter; i++)
			colourFade_attackUnit(rgb, i);
		// set the tick counter back to start
		return (tickCounter % (g_fadeAttackUnit.blinkingChangeInterval * 2)) + 1;
	}
	return 1;
}

//[END] of hero fade functions