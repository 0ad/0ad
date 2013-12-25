var g_ChatMessages = [];
var g_Name = "unknown";
var g_GameList = {};
var g_specialKey = Math.random();
var g_spamMonitor = {};
var g_spammers = {};
var g_timestamp = Engine.ConfigDB_GetValue("user", "lobby.chattimestamp") == "true";
var g_mapSizes = {};

////////////////////////////////////////////////////////////////////////////////////////////////

function init(attribs)
{
	// Play menu music
	initMusic();
	global.music.setState(global.music.states.MENU);

	g_Name = Engine.LobbyGetNick();

	g_mapSizes = initMapSizes();
	g_mapSizes.shortNames.push("Any");
	g_mapSizes.tiles.push("");

	var mapSizeFilter = getGUIObjectByName("mapSizeFilter");
	mapSizeFilter.list = g_mapSizes.shortNames;
	mapSizeFilter.list_data = g_mapSizes.tiles;

	var playersNumberFilter = getGUIObjectByName("playersNumberFilter");
	playersNumberFilter.list = [2,3,4,5,6,7,8,"Any"];
	playersNumberFilter.list_data = [2,3,4,5,6,7,8,""];

	var mapTypeFilter = getGUIObjectByName("mapTypeFilter");
	mapTypeFilter.list = ["Skirmish", "Random", "Scenario", "Any"];
	mapTypeFilter.list_data = ["skirmish", "random", "scenario", ""];

	Engine.LobbySetPlayerPresence("available");
	Engine.SendGetGameList();
	Engine.SendGetBoardList();
	updatePlayerList();

	resetFilters();
	setTimeout(clearSpamMonitor, 5000);
	setTimeout(clearSpammers, 30000);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Xmpp client connection management
////////////////////////////////////////////////////////////////////////////////////////////////


function lobbyStop()
{
	Engine.StopXmppClient();
}

function lobbyConnect()
{
	Engine.ConnectXmppClient();
}

function lobbyDisconnect()
{
	Engine.DisconnectXmppClient();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Server requests functions
////////////////////////////////////////////////////////////////////////////////////////////////

function lobbyRefreshGameList()
{
	Engine.SendGetGameList();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Update functions
////////////////////////////////////////////////////////////////////////////////////////////////

function resetFilters()
{
	// Reset states of gui objects
	getGUIObjectByName("mapSizeFilter").selected = getGUIObjectByName("mapSizeFilter").list.length - 1;
	getGUIObjectByName("playersNumberFilter").selected = getGUIObjectByName("playersNumberFilter").list.length - 1;
	getGUIObjectByName("mapTypeFilter").selected = getGUIObjectByName("mapTypeFilter").list.length - 1;
	getGUIObjectByName("showFullFilter").checked = false;

	// Update the list of games
	updateGameList();

	// Update info box about the game currently selected
	selectGame(getGUIObjectByName("gamesBox").selected);
}

function applyFilters()
{
	// Update the list of games
	updateGameList();

	// Update info box about the game currently selected
	selectGame(getGUIObjectByName("gamesBox").selected);
}

function displayGame(g, mapSizeFilter, playersNumberFilter, mapTypeFilter, showFullFilter)
{
	if(mapSizeFilter != "" && g.mapSize != mapSizeFilter) return false;
	if(playersNumberFilter != "" && g.tnbp != playersNumberFilter) return false;
	if(mapTypeFilter != "" && g.mapType != mapTypeFilter) return false;
	if(!showFullFilter && g.tnbp == g.nbp) return false;

	return true;
}

// Do a full update of the player listing **Only call on init**
function updatePlayerList()
{
	var playersBox = getGUIObjectByName("playersBox");
	[playerList, presenceList, nickList] = [[],[],[]];
	for each (var p in Engine.GetPlayerList())
	{
		var [name, status] = formatPlayerListEntry(p.name, p.presence);
		playerList.push(name);
		presenceList.push(status);
		nickList.push(p.name);
	}
	playersBox.list_name = playerList;
	playersBox.list_status = presenceList;
	playersBox.list = nickList;
	if (playersBox.selected >= playersBox.list.length)
		playersBox.selected = -1;
	return [playerList, presenceList, nickList];
}

// Update leaderboard listing
function updateBoardList()
{
	// Get list from C++
	var boardList = Engine.GetBoardList();
	// Get GUI leaderboard object
	var leaderboard = getGUIObjectByName("leaderboardBox");
	// Sort list in acending order by rating
	boardList.sort(function(a, b) b.rating - a.rating);

	var list = [];
	var list_name = [];
	var list_rank = [];
	var list_rating = [];

	// Push changes
	for (var i = 0; i < boardList.length; i++)
	{
		list_name.push(boardList[i].name);
		list_rating.push(boardList[i].rating);
		list_rank.push(i+1);
		list.push(boardList[i].name);
	}

	leaderboard.list_name = list_name;
	leaderboard.list_rating = list_rating;
	leaderboard.list_rank = list_rank;
	leaderboard.list = list;

	if (leaderboard.selected >= leaderboard.list.length)
		leaderboard.selected = -1;
}

// Update game listing
function updateGameList()
{
	var gamesBox = getGUIObjectByName("gamesBox");
	var gameList = Engine.GetGameList();
	// Store the game whole game list data so that we can access it later
	// to update the game info panel.
	g_GameList = gameList;

	// Sort the list of games to that games 'waiting' are displayed at the top
	g_GameList.sort(function (a,b) {
		return a.state == 'waiting' ? -1 : b.state == 'waiting' ? +1 : 0;
	});

	var list_name = [];
	var list_ip = [];
	var list_mapName = [];
	var list_mapSize = [];
	var list_mapType = [];
	var list_nPlayers = [];
	var list = [];
	var list_data = [];

	var mapSizeFilterDD = getGUIObjectByName("mapSizeFilter");
	var playersNumberFilterDD = getGUIObjectByName("playersNumberFilter");
	var mapTypeFilterDD = getGUIObjectByName("mapTypeFilter");
	var showFullFilterCB = getGUIObjectByName("showFullFilter");

	// Get filter values
	var mapSizeFilter = mapSizeFilterDD.selected >= 0 ? mapSizeFilterDD.list_data[mapSizeFilterDD.selected] : "";
	var playersNumberFilter = playersNumberFilterDD.selected >=0 ? playersNumberFilterDD.list_data[playersNumberFilterDD.selected] : "";
	var mapTypeFilter = mapTypeFilterDD.selected >= 0 ? mapTypeFilterDD.list_data[mapTypeFilterDD.selected] : "";
	var showFullFilter = showFullFilterCB.checked;

	var c = 0;
	for each (g in gameList)
	{
		if(displayGame(g, mapSizeFilter, playersNumberFilter, mapTypeFilter, showFullFilter))
		{
			// Highlight games 'waiting' for this player, otherwise display as green
			var name = (g.state != 'waiting') ? '[color="0 125 0"]' + g.name + '[/color]' : '[color="orange"]' + g.name + '[/color]';
			list_name.push(name);
			list_ip.push(g.ip);
			list_mapName.push(g.niceMapName);
			list_mapSize.push(g.mapSize.split("(")[0]);
			list_mapType.push(toTitleCase(g.mapType));
			list_nPlayers.push(g.nbp + "/" +g.tnbp);
			list.push(g.name);
			list_data.push(c);
		}
		c++;
	}

	gamesBox.list_name = list_name;
	//gamesBox.list_ip = list_ip;
	gamesBox.list_mapName = list_mapName;
	gamesBox.list_mapSize = list_mapSize;
	gamesBox.list_mapType = list_mapType;
	gamesBox.list_nPlayers = list_nPlayers;
	gamesBox.list = list;
	gamesBox.list_data = list_data;

	if (gamesBox.selected >= gamesBox.list_name.length)
		gamesBox.selected = -1;

	// If game selected, update info box about the game.
	if(getGUIObjectByName("gamesBox").selected != -1)
		selectGame(getGUIObjectByName("gamesBox").selected)
}

// The following function colorizes and formats the entries in the player list.
function formatPlayerListEntry(nickname, presence)
{
	// Set colors based on player status
	var color_close = '[/color]';
	switch (presence)
	{
	case "playing":
		var color = '[color="125 0 0"]';
		var status = color + "Busy" + color_close;
		break;
	case "away":
		var color = '[color="229 76 13"]';
		var status = color + "Away" + color_close;
		break;
	case "available":
		var color = '[color="0 125 0"]';
		var status = color + "Online" + color_close;
		break;
	case "offline":
		var color = '[color="0 0 0"]';
		var status = color + "Offline" + color_close;
		break;
	default:
		warn("Unknown presence '"+presence+"'");
		break;
	}

	var name = colorPlayerName(nickname);

	// Push this player's name and status onto the list
	return [name, status];
}

function selectGame(selected)
{
	if (selected == -1)
	{
		// Hide the game info panel if a game is not selected
		getGUIObjectByName("gameInfo").hidden = true;
		getGUIObjectByName("gameInfoEmpty").hidden = false;
		getGUIObjectByName("joinGameButton").hidden = true;
		return;
	}

	var mapData;
	var g = getGUIObjectByName("gamesBox").list_data[selected];

	// Load map data
	if (g_GameList[g].mapType == "random" && g_GameList[g].mapName == "random")
		mapData = {"settings": {"Description": "A randomly selected map."}};
	else if (g_GameList[g].mapType == "random" && fileExists(g_GameList[g].mapName + ".json"))
		mapData = parseJSONData(g_GameList[g].mapName + ".json");
	else if (fileExists(g_GameList[g].mapName + ".xml"))
		mapData = Engine.LoadMapSettings(g_GameList[g].mapName + ".xml");
	else
		// Warn the player if we can't find the map. 
		warn("Map '"+ g_GameList[g].mapName +"'  not found");

	// Show the game info paneland join button.
	getGUIObjectByName("gameInfo").hidden = false;
	getGUIObjectByName("gameInfoEmpty").hidden = true;
	getGUIObjectByName("joinGameButton").hidden = false;

	// Display the map name, number of players, the names of the players, the map size and the map type.
	getGUIObjectByName("sgMapName").caption = g_GameList[g].niceMapName;
	getGUIObjectByName("sgNbPlayers").caption = g_GameList[g].nbp + "/" + g_GameList[g].tnbp;
	getGUIObjectByName("sgPlayersNames").caption = g_GameList[g].players;
	getGUIObjectByName("sgMapSize").caption = g_GameList[g].mapSize.split("(")[0];
	getGUIObjectByName("sgMapType").caption = toTitleCase(g_GameList[g].mapType);

	// Display map description if it exists, otherwise display a placeholder.
	if (mapData && mapData.settings.Description)
		var mapDescription = mapData.settings.Description;
	else
		var mapDescription = "Sorry, no description available.";
	
	// Display map preview if it exists, otherwise display a placeholder.
	if (mapData && mapData.settings.Preview)
		var mapPreview = mapData.settings.Preview;
	else
		var mapPreview = "nopreview.png";
	
	getGUIObjectByName("sgMapDescription").caption = mapDescription;
	getGUIObjectByName("sgMapPreview").sprite = "cropped:(0.7812,0.5859)session/icons/mappreview/" + mapPreview;
}

function joinSelectedGame()
{
	var gamesBox = getGUIObjectByName("gamesBox");
	if (gamesBox.selected >= 0)
	{
		var g = gamesBox.list_data[gamesBox.selected];
		var sname = g_Name;
		var sip = g_GameList[g].ip;

		// TODO: What about valid host names?
		// Check if it looks like an ip address
		if (sip.split('.').length != 4)
		{
			addChatMessage({ "from": "system", "text": "This game does not have a valid address" });
			return;
		}

		// Open Multiplayer connection window with join option.
		Engine.PushGuiPage("page_gamesetup_mp.xml", { multiplayerGameType: "join", name: sname, ip: sip });
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Utils
////////////////////////////////////////////////////////////////////////////////////////////////

function twoDigits(n)
{
	return n < 10 ? "0" + n : n;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// GUI event handlers
////////////////////////////////////////////////////////////////////////////////////////////////

function onTick()
{
	// Wake up XmppClient
	Engine.RecvXmppClient();

	updateTimers();

	// Receive messages
	while (true)
	{
		var message = Engine.LobbyGuiPollMessage();
		// Clean Message
		if (!message)
			break;
		message.from = escapeText(message.from);
		message.text = escapeText(message.text);
		switch (message.type)
		{
		case "mucmessage": // For room messages
			addChatMessage({ "from": message.from, "text": message.text });
			break;
		case "message": // For private messages
			addChatMessage({ "from": message.from, "text": message.text });
			break;
		case "muc":
			var nick = message.text;
			var presence = Engine.LobbyGetPlayerPresence(nick);
			var playersBox = getGUIObjectByName("playersBox");
			var playerList = playersBox.list_name;
			var presenceList = playersBox.list_status;
			var nickList = playersBox.list;
			var nickIndex = nickList.indexOf(nick);
			switch(message.level)
			{
			case "join":
				if (nick == g_Name)
				{
					// We just joined, we need to get the full player list
					[playerList, presenceList, nickList] = updatePlayerList();
					break;
				}
				var [name, status] = formatPlayerListEntry(nick, presence);
				playerList.push(name);
				presenceList.push(status);
				nickList.push(nick);
				addChatMessage({ "text": "/special " + nick + " has joined.", "key": g_specialKey });
				break;
			case "leave":
				if (nickIndex == -1) // Left, but not present (TODO: warn about this?)
					break;
				playerList.splice(nickIndex, 1);
				presenceList.splice(nickIndex, 1);
				nickList.splice(nickIndex, 1);
				addChatMessage({ "text": "/special " + nick + " has left.", "key": g_specialKey });
				break;
			case "nick":
				if (nickIndex == -1) // This shouldn't ever happen
					break;
				if (!isValidNick(message.data))
				{
					addChatMessage({ "from": "system", "text": "Invalid nickname: " + message.data });
					break;
				}
				var [name, status] = formatPlayerListEntry(message.data, presence); // TODO: actually we don't want to change the presence here, so use what was used before
				playerList[nickIndex] = name;
				// presence stays the same
				nickList[nickIndex] = message.data;
				addChatMessage({ "text": "/special " + nick + " is now known as " + message.data + ".", "key": g_specialKey });
				break;
			case "presence":
				if (nickIndex == -1) // This shouldn't ever happen
					break;
				var [name, status] = formatPlayerListEntry(nick, presence);
				presenceList[nickIndex] = status;
				playerList[nickIndex] = name;
				break;
			default:
				warn("Unknown message.level '" + message.level + "'");
				break;
			}
			// Push new data to GUI
			playersBox.list_name = playerList;
			playersBox.list_status = presenceList;
			playersBox.list = nickList;
			if (playersBox.selected >= playersBox.list.length)
				playersBox.selected = -1;
			break;
		case "system":
			switch (message.level)
			{
			case "standard":
				addChatMessage({ "from": "system", "text": message.text, "color": "150 0 0" });
				if (message.text == "disconnected")
				{
					// Clear the list of games and the list of players
					updateGameList();
					updateBoardList();
					updatePlayerList();
					// Disable the 'host' button
					getGUIObjectByName("hostButton").enabled = false;
				}
				else if (message.text == "connected")
				{
					getGUIObjectByName("hostButton").enabled = true;
				}
				break;
			case "error":
				addChatMessage({ "from": "system", "text": message.text, "color": "150 0 0" });
				break;
			case "internal":
				switch (message.text)
				{
				case "gamelist updated":
					updateGameList();
					break;
				case "boardlist updated":
					updateBoardList();
					break;
				}
				break
			}
			break;
		default:
			error("Unrecognised message type "+message.type);
		}
	}
}

/* Messages */
function submitChatInput()
{
	var input = getGUIObjectByName("chatInput");
	var text = escapeText(input.caption);
	if (text.length)
	{
		if (!handleSpecialCommand(text))
			Engine.LobbySendMessage(text);
		input.caption = "";
	}
}

function completeNick()
{
	var input = getGUIObjectByName("chatInput");
	var text = escapeText(input.caption);
	if (text.length)
	{
		var matched = false;
		for each (var playerObj in Engine.GetPlayerList()) {
			var player = playerObj.name;
			var breaks = text.match(/(\s+)/g) || [];
			text.split(/\s+/g).reduceRight(function (wordsSoFar, word, index) {
				if (matched)
					return null;
				var matchCandidate = word + (breaks[index - 1] || "") + wordsSoFar;
				if (player.toUpperCase().indexOf(matchCandidate.toUpperCase().trim()) == 0) {
					input.caption = text.replace(matchCandidate.trim(), player);
					matched = true;
				}
				return matchCandidate;
			}, "");
			if (matched)
				break;
		}
	}
}

function isValidNick(nick)
{
	var prohibitedNicks = ["system"];
	return prohibitedNicks.indexOf(nick) == -1;
}

function handleSpecialCommand(text)
{
	if (text[0] != '/')
		return false;

	var [cmd, nick] = ircSplit(text);

	switch (cmd)
	{
	case "away":
		// TODO: Should we handle away messages?
		Engine.LobbySetPlayerPresence("away");
		break;
	case "back":
		Engine.LobbySetPlayerPresence("available");
		break;
	case "nick":
		if (g_spammers[g_Name] != undefined)
			break;
		// Strip invalid characters.
		nick = sanitizePlayerName(nick, true, true);
		Engine.LobbySetNick(nick);
		g_Name = nick;
		break;
	case "kick": // TODO: Split reason from nick and pass it too, for now just support "/kick nick"
			// also allow quoting nicks (and/or prevent users from changing it here, but that doesn't help if the spammer uses a different client)
		Engine.LobbyKick(nick, "");
		break;
	case "ban": // TODO: Split reason from nick and pass it too, for now just support "/ban nick"
		Engine.LobbyBan(nick, "");
		break;
	case "quit":
		lobbyStop();
		Engine.SwitchGuiPage("page_pregame.xml");
		break;
	case "say":
	case "me":
		return false;
	default:
		addChatMessage({ "from":"system", "text":"We're sorry, the '" + cmd + "' command is not supported."});
	}
	return true;
}

function addChatMessage(msg)
{
	// Highlight local user's nick
	if (msg.text.indexOf(g_Name) != -1 && g_Name != msg.from)
		msg.text = msg.text.replace(new RegExp('\\b' + '\\' + g_Name + '\\b', "g"), colorPlayerName(g_Name));

	// Run spam test
	if (updateSpamandDetect(msg.text, msg.from))
		return;

	// Format Text
	var formatted = ircFormat(msg.text, msg.from, msg.color, msg.key);

	// If there is text, add it to the chat box.
	if (formatted)
	{
		g_ChatMessages.push(formatted);
		getGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
	}
}

function ircSplit(string)
{
	var idx = string.indexOf(' ');
	if (idx != -1)
		return [string.substr(1,idx-1), string.substr(idx+1)];
	return [string.substr(1), ""];
}

// The following formats text in an IRC-like way
function ircFormat(text, from, color, key)
{
	// Generate and apply color to uncolored names,
	if (!color && from)
		var coloredFrom = colorPlayerName(from);
	else if (color && from)
		var coloredFrom = '[color="' + color + '"]' + from + "[/color]";

	// Time for optional time header
	var time = new Date(Date.now());

	// Build time header if enabled
	if (g_timestamp)
		var formatted = '[font="serif-bold-13"]\x5B' + twoDigits(time.getHours() % 12) + ":" + twoDigits(time.getMinutes()) + '\x5D[/font] '
	else
		var formatted = "";

	// Handle commands allowed past handleSpecialCommand.
	if (text[0] == '/')
	{
		var [command, message] = ircSplit(text);
		switch (command)
		{
			case "me":
				return formatted + '[font="serif-bold-13"]* ' + coloredFrom + '[/font] ' + message;
			case "say":
				return formatted + '[font="serif-bold-13"]<' + coloredFrom + '>[/font] ' + message;
			case "special":
				if (key === g_specialKey)
					return formatted + '[font="serif-bold-13"] == ' + message + '[/font]';
				break;
			default:
				// This should never happen.
				return "";
		}
	}
	return formatted + '[font="serif-bold-13"]<' + coloredFrom + '>[/font] ' + text;
}

// The following function tracks message stats and returns true if the input text is spam.
function updateSpamandDetect(text, from)
{
	// Check for blank lines.
	if (text == " ")
		return true;
	// Update the spam monitor.
	if (g_spamMonitor[from])
		g_spamMonitor[from]++;
	else
		g_spamMonitor[from] = 1;
	if (g_spamMonitor[from] > 5)
		g_spammers[from] = true
	// Block spammers and notify the player if they are blocked.
	if(from in g_spammers)
	{
		if (from == g_Name)
			addChatMessage({ "from": "system", "text": "Please do not spam. You have been blocked for thirty seconds." });
		return true;
	}
	// Return false if everything is clear.
	return false;
}

// Timers to clear spammers after some time.
function clearSpamMonitor()
{
	g_spamMonitor = {};
	setTimeout(clearSpamMonitor, 5000);
}

function clearSpammers()
{
	g_spammers = {};
	setTimeout(clearSpammers, 30000);
}

/* Utilities */
// Generate a (mostly) unique color for this player based on their name.
// See http://stackoverflow.com/questions/3426404/create-a-hexadecimal-colour-based-on-a-string-with-jquery-javascript
function getPlayerColor(playername)
{
	// Generate a probably-unique hash for the player name and use that to create a color.
	var hash = 0;
	for (var i = 0; i < playername.length; i++)
		hash = playername.charCodeAt(i) + ((hash << 5) - hash);

	// First create the color in RGB then HSL, clamp the lightness so it's not too dark to read, and then convert back to RGB to display.
	// The reason for this roundabout method is this algorithm can generate values from 0 to 255 for RGB but only 0 to 100 for HSL; this gives
	// us much more variety if we generate in RGB. Unfortunately, enforcing that RGB values are a certain lightness is very difficult, so
	// we convert to HSL to do the computation. Since our GUI code only displays RGB colors, we have to convert back.
	var [h, s, l] = rgbToHsl(hash >> 24 & 0xFF, hash >> 16 & 0xFF, hash >> 8 & 0xFF);
	return hslToRgb(h, s, Math.max(0.4, l)).join(" ");
}

function repeatString(times, string) {
	return Array(times + 1).join(string);
}

// Some names are special and should always appear in certain colors.
var fixedColors = { "system": repeatString(7, "255.0.0."), "WFGbot": repeatString(6, "255.24.24."),
					"pyrogenesis": repeatString(2, "97.0.0.") + repeatString(2, "124.0.0.") + "138.0.0." +
						repeatString(2, "174.0.0.") + repeatString(2, "229.40.0.") + repeatString(2, "243.125.15.") };
function colorPlayerName(playername)
{
	var color = fixedColors[playername];
	if (color) {
	color = color.split(".");
	return ('[color="' + playername.split("").map(function (c, i) color.slice(i * 3, i * 3 + 3).join(" ") + '"]' + c + '[/color][color="')
				.join("") + '"]').slice(0, -10);
	}
	return '[color="' + getPlayerColor(playername) + '"]' + playername + '[/color]';
}

// Ensure `value` is between 0 and 1.
function clampColorValue(value)
{
	return Math.abs(1 - Math.abs(value - 1));
}

// See http://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
function rgbToHsl(r, g, b)
{
	r /= 255;
	g /= 255;
	b /= 255;
	var max = Math.max(r, g, b), min = Math.min(r, g, b);
	var h, s, l = (max + min) / 2;

	if (max == min)
		h = s = 0; // achromatic
	else
	{
		var d = max - min;
		s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
		switch (max)
		{
			case r: h = (g - b) / d + (g < b ? 6 : 0); break;
			case g: h = (b - r) / d + 2; break;
			case b: h = (r - g) / d + 4; break;
		}
		h /= 6;
	}

	return [h, s, l];
}

function hslToRgb(h, s, l)
{
	function hue2rgb(p, q, t)
	{
		if (t < 0) t += 1;
		if (t > 1) t -= 1;
		if (t < 1/6) return p + (q - p) * 6 * t;
		if (t < 1/2) return q;
		if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
		return p;
	}

	[h, s, l] = [h, s, l].map(clampColorValue);
	var r, g, b;

	if (s == 0)
		r = g = b = l; // achromatic
	else {
		var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		var p = 2 * l - q;
		r = hue2rgb(p, q, h + 1/3);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1/3);
	}

	return [r, g, b].map(function (n) Math.round(n * 255));
}

(function () {
function hexToRgb(hex) {
	return parseInt(hex.slice(0, 2), 16) + "." + parseInt(hex.slice(2, 4), 16) + "." + parseInt(hex.slice(4, 6), 16) + ".";
}
function r(times, hex) {
	return repeatString(times, hexToRgb(hex));
}

fixedColors["Twilight_Sparkle"] = r(2, "d19fe3") + r(2, "b689c8") + r(2, "a76bc2") +
	r(4, "263773") + r(2, "131f46") + r(2, "662d8a") + r(2, "ed438a");
fixedColors["Applejack"] = r(3, "ffc261") + r(3, "efb05d") + r(3, "f26f31");
fixedColors["Rarity"] = r(1, "ebeff1") + r(1, "dee3e4") + r(1, "bec2c3") +
	r(1, "83509f") + r(1, "4b2568") + r(1, "4917d6");
fixedColors["Rainbow_Dash"] = r(2, "ee4144") + r(1, "f37033") + r(1, "fdf6af") +
	r(1, "62bc4d") + r(1, "1e98d3") + r(2, "672f89") + r(1, "9edbf9") +
	r(1, "88c4eb") + r(1, "77b0e0") + r(1, "1e98d3");
fixedColors["Pinkie_Pie"] = r(2, "f3b6cf") + r(2, "ec9dc4") + r(4, "eb81b4") +
	r(1, "ed458b") + r(1, "be1d77");
fixedColors["Fluttershy"] = r(2, "fdf6af") + r(2, "fee78f") + r(2, "ead463") +
	r(2, "f3b6cf") + r(2, "eb81b4");
fixedColors["Sweetie_Belle"] = r(2, "efedee") + r(3, "e2dee3") + r(3, "cfc8d1") +
	r(2, "b28dc0") + r(2, "f6b8d2") + r(1, "795b8a");
fixedColors["Apple_Bloom"] = r(2, "f4f49b") + r(2, "e7e793") + r(2, "dac582") +
	r(2, "f46091") + r(2, "f8415f") + r(1, "c52451");
fixedColors["Scootaloo"] = r(2, "fbba64") + r(2, "f2ab56") + r(2, "f37003") +
	r(2, "bf5d95") + r(1, "bf1f79");
fixedColors["Luna"] = r(1, "7ca7fa") + r(1, "5d6fc1") + r(1, "656cb9") + r(1, "393993");
fixedColors["Celestia"] = r(1, "fdfafc") + r(1, "f7eaf2") + r(1, "d99ec5") +
	r(1, "00aec5") + r(1, "f7c6dc") + r(1, "98d9ef") + r(1, "ced7ed") + r(1, "fed17b");
})();
