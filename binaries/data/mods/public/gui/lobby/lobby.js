var g_ChatMessages = [];
var g_Name = "unknown";
var g_GameList = {};
var g_specialKey = Math.random();
// This object looks like {"name":[numMessagesSinceReset, lastReset, timeBlocked]} when in use.
var g_spamMonitor = {};
var g_timestamp = Engine.ConfigDB_GetValue("user", "lobby.chattimestamp") == "true";
var g_mapSizes = {};
var g_userRating = ""; // Rating of user, defaults to Unrated
var g_modPrefix = "@";
// Block spammers for 30 seconds.
var SPAM_BLOCK_LENGTH = 30;

////////////////////////////////////////////////////////////////////////////////////////////////

function init(attribs)
{
	// Play menu music
	initMusic();
	global.music.setState(global.music.states.MENU);

	g_Name = Engine.LobbyGetNick();

	g_mapSizes = initMapSizes();
	g_mapSizes.shortNames.splice(0, 0, translateWithContext("map size", "Any"));
	g_mapSizes.tiles.splice(0, 0, "");

	var mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	mapSizeFilter.list = g_mapSizes.shortNames;
	mapSizeFilter.list_data = g_mapSizes.tiles;

	var playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
	playersNumberFilter.list = [translateWithContext("player number", "Any"),2,3,4,5,6,7,8];
	playersNumberFilter.list_data = ["",2,3,4,5,6,7,8];

	var mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
	mapTypeFilter.list = [translateWithContext("map", "Any"), translateWithContext("map", "Skirmish"), translateWithContext("map", "Random"), translate("Scenario")];
	mapTypeFilter.list_data = ["", "skirmish", "random", "scenario"];

	Engine.LobbySetPlayerPresence("available");
	Engine.SendGetGameList();
	Engine.SendGetBoardList();
	Engine.SendGetRatingList();
	updatePlayerList();
	updateSubject(Engine.LobbyGetRoomSubject());

	resetFilters();
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
// Update functions
////////////////////////////////////////////////////////////////////////////////////////////////

function resetFilters()
{
	// Reset states of gui objects
	Engine.GetGUIObjectByName("mapSizeFilter").selected = 0
	Engine.GetGUIObjectByName("playersNumberFilter").selected = 0;
	Engine.GetGUIObjectByName("mapTypeFilter").selected = 0;
	Engine.GetGUIObjectByName("showFullFilter").checked = true;

	// Update the list of games
	updateGameList();

	// Update info box about the game currently selected
	updateGameSelection();
}

function applyFilters()
{
	// Update the list of games
	updateGameList();

	// Update info box about the game currently selected
	updateGameSelection();
}

/**
 * Filter a game based on the status of the filter dropdowns.
 *
 * @param game Game to be tested.
 * @return True if game should not be displayed.
 */
function filterGame(game)
{
	var mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	var playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
	var mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
	var showFullFilter = Engine.GetGUIObjectByName("showFullFilter");
	// We assume index 0 means display all for any given filter.
	if (mapSizeFilter.selected != 0 && game.mapSize != mapSizeFilter.list_data[mapSizeFilter.selected])
		return true;
	if (playersNumberFilter.selected != 0 && game.tnbp != playersNumberFilter.list_data[playersNumberFilter.selected])
		return true;
	if (mapTypeFilter.selected != 0 && game.mapType != mapTypeFilter.list_data[mapTypeFilter.selected])
		return true;
	if (!showFullFilter.checked && game.tnbp <= game.nbp)
		return true;

	return false;
}

/**
 * Update the subject GUI object.
 *
 * @param newSubject New room subject.
 */
function updateSubject(newSubject)
{
	var subject = Engine.GetGUIObjectByName("subject");
	var subjectBox = Engine.GetGUIObjectByName("subjectBox");
	var logo = Engine.GetGUIObjectByName("logo");
	// Load new subject and un-escape newlines.
	subject.caption = newSubject.replace("\\n", "\n", "g");
	// If the subject is only whitespace, hide it and reposition the logo.
	if (subject.caption.match(/^([\s\t\r\n]*)$/g))
	{
		subjectBox.hidden = true;
		logo.size = "50%-110 50%-50 50%+110 50%+50";
	}
	else
	{
		subjectBox.hidden = false;
		logo.size = "50%-110 40 50%+110 140";
	}
}

/**
 * Do a full update of the player listing, including ratings from C++.
 *
 * @return Array containing the player, presence, nickname, and rating listings.
 */
function updatePlayerList()
{
	var playersBox = Engine.GetGUIObjectByName("playersBox");
	[playerList, presenceList, nickList, ratingList] = [[],[],[],[]];
	var cleanPlayerList = Engine.GetPlayerList();
	// Sort the player list, ignoring case.
	cleanPlayerList.sort(function(a,b) 
	{
		var aName = a.name.toLowerCase();
		var bName = b.name.toLowerCase();
		return ((aName > bName) ? 1 : (bName > aName) ? -1 : 0);
	} );
	for (var i = 0; i < cleanPlayerList.length; i++)
	{
		// Identify current user's rating.
		if (cleanPlayerList[i].name == g_Name && cleanPlayerList[i].rating)
			g_userRating = cleanPlayerList[i].rating;
		// Add a "-" for unrated players.
		if (!cleanPlayerList[i].rating)
			cleanPlayerList[i].rating = "-";
		// Colorize.
		var [name, status, rating] = formatPlayerListEntry(cleanPlayerList[i].name, cleanPlayerList[i].presence, cleanPlayerList[i].rating, cleanPlayerList[i].role);
		// Push to lists.
		playerList.push(name);
		presenceList.push(status);
		nickList.push(cleanPlayerList[i].name);
		var ratingSpaces = "  ";
		for (var index = 0; index < 4 - Math.ceil(Math.log(cleanPlayerList[i].rating) / Math.LN10); index++)
			ratingSpaces += "  ";
		ratingList.push(String(ratingSpaces + rating));
	}
	playersBox.list_name = playerList;
	playersBox.list_status = presenceList;
	playersBox.list_rating = ratingList;
	playersBox.list = nickList;
	if (playersBox.selected >= playersBox.list.length)
		playersBox.selected = -1;
	return [playerList, presenceList, nickList, ratingList];
}

/**
 * Update the leaderboard from data cached in C++.
 */
function updateLeaderboard()
{
	// Get list from C++
	var boardList = Engine.GetBoardList();
	// Get GUI leaderboard object
	var leaderboard = Engine.GetGUIObjectByName("leaderboardBox");
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

/**
 * Update the game listing from data cached in C++.
 */
function updateGameList()
{
	var gamesBox = Engine.GetGUIObjectByName("gamesBox");
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

	var c = 0;
	for each (var g in gameList)
	{
		if(!filterGame(g))
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

	// Update info box about the game currently selected
	updateGameSelection();
}

/**
 * Colorize and format the entries in the player list.
 *
 * @param nickname Name of player.
 * @param presence Presence of player.
 * @param rating Rating of player.
 * @return Colorized versions of name, status, and rating.
 */
function formatPlayerListEntry(nickname, presence, rating)
{
	// Set colors based on player status
	var color;
	var status;
	switch (presence)
	{
	case "playing":
		color = "125 0 0";
		status = translate("Busy");
		break;
	case "gone":
	case "away":
		color = "229 76 13";
		status = translate("Away");
		break;
	case "available":
		color = "0 125 0";
		status = translate("Online");
		break;
	case "offline":
		color = "0 0 0";
		status = translate("Offline");
		break;
	default:
		warn(sprintf("Unknown presence '%(presence)s'", { presence: presence }));
		color = "178 178 178";
		status = translateWithContext("lobby presence", "Unknown");
		break;
	}
	// Center the unrated symbol.
	if (rating == "-")
		rating = "    -";
	var formattedStatus = '[color="' + color + '"]' + status + "[/color]";
	var formattedRating = '[color="' + color + '"]' + rating + "[/color]";
	var role = Engine.LobbyGetPlayerRole(nickname);
	if (role == "moderator")
		nickname = g_modPrefix + nickname;
	var formattedName = colorPlayerName(nickname);

	// Give moderators special formatting.
	if (role == "moderator")
		formattedName = formattedName; //TODO

	// Push this player's name and status onto the list
	return [formattedName, formattedStatus, formattedRating];
}

/**
 * Populate the game info area with information on the current game selection.
 */
function updateGameSelection()
{
	var selected = Engine.GetGUIObjectByName("gamesBox").selected;
	// If a game is not selected, hide the game information area.
	if (selected == -1)
	{
		Engine.GetGUIObjectByName("gameInfo").hidden = true;
		Engine.GetGUIObjectByName("joinGameButton").hidden = true;
		Engine.GetGUIObjectByName("gameInfoEmpty").hidden = false;
		return;
	}

	var mapData;
	var g = Engine.GetGUIObjectByName("gamesBox").list_data[selected];

	// Load map data
	if (g_GameList[g].mapType == "random" && g_GameList[g].mapName == "random")
		mapData = {"settings": {"Description": translate("A randomly selected map.")}};
	else if (g_GameList[g].mapType == "random" && Engine.FileExists(g_GameList[g].mapName + ".json"))
		mapData = parseJSONData(g_GameList[g].mapName + ".json");
	else if (Engine.FileExists(g_GameList[g].mapName + ".xml"))
		mapData = Engine.LoadMapSettings(g_GameList[g].mapName + ".xml");
	else
		// Warn the player if we can't find the map. 
		warn(sprintf("Map '%(mapName)s' not found locally.", { mapName: g_GameList[g].mapName }));

	// Show the game info panel and join button.
	Engine.GetGUIObjectByName("gameInfo").hidden = false;
	Engine.GetGUIObjectByName("joinGameButton").hidden = false;
	Engine.GetGUIObjectByName("gameInfoEmpty").hidden = true;

	// Display the map name, number of players, the names of the players, the map size and the map type.
	Engine.GetGUIObjectByName("sgMapName").caption = g_GameList[g].niceMapName;
	Engine.GetGUIObjectByName("sgNbPlayers").caption = g_GameList[g].nbp + "/" + g_GameList[g].tnbp;
	Engine.GetGUIObjectByName("sgPlayersNames").caption = g_GameList[g].players;
	Engine.GetGUIObjectByName("sgMapSize").caption = g_GameList[g].mapSize.split("(")[0];
	Engine.GetGUIObjectByName("sgMapType").caption = toTitleCase(g_GameList[g].mapType);

	// Display map description if it exists, otherwise display a placeholder.
	if (mapData && mapData.settings.Description)
		var mapDescription = mapData.settings.Description;
	else
		var mapDescription = translate("Sorry, no description available.");

	// Display map preview if it exists, otherwise display a placeholder.
	if (mapData && mapData.settings.Preview)
		var mapPreview = mapData.settings.Preview;
	else
		var mapPreview = "nopreview.png";

	Engine.GetGUIObjectByName("sgMapDescription").caption = mapDescription;
	Engine.GetGUIObjectByName("sgMapPreview").sprite = "cropped:(0.7812,0.5859)session/icons/mappreview/" + mapPreview;
}

/**
 * Start the joining process on the currectly selected game.
 */
function joinSelectedGame()
{
	var gamesBox = Engine.GetGUIObjectByName("gamesBox");
	if (gamesBox.selected >= 0)
	{
		var g = gamesBox.list_data[gamesBox.selected];
		var sname = g_Name;
		var sip = g_GameList[g].ip;

		// TODO: What about valid host names?
		// Check if it looks like an ip address
		if (sip.split('.').length != 4)
		{
			addChatMessage({ "from": "system", "text": sprintf(translate("This game's address '%(ip)s' does not appear to be valid."), { ip: sip }) });
			return;
		}

		// Open Multiplayer connection window with join option.
		Engine.PushGuiPage("page_gamesetup_mp.xml", { multiplayerGameType: "join", name: sname, ip: sip, rating: g_userRating });
	}
}

/**
 * Start the hosting process.
 */
function hostGame()
{
	// Open Multiplayer connection window with host option.
	Engine.PushGuiPage("page_gamesetup_mp.xml", { multiplayerGameType: "host", name: g_Name, rating: g_userRating });
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Utils
////////////////////////////////////////////////////////////////////////////////////////////////

function stripColorCodes(input)
{
	return input.replace(/\[(\w+)[^w]*?](.*?)\[\/\1]/g, '$2');
}

////////////////////////////////////////////////////////////////////////////////////////////////
// GUI event handlers
////////////////////////////////////////////////////////////////////////////////////////////////

function onTick()
{
	updateTimers();
	checkSpamMonitor();

	// Receive messages
	while (true)
	{
		var message = Engine.LobbyGuiPollMessage();
		// Clean Message
		if (!message)
			break;
		var text = escapeText(message.text);
		switch (message.type)
		{
		case "mucmessage": // For room messages
			var from = escapeText(message.from);
			addChatMessage({ "from": from, "text": text });
			break;
		case "message": // For private messages
			var from = escapeText(message.from);
			addChatMessage({ "from": from, "text": text });
			break;
		case "muc":
			var nick = message.text;
			var presence = Engine.LobbyGetPlayerPresence(nick);
			var playersBox = Engine.GetGUIObjectByName("playersBox");
			var playerList = playersBox.list_name;
			var presenceList = playersBox.list_status;
			var nickList = playersBox.list;
			var ratingList = playersBox.list_rating;
			var nickIndex = nickList.indexOf(nick);
			switch(message.level)
			{
			case "join":
				if (nick == g_Name)
				{
					// We just joined, we need to get the full player list
					[playerList, presenceList, nickList, ratingList] = updatePlayerList();
					break;
				}
				var [name, status, rating] = formatPlayerListEntry(nick, presence, "-");
				playerList.push(name);
				presenceList.push(status);
				nickList.push(nick);
				ratingList.push(String(rating));
				Engine.SendGetRatingList();
				addChatMessage({ "text": "/special " + sprintf(translate("%(nick)s has joined."), { nick: nick }), "key": g_specialKey });
				break;
			case "leave":
				if (nickIndex == -1) // Left, but not present (TODO: warn about this?)
					break;
				playerList.splice(nickIndex, 1);
				presenceList.splice(nickIndex, 1);
				nickList.splice(nickIndex, 1);
				ratingList.splice(nickIndex, 1);
				addChatMessage({ "text": "/special " + sprintf(translate("%(nick)s has left."), { nick: nick }), "key": g_specialKey });
				break;
			case "nick":
				if (nickIndex == -1) // This shouldn't ever happen
					break;
				if (!isValidNick(message.data))
				{
					addChatMessage({ "from": "system", "text": sprintf(translate("Invalid nickname: %(nick)s"), { nick: message.data })});
					break;
				}
				var [name, status, rating] = formatPlayerListEntry(message.data, presence, stripColorCodes(ratingList[nickIndex])); // TODO: actually we don't want to change the presence here, so use what was used before
				playerList[nickIndex] = name;
				// presence stays the same
				nickList[nickIndex] = message.data;
				addChatMessage({ "text": "/special " + sprintf(translate("%(oldnick)s is now known as %(newnick)s."), { oldnick: nick, newnick: message.data }), "key": g_specialKey });
				Engine.SendGetRatingList();
				break;
			case "presence":
				if (nickIndex == -1) // This shouldn't ever happen
					break;
				var [name, status, rating] = formatPlayerListEntry(nick, presence, stripColorCodes(ratingList[nickIndex]));
				presenceList[nickIndex] = status;
				playerList[nickIndex] = name;
				ratingList[nickIndex] = rating;
				break;
			case "subject":
				updateSubject(message.text);
				break;
			default:
				warn(sprintf("Unknown message.level '%(msglvl)s'", { msglvl: message.level }));
				break;
			}
			// Push new data to GUI
			playersBox.list_name = playerList;
			playersBox.list_status = presenceList;
			playersBox.list_rating = ratingList;
			playersBox.list = nickList;		
			if (playersBox.selected >= playersBox.list.length)
				playersBox.selected = -1;
			break;
		case "system":
			switch (message.level)
			{
			case "standard":
				addChatMessage({ "from": "system", "text": text, "color": "150 0 0" });
				if (message.text == "disconnected")
				{
					// Clear the list of games and the list of players
					updateGameList();
					updateLeaderboard();
					updatePlayerList();
					// Disable the 'host' button
					Engine.GetGUIObjectByName("hostButton").enabled = false;
				}
				else if (message.text == "connected")
				{
					Engine.GetGUIObjectByName("hostButton").enabled = true;
				}
				break;
			case "error":
				addChatMessage({ "from": "system", "text": text, "color": "150 0 0" });
				break;
			case "internal":
				switch (message.text)
				{
				case "gamelist updated":
					updateGameList();
					break;
				case "boardlist updated":
					updateLeaderboard();
					break;
				case "ratinglist updated":
					updatePlayerList();
					break;
				}
				break
			}
			break;
		default:
			error(sprintf("Unrecognised message type %(msgtype)s", { msgtype: message.type }));
		}
	}
}

/* Messages */
function submitChatInput()
{
	var input = Engine.GetGUIObjectByName("chatInput");
	var text = escapeText(input.caption);
	if (text.length)
	{
		if (!handleSpecialCommand(text) && !isSpam(text, g_Name))
			Engine.LobbySendMessage(text);
		input.caption = "";
	}
}

function completeNick()
{
	var input = Engine.GetGUIObjectByName("chatInput");
	var text = escapeText(input.caption);
	if (text.length)
	{
		var matched = false;
		for each (var playerObj in Engine.GetPlayerList())
		{
			var player = playerObj.name;
			var breaks = text.match(/(\s+)/g) || [];
			text.split(/\s+/g).reduceRight(function (wordsSoFar, word, index)
			{
				if (matched)
					return null;
				var matchCandidate = word + (breaks[index - 1] || "") + wordsSoFar;
				if (player.toUpperCase().indexOf(matchCandidate.toUpperCase().trim()) == 0)
				{
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

/**
 * Handle all '/' commands.
 *
 * @param text Text to be checked for commands.
 * @return true if more text processing is needed, false otherwise.
 */
function handleSpecialCommand(text)
{
	if (text[0] != '/')
		return false;

	var [cmd, nick] = ircSplit(text);

	switch (cmd)
	{
	case "away":
		Engine.LobbySetPlayerPresence("away");
		break;
	case "back":
		Engine.LobbySetPlayerPresence("available");
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
		addChatMessage({ "from":"system", "text": sprintf(translate("We're sorry, the '%(cmd)s' command is not supported."), { cmd: cmd})});
	}
	return true;
}

/**
 * Process and, if appropriate, display a formatted message.
 *
 * @param msg The message to be processed.
 */
function addChatMessage(msg)
{
	// Some calls of this function will leave some msg parameters empty. Text is required though.
	if (msg.from)
	{
		// Display the moderator symbol in the chatbox.
		var playerRole = Engine.LobbyGetPlayerRole(msg.from);
		if (playerRole == "moderator")
			msg.from = g_modPrefix + msg.from;
	}
	else
		msg.from = null;
	if (!msg.color)
		msg.color = null;
	if (!msg.key)
		msg.key = null;	

	// Highlight local user's nick
	if (msg.text.indexOf(g_Name) != -1 && g_Name != msg.from)
		msg.text = msg.text.replace(new RegExp('\\b' + '\\' + g_Name + '\\b', "g"), colorPlayerName(g_Name));

	// Run spam test
	updateSpamMonitor(msg.from);
	if (isSpam(msg.text, msg.from))
		return;

	// Format Text
	var formatted = ircFormat(msg.text, msg.from, msg.color, msg.key);

	// If there is text, add it to the chat box.
	if (formatted)
	{
		g_ChatMessages.push(formatted);
		Engine.GetGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
	}
}

function ircSplit(string)
{
	var idx = string.indexOf(' ');
	if (idx != -1)
		return [string.substr(1,idx-1), string.substr(idx+1)];
	return [string.substr(1), ""];
}

/**
 * Format text in an IRC-like way.
 *
 * @param text Body of the message.
 * @param from Sender of the message.
 * @param color Optional color of sender.
 * @param key Key to verify join/leave messages with. TODO: Remove this, it only provides synthetic security.
 * @return Formatted text.
 */
function ircFormat(text, from, color, key)
{
	// Generate and apply color to uncolored names,
	if (!color && from)
		var coloredFrom = colorPlayerName(from);
	else if (color && from)
		var coloredFrom = '[color="' + color + '"]' + from + "[/color]";

	// Handle commands allowed past handleSpecialCommand.
	if (text[0] == '/')
	{
		var [command, message] = ircSplit(text);
		switch (command)
		{
			case "me":
				// Translation: IRC message prefix when the sender uses the /me command.
				var senderString = '[font="sans-bold-13"]' + sprintf(translate("* %(sender)s"), { sender: coloredFrom }) + '[/font]';
				// Translation: IRC message issued using the ‘/me’ command.
				var formattedMessage = sprintf(translate("%(sender)s %(action)s"), { sender: senderString, action: message });
				break;
			case "say":
				// Translation: IRC message prefix.
				var senderString = '[font="sans-bold-13"]' + sprintf(translate("<%(sender)s>"), { sender: coloredFrom }) + '[/font]';
				// Translation: IRC message.
				var formattedMessage = sprintf(translate("%(sender)s %(message)s"), { sender: senderString, message: message });
				break
			case "special":
				if (key === g_specialKey)
					// Translation: IRC system message.
					var formattedMessage = '[font="sans-bold-13"]' + sprintf(translate("== %(message)s"), { message: message }) + '[/font]';
				else
				{
					// Translation: IRC message prefix.
					var senderString = '[font="sans-bold-13"]' + sprintf(translate("<%(sender)s>"), { sender: coloredFrom }) + '[/font]';
					// Translation: IRC message.
					var formattedMessage = sprintf(translate("%(sender)s %(message)s"), { sender: senderString, message: message });
				}
				break;
			default:
				// This should never happen.
				var formattedMessage = "";
		}
	}
	else
	{
		// Translation: IRC message prefix.
		var senderString = '[font="sans-bold-13"]' + sprintf(translate("<%(sender)s>"), { sender: coloredFrom }) + '[/font]';
		// Translation: IRC message.
		var formattedMessage = sprintf(translate("%(sender)s %(message)s"), { sender: senderString, message: text });
	}

	// Build time header if enabled
	if (g_timestamp)
	{
		// Time for optional time header
		var time = new Date(Date.now());

		// Translation: Time as shown in the multiplayer lobby (when you enable it in the options page).
		// For a list of symbols that you can use, see:
		// https://sites.google.com/site/icuprojectuserguide/formatparse/datetime?pli=1#TOC-Date-Field-Symbol-Table
		var timeString = Engine.FormatMillisecondsIntoDateString(time.getTime(), translate("HH:mm"));

		// Translation: Time prefix as shown in the multiplayer lobby (when you enable it in the options page).
		var timePrefixString = '[font="sans-bold-13"]' + sprintf(translate("[%(time)s]"), { time: timeString }) + '[/font]';

		// Translation: IRC message format when there is a time prefix.
		return sprintf(translate("%(time)s %(message)s"), { time: timePrefixString, message: formattedMessage });
	}
	else
		return formattedMessage;
}

/**
 * Update the spam monitor.
 *
 * @param from User to update.
 */
function updateSpamMonitor(from)
{
	// Integer time in seconds.
	var time = Math.floor(Date.now() / 1000);

	// Update or initialize the user in the spam monitor.
	if (g_spamMonitor[from])
		g_spamMonitor[from][0]++;
	else
		g_spamMonitor[from] = [1, time, 0];
}

/**
 * Check if a message is spam.
 *
 * @param text Body of message.
 * @param from Sender of message.
 * @return True if message should be blocked.
 */
function isSpam(text, from)
{
	// Integer time in seconds.
	var time = Math.floor(Date.now() / 1000);

	// Initialize if not already in the database.
	if (!g_spamMonitor[from])
		g_spamMonitor[from] = [1, time, 0];

	// Block blank lines.
	if (text == " ")
		return true;
	// Block users who are still within their spam block period.
	else if (g_spamMonitor[from][2] + SPAM_BLOCK_LENGTH >= time)
		return true;
	// Block users who exceed the rate of 1 message per second for five seconds and are not already blocked. TODO: Make this smarter and block profanity.
	else if (g_spamMonitor[from][0] == 6)
	{
		g_spamMonitor[from][2] = time;
		if (from == g_Name)
			addChatMessage({ "from": "system", "text": translate("Please do not spam. You have been blocked for thirty seconds.") });
		return true;
	}
	// Return false if everything is clear.
	else
		return false;
}

/**
 * Reset timer used to measure message send speed.
 */
function checkSpamMonitor()
{
	// Integer time in seconds.
	var time = Math.floor(Date.now() / 1000);

	// Clear message count every 5 seconds.
	for each (var stats in g_spamMonitor)
	{
		if (stats[1] + 5 <= time)
		{
			stats[1] = time;
			stats[0] = 0;
		}
	}

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
var fixedColors = { "system": repeatString(7, "255.0.0."), "@WFGbot": repeatString(7, "255.24.24."),
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
	return '[color="' + getPlayerColor(playername.replace(g_modPrefix, "")) + '"]' + playername + '[/color]';
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
