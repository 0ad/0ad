/**
 * Used by notifyUser() to limit the number of pings
 */
var g_LastNickNotification = -1;

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

function getJSONFileList(pathname)
{
	// Remove the path and extension from each name, since we just want the filename
	return Engine.BuildDirEntList(pathname, "*.json", false).map(
		filename => filename.substring(pathname.length, filename.length-5));
}

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

/**
 * Escape tag start and escape characters, so users cannot use special formatting.
 * Also limit string length to 256 characters (not counting escape characters).
 */
function escapeText(text, limitLength = true)
{
	if (!text)
		return text;

	if (limitLength)
		text = text.substr(0, 255);

	return text.replace(/\\/g, "\\\\").replace(/\[/g, "\\[");
}

function unescapeText(text)
{
	if (!text)
		return text;
	return text.replace(/\\\\/g, "\\").replace(/\\\[/g, "\[");
}

/**
 * Merge players by team to remove duplicate Team entries, thus reducing the packet size of the lobby report.
 */
function playerDataToStringifiedTeamList(playerData)
{
	let teamList = {};

	for (let pData of playerData)
	{
		let team = pData.Team === undefined ? -1 : pData.Team;
		if (!teamList[team])
			teamList[team] = [];
		teamList[team].push(pData);
		delete teamList[team].Team;
	}

	return escapeText(JSON.stringify(teamList), false);
}

function stringifiedTeamListToPlayerData(stringifiedTeamList)
{
	let teamList = JSON.parse(unescapeText(stringifiedTeamList));
	let playerData = [];

	for (let team in teamList)
		for (let pData of teamList[team])
		{
			pData.Team = team;
			playerData.push(pData);
		}

	return playerData;
}

function translateMapTitle(mapTitle)
{
	return mapTitle == "random" ? translateWithContext("map selection", "Random") : translate(mapTitle);
}

/**
 * Convert time in milliseconds to [hh:]mm:ss string representation.
 * @param time Time period in milliseconds (integer)
 * @return String representing time period
 */
function timeToString(time)
{
	return Engine.FormatMillisecondsIntoDateStringGMT(time, time < 1000 * 60 * 60 ?
		translate("mm:ss") : translate("HH:mm:ss"));
}

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

function singleplayerName()
{
	return Engine.ConfigDB_GetValue("user", "playername.singleplayer") || Engine.GetSystemUsername();
}

function multiplayerName()
{
	return Engine.ConfigDB_GetValue("user", "playername.multiplayer") || Engine.GetSystemUsername();
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

		text = wordSplit.join(" ");
		if (text.length > 0)
			text += " ";

		text += word;
		break;
	}
	return text;
}

function autoCompleteNick(guiObject, playernames)
{
	let text = guiObject.caption;
	if (!text.length)
		return;

	let bufferPosition = guiObject.buffer_position;
	let textTillBufferPosition = text.substring(0, bufferPosition);
	let newText = tryAutoComplete(textTillBufferPosition, playernames);

	guiObject.caption = newText + text.substring(bufferPosition);
	guiObject.buffer_position = bufferPosition + (newText.length - textTillBufferPosition.length);
}

function clearChatMessages()
{
	g_ChatMessages.length = 0;
	Engine.GetGUIObjectByName("chatText").caption = "";

	try {
		for (let timer of g_ChatTimers)
			clearTimeout(timer);
		g_ChatTimers.length = 0;
	} catch (e) {
	}
}

/**
 * Plays a sound if user's nick is mentioned in chat
 */
function notifyUser(userName, msgText)
{
	if (Engine.ConfigDB_GetValue("user", "sound.notify.nick") != "true" ||
	    msgText.toLowerCase().indexOf(userName.toLowerCase()) == -1)
		return;

	let timeNow = Date.now();

	if (!g_LastNickNotification || timeNow > g_LastNickNotification + 3000)
		Engine.PlayUISound("audio/interface/ui/chat_alert.ogg", false);

	g_LastNickNotification = timeNow;
}

/**
 * Horizontally spaces objects within a parent
 *
 * @param margin The gap, in px, between the objects
 */
function horizontallySpaceObjects(parentName, margin=0)
{
	let objects = Engine.GetGUIObjectByName(parentName).children;
	for (let i = 0; i < objects.length; ++i)
	{
		let size = objects[i].size;
		let width = size.right - size.left;
		size.left = i * (width + margin) + margin;
		size.right = (i + 1) * (width + margin);
		objects[i].size = size;
	}
}

/**
 * Hide all children after a certain index
 */
function hideRemaining(parentName, start = 0)
{
	let objects = Engine.GetGUIObjectByName(parentName).children;

	for (let i = start; i < objects.length; ++i)
		objects[i].hidden = true;
}
