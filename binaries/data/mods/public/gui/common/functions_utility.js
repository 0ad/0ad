/**
 * Used for acoustic GUI notifications.
 * Define the soundfile paths and specific time thresholds (avoid spam).
 * And store the timestamp of last interaction for each notification.
 */
var g_SoundNotifications = {
	"nick": { "soundfile": "audio/interface/ui/chat_alert.ogg", "threshold": 3000 }
};

// Get list of XML files in pathname with recursion, excepting those starting with _
function getXMLFileList(pathname)
{
	var files = Engine.BuildDirEntList(pathname, "*.xml", true);

	var result = [];

	// Get only subpath from filename and discard extension
	for (var i = 0; i < files.length; ++i)
	{
		var file = files[i];
		file = file.substring(pathname.length, file.length - 4);

		// Split path into directories so we can check for beginning _ character
		var tokens = file.split("/");

		if (tokens[tokens.length - 1][0] != "_")
			result.push(file);
	}

	return result;
}

function getJSONFileList(pathname)
{
	// Remove the path and extension from each name, since we just want the filename
	return Engine.BuildDirEntList(pathname, "*.json", false).map(
		filename => filename.substring(pathname.length, filename.length - 5));
}

// A sorting function for arrays of objects with 'name' properties, ignoring case
function sortNameIgnoreCase(x, y)
{
	let lowerX = x.name.toLowerCase();
	let lowerY = y.name.toLowerCase();

	if (lowerX < lowerY)
		return -1;
	if (lowerX > lowerY)
		return 1;
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
	let i = array.length;
	while (i--)
		if (array.indexOf(array[i]) != i)
			array.splice(i, 1);
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
 * Manage acoustic GUI notifications.
 *
 * @param {string} type - Notification type.
 */
function soundNotification(type)
{
	if (Engine.ConfigDB_GetValue("user", "sound.notify." + type) != "true")
		return;

	let notificationType = g_SoundNotifications[type];
	let timeNow = Date.now();

	if (!notificationType.lastInteractionTime || timeNow > notificationType.lastInteractionTime + notificationType.threshold)
		Engine.PlayUISound(notificationType.soundfile, false);

	notificationType.lastInteractionTime = timeNow;
}

/**
 * Horizontally spaces objects within a parent
 *
 * @param margin The gap, in px, between the objects
 */
function horizontallySpaceObjects(parentName, margin = 0)
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
