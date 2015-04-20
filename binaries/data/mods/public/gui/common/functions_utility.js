/*
	DESCRIPTION	: Generic utility functions.
	NOTES	:
*/

// ====================================================================

function getRandom(randomMin, randomMax)
{
	// Returns a random whole number in a min..max range.
	// NOTE: There should probably be an engine function for this,
	// since we'd need to keep track of random seeds for replays.

	var randomNum = randomMin + (randomMax-randomMin)*Math.random();  // num is random, from A to B
	return Math.round(randomNum);
}

// ====================================================================

// Get list of XML files in pathname with recursion, excepting those starting with _
function getXMLFileList(pathname)
{
	var files = Engine.BuildDirEntList(pathname, "*.xml", true);

	var result = [];

	// Get only subpath from filename and discard extension
	for (var i = 0; i < files.length; ++i)
	{
		var file = files[i];
		file = file.substring(pathname.length, file.length-4);

		// Split path into directories so we can check for beginning _ character
		var tokens = file.split("/");

		if (tokens[tokens.length-1][0] != "_")
			result.push(file);
	}

	return result;
}

// ====================================================================

// Get list of JSON files in pathname
function getJSONFileList(pathname)
{
	var files = Engine.BuildDirEntList(pathname, "*.json", false);

	// Remove the path and extension from each name, since we just want the filename
	files = [ n.substring(pathname.length, n.length-5) for each (n in files) ];

	return files;
}

// ====================================================================

// A sorting function for arrays of objects with 'name' properties, ignoring case
function sortNameIgnoreCase(x, y)
{
	var lowerX = x.name.toLowerCase();
	var lowerY = y.name.toLowerCase();

	if (lowerX < lowerY)
		return -1;
	else if (lowerX > lowerY)
		return 1;
	else
		return 0;
}

// ====================================================================

/**
 * Escape tag start and escape characters, so users cannot use special formatting.
 * Also limit string length to 256 characters (not counting escape characters).
 */
function escapeText(text)
{
	if (!text)
		return text;

	return text.substr(0, 255).replace(/\\/g, "\\\\").replace(/\[/g, "\\[");
}

// ====================================================================

// Load default player data, for when it's not otherwise specified
function initPlayerDefaults()
{
	var data = Engine.ReadJSONFile("simulation/data/player_defaults.json");
	if (!data || !data.PlayerData)
	{
		error("Failed to parse player defaults in player_defaults.json (check for valid JSON data)");
		return [];
	}

	return data.PlayerData;
}

// ====================================================================

// Load map size data
function initMapSizes()
{
	var sizes = {
		"shortNames":[],
		"names":[],
		"tiles": [],
		"default": 0
	};

	var data = Engine.ReadJSONFile("simulation/data/map_sizes.json");
	if (!data || !data.Sizes)
	{
		error("Failed to parse map sizes in map_sizes.json (check for valid JSON data)");
		return sizes;
	}

	translateObjectKeys(data, ["Name", "LongName"]);
	for (var i = 0; i < data.Sizes.length; ++i)
	{
		sizes.shortNames.push(data.Sizes[i].Name);
		sizes.names.push(data.Sizes[i].LongName);
		sizes.tiles.push(data.Sizes[i].Tiles);

		if (data.Sizes[i].Default)
			sizes["default"] = i;
	}

	return sizes;
}

// ====================================================================

// Load game speed data
function initGameSpeeds()
{
	var gameSpeeds = {
		"names": [],
		"speeds": [],
		"default": 0
	};

	var data = Engine.ReadJSONFile("simulation/data/game_speeds.json");
	if (!data || !data.Speeds)
	{
		error("Failed to parse game speeds in game_speeds.json (check for valid JSON data)");
		return gameSpeeds;
	}

	translateObjectKeys(data, ["Name"]);
	for (var i = 0; i < data.Speeds.length; ++i)
	{
		gameSpeeds.names.push(data.Speeds[i].Name);
		gameSpeeds.speeds.push(data.Speeds[i].Speed);

		if (data.Speeds[i].Default)
			gameSpeeds["default"] = i;
	}

	return gameSpeeds;
}


// ====================================================================

// Convert integer color values to string (for use in GUI objects)
function rgbToGuiColor(color, alpha)
{
	var ret;
	if (color && ("r" in color) && ("g" in color) && ("b" in color))
		ret = color.r + " " + color.g + " " + color.b;
	else
		ret = "0 0 0";
	if (alpha)
		ret += " " + alpha;
	return ret;
}

// ====================================================================

/**
 * Convert time in milliseconds to [hh:]mm:ss string representation.
 * @param time Time period in milliseconds (integer)
 * @return String representing time period
 */
function timeToString(time)
{
	if (time < 1000 * 60 * 60)
		var format = translate("mm:ss");
	else
		var format = translate("HH:mm:ss");
	return Engine.FormatMillisecondsIntoDateString(time, format);
}

// ====================================================================

function removeDupes(array)
{
	// loop backwards to make splice operations cheaper
	var i = array.length;
	while (i--)
	{
		if (array.indexOf(array[i]) != i)
			array.splice(i, 1);
	}
}

// ====================================================================
// "Inside-out" implementation of Fisher-Yates shuffle
function shuffleArray(source)
{
	if (!source.length)
		return [];

	var result = [source[0]];
	for (var i = 1; i < source.length; ++i)
	{
		var j = Math.floor(Math.random() * i);
		result[i] = result[j];
		result[j] = source[i];
	}
	return result;
}

// ====================================================================
// Filter out conflicting characters and limit the length of a given name.
// @param name Name to be filtered.
// @param stripUnicode Whether or not to remove unicode characters.
// @param stripSpaces Whether or not to remove whitespace.
function sanitizePlayerName(name, stripUnicode, stripSpaces)
{
	// We delete the '[', ']' characters (GUI tags) and delete the ',' characters (player name separators) by default.
	var sanitizedName = name.replace(/[\[\],]/g, "");
	// Optionally strip unicode
	if (stripUnicode)
		sanitizedName = sanitizedName.replace(/[^\x20-\x7f]/g, "");
	// Optionally strip whitespace
	if (stripSpaces)
		sanitizedName = sanitizedName.replace(/\s/g, "");
	// Limit the length to 20 characters
	return sanitizedName.substr(0,20);
}

function tryAutoComplete(text, autoCompleteList)
{
	if (!text.length)
		return text;

	var wordSplit = text.split(/\s/g);
	if (!wordSplit.length)
		return text;

	var lastWord = wordSplit.pop();
	if (!lastWord.length)
		return text;

	for (var word of autoCompleteList)
	{
		if (word.toLowerCase().indexOf(lastWord.toLowerCase()) != 0)
			continue;
		
		text = wordSplit.join(" ")
		if (text.length > 0)
			text += " ";

		text += word;
		break;
	}
	return text;
}

function autoCompleteNick(guiName, playerList)
{
	var input = Engine.GetGUIObjectByName(guiName);
	var text = input.caption;
	if (!text.length)
		return;

	var autoCompleteList = [];
	for (var player of playerList)
		autoCompleteList.push(player.name);

	var bufferPosition = input.buffer_position;
	var textTillBufferPosition = text.substring(0, bufferPosition);
	var newText = tryAutoComplete(textTillBufferPosition, autoCompleteList);
	input.caption = newText + text.substring(bufferPosition);
	input.buffer_position = bufferPosition + (newText.length - textTillBufferPosition.length);
}
