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

function parseDelimiterString (parseString, delimiter) 
{ 
	// Seeks through the delimiters in a string and populates the elements of an array with fields found between them. 

	// Declare local variables. 
	var parseLoop = 0; 
	var parseElement = 0; 
	var seekDelimiter = 0; 
	var parseArray = new Array(); 

	// While we're still within the bounds of the string, 
	while (parseLoop <= parseString.length) 
	{ 
		// Seek until we find a delimiter. 
		seekDelimiter = parseLoop; 
		while (parseString[seekDelimiter] != delimiter && seekDelimiter <= parseString.length) 
			seekDelimiter++; 

		// If we found a delimiter within the string, 
		if (seekDelimiter != parseString.length) 
		{ 
			// Store sub-string between start point and delimiter in array element. 
			parseArray[parseElement] = parseString.substring(parseLoop, seekDelimiter); 
			parseElement++; 
		} 

		// Move to after delimiter position for next seek. 
		parseLoop = seekDelimiter+1; 
	} 

	// Store length of array. 
	parseArray.length = parseElement; 

	return parseArray; 
}

// ====================================================================

// Get list of XML files in pathname excepting those starting with _
function getXMLFileList(pathname)
{
	var files = buildDirEntList(pathname, "*.xml", false);

	// Remove the path and extension from each name, since we just want the filename      
	files = [ n.substring(pathname.length, n.length-4) for each (n in files) ];

	// Remove any files starting with "_" (these are for special maps used by the engine/editor)
	files = [ n for each (n in files) if (n[0] != "_") ];

	return files;
}

// ====================================================================

// Get list of JSON files in pathname excepting those starting with _
function getJSONFileList(pathname)
{
	var files = buildDirEntList(pathname, "*.json", false);

	// Remove the path and extension from each name, since we just want the filename      
	files = [ n.substring(pathname.length, n.length-5) for each (n in files) ];

	// Remove any files starting with "_" (these are for special maps used by the engine/editor)
	files = [ n for each (n in files) if (n[0] != "_") ];

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

// A sorting function for arrays, that ignores case
function sortIgnoreCase (x, y)
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

function addArrayElement(Array) 
{ 
	// Adds an element to an array, updates its given index, and returns the index of the element. 

	Array[Array.last] = new Object(); 
	Array.last++; 

	return (Array.last - 1); 
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
	var defaults = [];
	var rawData = readFile("simulation/data/player_defaults.json");
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

// Convert integer color values to string (for use in GUI objects)
function iColorToString(color)
{
	var string = "0 0 0";
	if(color.r && color.g && color.b)
		string = color.r + " " + color.g + " " + color.b;
	
	return string;
}

// ====================================================================