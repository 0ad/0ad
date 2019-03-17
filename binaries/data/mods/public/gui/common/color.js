/**
 * Used to highlight hotkeys in tooltip descriptions.
 */
var g_HotkeyTags = {"color": "255 251 131" };

/**
 * Concatenate integer color values to a string (for use in GUI objects)
 *
 * @param {Object} color
 * @param {number} alpha
 * @returns {string}
 */
function rgbToGuiColor(color, alpha)
{
	let ret = "0 0 0";

	if (color && ("r" in color) && ("g" in color) && ("b" in color))
		ret = color.r + " " + color.g + " " + color.b;

	if (alpha !== undefined)
		ret += " " + alpha;

	return ret;
}

function guiToRgbColor(string)
{
	let color = string.split(" ");
	if (color.length != 3 && color.length != 4 ||
	    color.some(num => !Number.isInteger(+num) || num < 0 || num > 255))
		return undefined;

	return {
		"r": +color[0],
		"g": +color[1],
		"b": +color[2],
		"alpha": color.length == 4 ? +color[3] : undefined
	};
}

/**
 * True if the colors are identical.
 *
 * @param {Object} color1
 * @param {Object} color2
 * @returns {boolean}
 */
function sameColor(color1, color2)
{
	return color1.r === color2.r && color1.g === color2.g && color1.b === color2.b;
}

/**
 * Computes the euclidian distance between the two colors.
 * The smaller the return value, the close the colors. Zero if identical.
 *
 * @param {Object} color1
 * @param {Object} color2
 * @returns {number}
 */
function colorDistance(color1, color2)
{
	return Math.euclidDistance3D(color1.r, color1.g, color1.b, color2.r, color2.g, color2.b);
}

/**
 * Ensure `value` is between 0 and 1.
 *
 * @param {number} value
 * @returns {number}
 */
function clampColorValue(value)
{
	return Math.abs(1 - Math.abs(value - 1));
}

/**
 * Convert color value from RGB to HSL space.
 *
 * @see {@link https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion}
 * @param {number} r - red
 * @param {number} g - green
 * @param {number} b - blue
 * @returns {Array}
 */
function rgbToHsl(r, g, b)
{
	r /= 255;
	g /= 255;
	b /= 255;
	let max = Math.max(r, g, b), min = Math.min(r, g, b);
	let h, s, l = (max + min) / 2;

	if (max == min)
		h = s = 0; // achromatic
	else
	{
		let d = max - min;
		s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
		switch (max)
		{
		case r:
			h = (g - b) / d + (g < b ? 6 : 0);
			break;
		case g:
			h = (b - r) / d + 2;
			break;
		case b:
			h = (r - g) / d + 4;
			break;
		}
		h /= 6;
	}

	return [h, s, l];
}

/**
 * Convert color value from HSL to RGB space.
 *
 * @see {@link https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion}
 * @param {number} h - hueness
 * @param {number} s - saturation
 * @param {number} l - lightness
 * @returns {Array}
 */
function hslToRgb(h, s, l)
{
	function hue2rgb(p, q, t)
	{
		if (t < 0)
			t += 1;
		if (t > 1)
			t -= 1;
		if (t < 1/6)
			return p + (q - p) * 6 * t;
		if (t < 1/2)
			return q;
		if (t < 2/3)
			return p + (q - p) * (2/3 - t) * 6;
		return p;
	}

	[h, s, l] = [h, s, l].map(clampColorValue);
	let r, g, b;
	if (s == 0)
		r = g = b = l; // achromatic
	else {
		let q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		let p = 2 * l - q;
		r = hue2rgb(p, q, h + 1/3);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1/3);
	}

	return [r, g, b].map(n => Math.round(n * 255));
}

function colorizeHotkey(text, hotkey)
{
	let key = Engine.ConfigDB_GetValue("user", "hotkey." + hotkey);

	if (!key || key.toLowerCase() == "unused")
		key = sprintf(translate("Unassigned hotkey: %(hotkeyName)s"), {
			"hotkeyName": hotkey
		});

	return sprintf(text, {
		"hotkey": setStringTags("\\[" + key + "]", g_HotkeyTags)
	});
}

/**
 * The autocomplete hotkey is hardcoded in SDLK_TAB of CInput.cpp,
 * as we don't want hotkeys interfering with typing text.
 */
function colorizeAutocompleteHotkey(string)
{
	return sprintf(string || translate("Press %(hotkey)s to autocomplete player names."), {
		"hotkey":
			setStringTags("\\[" + translateWithContext("hotkey", "Tab") + "]", g_HotkeyTags)
	});
}

/**
 * Adds grey font if savegame/replay is not compatible.
 */
function compatibilityColor(text, isCompatible)
{
	return isCompatible ? text : coloredText(text, "96 96 96");
}
