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
	var files = buildDirEntList(pathname, "*.xml", true);

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
	var files = buildDirEntList(pathname, "*.json", false);

	// Remove the path and extension from each name, since we just want the filename      
	files = [ n.substring(pathname.length, n.length-5) for each (n in files) ];

	return files;
}


// ====================================================================

// Parse JSON data
function parseJSONData(pathname)
{
	var data = {};
		
	var rawData = readFile(pathname);
	if (!rawData)
	{
		error("Failed to read file: "+pathname);
	}
	else
	{
		try
		{	// Catch nasty errors from JSON parsing
			// TODO: Need more info from the parser on why it failed: line number, position, etc!
			data = JSON.parse(rawData);
			if (!data)
				error("Failed to parse JSON data in: "+pathname+" (check for valid JSON data)");
			
			
		}
		catch(err)
		{
			error(err.toString()+": parsing JSON data in "+pathname);
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

// Load default player data, for when it's not otherwise specified
function initPlayerDefaults()
{
	var filename = "simulation/data/player_defaults.json";
	var defaults = [];
	var rawData = readFile(filename);
	if (!rawData)
		error("Failed to read player defaults file: "+filename);
	
	try
	{	// Catch nasty errors from JSON parsing
		// TODO: Need more info from the parser on why it failed: line number, position, etc!
		var data = JSON.parse(rawData);
		if (!data || !data.PlayerData)
			error("Failed to parse player defaults in: "+filename+" (check for valid JSON data)");
		
		defaults = data.PlayerData;
	}
	catch(err)
	{
		error(err.toString()+": parsing player defaults in "+filename);
	}
	
	return defaults;
}

// ====================================================================

// Load map size data
function initMapSizes()
{
	var filename = "simulation/data/map_sizes.json";
	var sizes = {
		names: [],
		tiles: [],
		default: 0
	};
	var rawData = readFile(filename);
	if (!rawData)
		error("Failed to read map sizes file: "+filename);
	
	try
	{	// Catch nasty errors from JSON parsing
		// TODO: Need more info from the parser on why it failed: line number, position, etc!
		var data = JSON.parse(rawData);
		if (!data || !data.Sizes)
			error("Failed to parse map sizes in: "+filename+" (check for valid JSON data)");
		
		for (var i = 0; i < data.Sizes.length; ++i)
		{
			sizes.names.push(data.Sizes[i].LongName);
			sizes.tiles.push(data.Sizes[i].Tiles);
			
			if (data.Sizes[i].Default)
				sizes.default = i;
		}
	}
	catch(err)
	{
		error(err.toString()+": parsing map sizes in "+filename);
	}
	
	return sizes;
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
