/**
 * Used by notifyUser() to limit the number of pings
 */
var g_LastNickNotification = -1;

function getRandom(randomMin, randomMax)
{
	// Returns a random whole number in a min..max range.
	// NOTE: There should probably be an engine function for this,
	// since we'd need to keep track of random seeds for replays.

	var randomNum = randomMin + (randomMax-randomMin)*Math.random();  // num is random, from A to B
	return Math.round(randomNum);
}

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
	var files = Engine.BuildDirEntList(pathname, "*.json", false);

	// Remove the path and extension from each name, since we just want the filename
	files = [ n.substring(pathname.length, n.length-5) for each (n in files) ];

	return files;
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
 * Returns map description and preview image or placeholder.
 */
function getMapDescriptionAndPreview(mapType, mapName)
{
	var mapData;
	if (mapType == "random" && mapName == "random")
		mapData = { "settings": { "Description": translate("A randomly selected map.") } };
	else if (mapType == "random" && Engine.FileExists(mapName + ".json"))
		mapData = Engine.ReadJSONFile(mapName + ".json");
	else if (Engine.FileExists(mapName + ".xml"))
		mapData = Engine.LoadMapSettings(mapName + ".xml");

	return {
		"description": mapData && mapData.settings && mapData.settings.Description ? translate(mapData.settings.Description) : translate("Sorry, no description available."),
		"preview": mapData && mapData.settings && mapData.settings.Preview ? mapData.settings.Preview : "nopreview.png"
	};
}

/**
 * Sets the mappreview image correctly.
 * It needs to be cropped as the engine only allows loading square textures.
 *
 * @param {string} guiObject
 * @param {string} filename
 */
function setMapPreviewImage(guiObject, filename)
{
	Engine.GetGUIObjectByName(guiObject).sprite = "cropped:" + 400/512+ "," + 300/512 + ":session/icons/mappreview/" + filename;
}

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

	let timeNow = new Date().getTime();

	if (!g_LastNickNotification || timeNow > g_LastNickNotification + 3000)
		Engine.PlayUISound("audio/interface/ui/chat_alert.ogg", false);

	g_LastNickNotification = timeNow;
}

/**
 * Returns a formatted string describing the player assignments.
 * Needs g_CivData to translate!
 *
 * @param {object} playerDataArray - As known from gamesetup and simstate.
 * @param {(string[]|false)} playerStates - One of "won", "defeated", "active" for each player.
 * @returns {string}
 */
function formatPlayerInfo(playerDataArray, playerStates)
{
	let playerDescriptions = {};
	let playerIdx = 0;

	for (let playerData of playerDataArray)
	{
		if (playerData == null || playerData.Civ && playerData.Civ == "gaia")
			continue;

		++playerIdx;
		let teamIdx = playerData.Team;
		let isAI = playerData.AI && playerData.AI != "";
		let playerState = playerStates && playerStates[playerIdx] || playerData.State;
		let isActive = !playerState || playerState == "active";

		let playerDescription;
		if (isAI)
		{
			if (playerData.Civ)
			{
				if (isActive)
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(civ)s, %(AIdifficulty)s %(AIname)s)");
				else
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(civ)s, %(AIdifficulty)s %(AIname)s, %(state)s)");
			}
			else
			{
				if (isActive)
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(AIdifficulty)s %(AIname)s)");
				else
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(AIdifficulty)s %(AIname)s, %(state)s)");
			}
		}
		else
		{
			if (playerData.Offline)
			{
				// Can only occur in the lobby for now, so no strings with civ needed
				if (isActive)
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (OFFLINE)");
				else
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (OFFLINE, %(state)s)");
			}
			else
			{
				if (playerData.Civ)
					if (isActive)
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s (%(civ)s)");
					else
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s (%(civ)s, %(state)s)");
				else
					if (isActive)
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s");
					else
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s (%(state)s)");
			}
		}

		// Sort player descriptions by team
		if (!playerDescriptions[teamIdx])
			playerDescriptions[teamIdx] = [];

		playerDescriptions[teamIdx].push(sprintf(playerDescription, {
			"playerName":
				'[color="' +
				(typeof getPlayerColor == 'function' ?
					(isAI ? "white" : getPlayerColor(playerData.Name)) :
					rgbToGuiColor(playerData.Color || g_Settings.PlayerDefaults[playerIdx].Color)) +
				'"]' + escapeText(playerData.Name) + "[/color]",

			"civ":
				!playerData.Civ ?
					translate("Unknown Civilization") :
						g_CivData && g_CivData[playerData.Civ] && g_CivData[playerData.Civ].Name ?
						translate(g_CivData[playerData.Civ].Name) :
						playerData.Civ,

			"state":
				playerState == "defeated" ?
					translateWithContext("playerstate", "defeated") :
					translateWithContext("playerstate", "won"),

			"AIname": isAI ? translateAIName(playerData.AI) : "",
			"AIdifficulty": isAI ? translateAIDifficulty(playerData.AIDiff) : ""
		}));
	}

	let teams = Object.keys(playerDescriptions);
	if (teams.indexOf("observer") > -1)
		teams.splice(teams.indexOf("observer"), 1);

	let teamDescription = [];

	// If there are no teams, merge all playersDescriptions
	if (teams.length == 1)
		teamDescription.push(playerDescriptions[teams[0]].join("\n"));

	// If there are teams, merge "Team N:" + playerDescriptions
	else
		teamDescription = teams.map(team => {

			let teamCaption = team == -1 ?
				translate("No Team") :
				sprintf(translate("Team %(team)s"), { "team": +team + 1 });

			// Translation: Describe players of one team in a selected game, f.e. in the replay- or savegame menu or lobby
			return sprintf(translate("%(team)s:\n%(playerDescriptions)s"), {
				"team": '[font="sans-bold-14"]' + teamCaption + "[/font]",
				"playerDescriptions": playerDescriptions[team].join("\n")
			});
		});

	if (playerDescriptions.observer)
		teamDescription.push(sprintf(translate("%(team)s:\n%(playerDescriptions)s"), {
			"team": '[font="sans-bold-14"]' + translatePlural("Observer", "Observers", playerDescriptions.observer.length) + "[/font]",
			"playerDescriptions": playerDescriptions.observer.join("\n")
		}));

	return teamDescription.join("\n\n");
}
