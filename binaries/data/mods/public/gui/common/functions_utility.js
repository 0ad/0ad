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

// Parse JSON data
function parseJSONData(pathname)
{
	var data = {};

	var rawData = Engine.ReadFile(pathname);
	if (!rawData)
	{
		error(sprintf("Failed to read file: %(path)s", { path: pathname }));
	}
	else
	{
		try
		{	// Catch nasty errors from JSON parsing
			// TODO: Need more info from the parser on why it failed: line number, position, etc!
			data = JSON.parse(rawData);
			if (!data)
				error(sprintf("Failed to parse JSON data in: %(path)s (check for valid JSON data)", { path: pathname }));
		}
		catch(err)
		{
			error(sprintf("%(error)s: parsing JSON data in %(path)s", { error: err.toString(), path: pathname }));
		}
	}

	return data;
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

// Escape text tags and whitespace, so users can't use special formatting in their chats
// Limit string length to 256 characters
function escapeText(text)
{
	if (!text)
		return text;

	var out = text.replace(/[\[\]]+/g,"");
	out = out.replace(/\s+/g, " ");

	return out.substr(0, 255);

}

// ====================================================================

function toTitleCase (string)
{
	if (!string)
		return string;

	// Returns the title-case version of a given string.
	string = string.toString();
	string = string[0].toUpperCase() + string.substring(1).toLowerCase();

	return string;
}

// ====================================================================

// Parse and return JSON data from file in simulation/data/*
// returns valid object or undefined on error
function parseJSONFromDataFile(filename)
{
	var path = "simulation/data/"+filename;
	var rawData = Engine.ReadFile(path);
	if (!rawData)
		error(sprintf("Failed to read file: %(path)s", { path: path }));

	try
	{
		// Catch nasty errors from JSON parsing
		// TODO: Need more info from the parser on why it failed: line number, position, etc!
		var data = JSON.parse(rawData);
		return data;
	}
	catch(err)
	{
		error(sprintf("%(error)s: parsing JSON data in %(path)s", { error: err.toString(), path: path }));
	}

	return undefined;
}

// ====================================================================

// Load default player data, for when it's not otherwise specified
function initPlayerDefaults()
{
	var defaults = [];

	var data = parseJSONFromDataFile("player_defaults.json");
	if (!data || !data.PlayerData)
		error("Failed to parse player defaults in player_defaults.json (check for valid JSON data)");
	else
		defaults = data.PlayerData;

	return defaults;
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

	var data = parseJSONFromDataFile("map_sizes.json");
	if (!data || !data.Sizes)
		error("Failed to parse map sizes in map_sizes.json (check for valid JSON data)");
	else
	{
		translateObjectKeys(data, ["Name", "LongName"]);
		for (var i = 0; i < data.Sizes.length; ++i)
		{
			sizes.shortNames.push(data.Sizes[i].Name);
			sizes.names.push(data.Sizes[i].LongName);
			sizes.tiles.push(data.Sizes[i].Tiles);

			if (data.Sizes[i].Default)
				sizes["default"] = i;
		}
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

	var data = parseJSONFromDataFile("game_speeds.json");
	if (!data || !data.Speeds)
		error("Failed to parse game speeds in game_speeds.json (check for valid JSON data)");
	else
	{
		translateObjectKeys(data, ["Name"]);
		for (var i = 0; i < data.Speeds.length; ++i)
		{
			gameSpeeds.names.push(data.Speeds[i].Name);
			gameSpeeds.speeds.push(data.Speeds[i].Speed);

			if (data.Speeds[i].Default)
				gameSpeeds["default"] = i;
		}
	}

	return gameSpeeds;
}


// ====================================================================

// Convert integer color values to string (for use in GUI objects)
function iColorToString(color)
{
	var string = "0 0 0";
	if (color && ("r" in color) && ("g" in color) && ("b" in color))
		string = color.r + " " + color.g + " " + color.b;

	return string;
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
	for (var i = 0; i < array.length; i++)
	{
		if (array.indexOf(array[i]) < i)
		{
			array.splice(i, 1);
			i--;
		}
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
