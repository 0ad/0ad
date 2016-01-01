/**
 * Used for the gamelist-filtering.
 */
const g_MapSizes = prepareForDropdown(g_Settings ? g_Settings.MapSizes : undefined);

/**
 * Used for the gamelist-filtering.
 */
const g_MapTypes = prepareForDropdown(g_Settings ? g_Settings.MapTypes : undefined);

/**
 * Whether or not to display timestamps in the chat window.
 */
const g_ShowTimestamp = Engine.ConfigDB_GetValue("user", "lobby.chattimestamp") == "true";

/**
 * Mute spammers for this time.
 */
const g_SpamBlockDuration = 30;

/**
 * A symbol which is prepended to the username of moderators.
 */
const g_ModeratorPrefix = "@";

/**
 * Current username. Cannot contain whitespace.
 */
const g_Username = Engine.LobbyGetNick();

/**
 * Current games will be listed in these colors.
 */
const g_GameColors = {
	"init": "0 219 0",
	"waiting": "255 127 0",
	"running": "219 0 0"
};

/**
 * The playerlist will be assembled using these values.
 */
const g_PlayerStatuses = {
	"available": { "color": "0 219 0",     "status": translate("Online") },
	"away":      { "color": "229 76 13",   "status": translate("Away") },
	"playing":   { "color": "200 0 0",     "status": translate("Busy") },
	"offline":   { "color": "0 0 0",       "status": translate("Offline") },
	"unknown":   { "color": "178 178 178", "status": translateWithContext("lobby presence", "Unknown") }
};

/**
 * Color for error messages in the chat.
 */
const g_SystemColor = "150 0 0";

/**
 * Used for highlighting the sender of chat messages.
 */
const g_SenderFont = "sans-bold-13";

/**
 * All chat messages received since init (i.e. after lobby join and after returning from a game).
 */
var g_ChatMessages = [];

/**
 * Rating of the current user.
 * Contains the number or an empty string in case the user has no rating.
 */
var g_UserRating = "";

/**
 * All games currently running.
 */
var g_GameList = {};

/**
 * Remembers how many messages were sent by each user since the last reset.
 *
 * For example { "username": [numMessagesSinceReset, lastReset, timeBlocked] }
 */
var g_SpamMonitor = {};

/**
 * Used to restore the selection after updating the playerlist.
 */
var g_SelectedPlayer = "";

/**
 * Used to restore the selection after updating the gamelist.
 */
var g_SelectedGameIP = "";

/**
 * Notifications sent by XmppClient.cpp
 */
var g_NetMessageTypes = {
	"system": {
		// Three cases are handled in prelobby.js
		"registered": msg => {
		},
		"login-failed": msg => {
		},
		"connected": msg => {
		},
		"disconnected": msg => {
			updateGameList();
			updateLeaderboard();
			updatePlayerList();
			Engine.GetGUIObjectByName("hostButton").enabled = false;
			addChatMessage({ "from": "system", "text": translate("Disconnected."), "color": g_SystemColor });
		},
		"error": msg => {
			addChatMessage({ "from": "system", "text": escapeText(msg.text), "color": g_SystemColor });
		}
	},
	"chat": {
		"subject": msg => {
			updateSubject(msg.text);
		},
		"join": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(nick)s has joined."), { "nick": msg.text }),
				"isSpecial": true
			});
			Engine.SendGetRatingList();
		},
		"leave": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(nick)s has left."), { "nick": msg.text }),
				"isSpecial": true
			});
		},
		"presence": msg => {
		},
		"nick": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(oldnick)s is now known as %(newnick)s."), {
					"oldnick": msg.text,
					"newnick": msg.data
				}),
				"isSpecial": true
			});
		},
		"room-message": msg => {
			addChatMessage({
				"from": escapeText(msg.from),
				"text": escapeText(msg.text),
				"datetime": msg.datetime
			});
		},
		"private-message": msg => {
			if (Engine.LobbyGetPlayerRole(msg.from) == "moderator")
				addChatMessage({
					"from": "(Private) " + escapeText(msg.from), // TODO: placeholder
					"text": escapeText(msg.text.trim()), // some XMPP clients send trailing whitespace
					"datetime": msg.datetime
				});
		}
	},
	"game": {
		"gamelist": msg => updateGameList(),
		"profile": msg => updateProfile(),
		"leaderboard": msg => updateLeaderboard(),
		"ratinglist": msg => updatePlayerList()
	}
};

/**
 * Called after the XmppConnection succeeded and when returning from a game.
 *
 * @param {Object} attribs
 */
function init(attribs)
{
	if (!g_Settings)
	{
		returnToMainMenu();
		return;
	}

	initMusic();
	global.music.setState(global.music.states.MENU);

	initGameFilters();

	Engine.LobbySetPlayerPresence("available");
	Engine.SendGetGameList();
	Engine.SendGetBoardList();

	// When rejoining the lobby after a game, we don't need to process presence changes
	Engine.LobbyClearPresenceUpdates();
	updatePlayerList();
	updateSubject(Engine.LobbyGetRoomSubject());
}

function returnToMainMenu()
{
	Engine.StopXmppClient();
	Engine.SwitchGuiPage("page_pregame.xml");
}

function initGameFilters()
{
	var mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	mapSizeFilter.list = [translateWithContext("map size", "Any")].concat(g_MapSizes.Name);
	mapSizeFilter.list_data = [""].concat(g_MapSizes.Tiles);

	var playersArray = Array(g_MaxPlayers).fill(0).map((v, i) => i + 1); // 1, 2, ... MaxPlayers
	var playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
	playersNumberFilter.list = [translateWithContext("player number", "Any")].concat(playersArray);
	playersNumberFilter.list_data = [""].concat(playersArray);

	var mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
	mapTypeFilter.list = [translateWithContext("map", "Any")].concat(g_MapTypes.Title);
	mapTypeFilter.list_data = [""].concat(g_MapTypes.Name);

	resetFilters();
}

function resetFilters()
{
	Engine.GetGUIObjectByName("mapSizeFilter").selected = 0;
	Engine.GetGUIObjectByName("playersNumberFilter").selected = 0;
	Engine.GetGUIObjectByName("mapTypeFilter").selected = g_MapTypes.Default;
	Engine.GetGUIObjectByName("showFullFilter").checked = false;

	applyFilters();
}

function applyFilters()
{
	updateGameList();
	updateGameSelection();
}

/**
 * Filter a game based on the status of the filter dropdowns.
 *
 * @param {Object} game
 * @returns {boolean} - True if game should not be displayed.
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
 * @param {string} newSubject
 */
function updateSubject(newSubject)
{
	var subject = Engine.GetGUIObjectByName("subject").caption = newSubject;
	var subjectBox = Engine.GetGUIObjectByName("subjectBox");
	var logo = Engine.GetGUIObjectByName("logo");

	// If the subject is only whitespace, hide it and reposition the logo.
	if (!newSubject.trim())
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
 * Do a full update of the player listing, including ratings from cached C++ information.
 */
function updatePlayerList()
{
	var playersBox = Engine.GetGUIObjectByName("playersBox");
	var sortBy = playersBox.selected_column || "name";
	var sortOrder = playersBox.selected_column_order || 1;

	if (playersBox.selected > -1)
		g_SelectedPlayer = playersBox.list[playersBox.selected];

	var playerList = [];
	var presenceList = [];
	var nickList = [];
	var ratingList = [];

	var cleanPlayerList = Engine.GetPlayerList().sort((a, b) => {
		var sortA, sortB;
		switch (sortBy)
		{
		case 'rating':
			sortA = +a.rating;
			sortB = +b.rating;
			break;
		case 'status':
			let statusOrder = Object.keys(g_PlayerStatuses);
			sortA = statusOrder.indexOf(a.presence);
			sortB = statusOrder.indexOf(b.presence);
			break;
		case 'name':
		default:
			sortA = a.name.toLowerCase();
			sortB = b.name.toLowerCase();
			break;
		}
		if (sortA < sortB) return -sortOrder;
		if (sortA > sortB) return +sortOrder;
		return 0;
	});

	// Colorize list entries
	for (let player of cleanPlayerList)
	{
		if (player.rating && player.name == g_Username)
			g_UserRating = player.rating;
		let rating = player.rating ? ("     " + player.rating).substr(-5) : "     -";

		let presence = g_PlayerStatuses[player.presence] ? player.presence : "unknown";
		if (presence == "unknown")
			warn("Unknown presence:" + player.presence);

		let statusColor = g_PlayerStatuses[presence].color;
		let coloredName = colorPlayerName((player.role == "moderator" ? g_ModeratorPrefix : "") + player.name);
		let coloredPresence = '[color="' + statusColor + '"]' + g_PlayerStatuses[presence].status + "[/color]";
		let coloredRating = '[color="' + statusColor + '"]' + rating + "[/color]";

		playerList.push(coloredName);
		presenceList.push(coloredPresence);
		ratingList.push(coloredRating);
		nickList.push(player.name);
	}

	playersBox.list_name = playerList;
	playersBox.list_status = presenceList;
	playersBox.list_rating = ratingList;
	playersBox.list = nickList;
	playersBox.selected = playersBox.list.indexOf(g_SelectedPlayer);
}

/**
 * Display the profile of the selected player.
 * Displays N/A for all stats until updateProfile is called when the stats
 * 	are actually received from the bot.
 *
 * @param {string} caller - From which screen is the user requesting data from?
 */
function displayProfile(caller)
{
	var playerList, rating;
	if (caller == "leaderboard")
		playerList = Engine.GetGUIObjectByName("leaderboardBox");
	else if (caller == "lobbylist")
		playerList = Engine.GetGUIObjectByName("playersBox");
	else if (caller == "fetch")
	{
		Engine.SendGetProfile(Engine.GetGUIObjectByName("fetchInput").caption);
		return;
	}
	else
		return;

	var playerName = playerList.list[playerList.selected];
	Engine.GetGUIObjectByName("profileArea").hidden = !playerName;
	if (!playerName)
		return;
	Engine.SendGetProfile(playerName);

	var isModerator = Engine.LobbyGetPlayerRole(playerName) == "moderator";
	Engine.GetGUIObjectByName("usernameText").caption = playerList.list_name[playerList.selected];
	Engine.GetGUIObjectByName("roleText").caption = isModerator ? translate("Moderator") : translate("Player");
	Engine.GetGUIObjectByName("rankText").caption = translate("N/A");
	Engine.GetGUIObjectByName("highestRatingText").caption = translate("N/A");
	Engine.GetGUIObjectByName("totalGamesText").caption = translate("N/A");
	Engine.GetGUIObjectByName("winsText").caption = translate("N/A");
	Engine.GetGUIObjectByName("lossesText").caption = translate("N/A");
	Engine.GetGUIObjectByName("ratioText").caption = translate("N/A");
}

/**
 * Update the profile of the selected player with data from the bot.
 */
function updateProfile()
{
	var user;
	var playerList;
	var attributes = Engine.GetProfile();

	if (!Engine.GetGUIObjectByName("profileFetch").hidden)
	{
		let profileFound = attributes[0].rating != "-2";
		Engine.GetGUIObjectByName("profileWindowArea").hidden = !profileFound;
		Engine.GetGUIObjectByName("profileErrorText").hidden = profileFound;

		if (!profileFound)
			return;

		user = attributes[0].player;
		if (attributes[0].rating != "")
			user = sprintf(translate("%(nick)s (%(rating)s)"), { "nick": user, "rating": attributes[0].rating });

		Engine.GetGUIObjectByName("profileUsernameText").caption = user;
		Engine.GetGUIObjectByName("profileRankText").caption = attributes[0].rank;
		Engine.GetGUIObjectByName("profileHighestRatingText").caption = attributes[0].highestRating;
		Engine.GetGUIObjectByName("profileTotalGamesText").caption = attributes[0].totalGamesPlayed;
		Engine.GetGUIObjectByName("profileWinsText").caption = attributes[0].wins;
		Engine.GetGUIObjectByName("profileLossesText").caption = attributes[0].losses;

		var winRate = (attributes[0].wins / attributes[0].totalGamesPlayed * 100).toFixed(2);
		if (attributes[0].totalGamesPlayed != 0)
			Engine.GetGUIObjectByName("profileRatioText").caption = sprintf(translate("%(percentage)s%%"), { "percentage": winRate });
		else
			Engine.GetGUIObjectByName("profileRatioText").caption = translateWithContext("Used for an undefined winning rate", "-");
		return;
	}
	else if (!Engine.GetGUIObjectByName("leaderboard").hidden)
		playerList = Engine.GetGUIObjectByName("leaderboardBox");
	else
		playerList = Engine.GetGUIObjectByName("playersBox");

	if (attributes[0].rating == "-2")
		return;

	// Make sure the stats we have received coincide with the selected player.
	if (attributes[0].player != playerList.list[playerList.selected])
		return;

	user = playerList.list_name[playerList.selected];
	if (attributes[0].rating != "")
		user = sprintf(translate("%(nick)s (%(rating)s)"), { "nick": user, "rating": attributes[0].rating });

	Engine.GetGUIObjectByName("usernameText").caption = user;
	Engine.GetGUIObjectByName("rankText").caption = attributes[0].rank;
	Engine.GetGUIObjectByName("highestRatingText").caption = attributes[0].highestRating;
	Engine.GetGUIObjectByName("totalGamesText").caption = attributes[0].totalGamesPlayed;
	Engine.GetGUIObjectByName("winsText").caption = attributes[0].wins;
	Engine.GetGUIObjectByName("lossesText").caption = attributes[0].losses;

	var winRate = (attributes[0].wins / attributes[0].totalGamesPlayed * 100).toFixed(2);
	if (attributes[0].totalGamesPlayed != 0)
		Engine.GetGUIObjectByName("ratioText").caption = sprintf(translate("%(percentage)s%%"), { "percentage": winRate });
	else
		Engine.GetGUIObjectByName("ratioText").caption = translateWithContext("Used for an undefined winning rate", "-");
}

/**
 * Update the leaderboard from data cached in C++.
 */
function updateLeaderboard()
{
	var leaderboard = Engine.GetGUIObjectByName("leaderboardBox");
	var boardList = Engine.GetBoardList().sort((a, b) => b.rating - a.rating);

	var list = [];
	var list_name = [];
	var list_rank = [];
	var list_rating = [];

	for (let i in boardList)
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
	var sortBy = gamesBox.selected_column || "status";
	var sortOrder = gamesBox.selected_column_order || 1;

	if (gamesBox.selected > -1)
		g_SelectedGameIP = g_GameList[gamesBox.selected].ip;

	g_GameList = Engine.GetGameList().filter(game => !filterGame(game)).sort((a, b) => {
		var sortA, sortB;
		switch (sortBy)
		{
		case 'name':
		case 'mapSize':
		case 'mapType':
			sortA = a[sortBy];
			sortB = b[sortBy];
			break;
		case 'mapName':
			sortA = translate(a.niceMapName);
			sortB = translate(b.niceMapName);
			break;
		case 'nPlayers':
			// Compare playercount ratio
			sortA = a.nbp * b.tnbp;
			sortB = b.nbp * a.tnbp;
			break;
		case 'status':
		default:
			sortA = gameStatuses.indexOf(a.state);
			sortB = gameStatuses.indexOf(b.state);
			break;
		}
		if (sortA < sortB) return -sortOrder;
		if (sortA > sortB) return +sortOrder;
		return 0;
	});

	var list_name = [];
	var list_mapName = [];
	var list_mapSize = [];
	var list_mapType = [];
	var list_nPlayers = [];
	var list = [];
	var list_data = [];
	var selectedGameIndex = -1;

	for (let i in g_GameList)
	{
		let game = g_GameList[i];
		let gameName = escapeText(game.name);
		let mapTypeIdx = g_MapTypes.Name.indexOf(game.mapType);

		if (game.ip == g_SelectedGameIP)
			selectedGameIndex = +i;

		list_name.push('[color="' + g_GameColors[game.state] + '"]' + gameName);
		list_mapName.push(translate(game.niceMapName));
		list_mapSize.push(translateMapSize(game.mapSize));
		list_mapType.push(mapTypeIdx != -1 ? g_MapTypes.Title[mapTypeIdx] : "");
		list_nPlayers.push(game.nbp + "/" + game.tnbp);
		list.push(gameName);
		list_data.push(i);
	}

	gamesBox.list_name = list_name;
	gamesBox.list_mapName = list_mapName;
	gamesBox.list_mapSize = list_mapSize;
	gamesBox.list_mapType = list_mapType;
	gamesBox.list_nPlayers = list_nPlayers;
	// Change these last, otherwise crash
	gamesBox.list = list;
	gamesBox.list_data = list_data;
	gamesBox.selected = selectedGameIndex;

	updateGameSelection();
}

/**
 * Populate the game info area with information on the current game selection.
 */
function updateGameSelection()
{
	var selected = Engine.GetGUIObjectByName("gamesBox").selected;
	if (selected == -1)
	{
		Engine.GetGUIObjectByName("gameInfo").hidden = true;
		Engine.GetGUIObjectByName("joinGameButton").hidden = true;
		Engine.GetGUIObjectByName("gameInfoEmpty").hidden = false;
		return;
	}

	// Show the game info panel and join button.
	Engine.GetGUIObjectByName("gameInfo").hidden = false;
	Engine.GetGUIObjectByName("joinGameButton").hidden = false;
	Engine.GetGUIObjectByName("gameInfoEmpty").hidden = true;

	// Display the map name, number of players, the names of the players, the map size and the map type.
	var game = g_GameList[selected];
	Engine.GetGUIObjectByName("sgMapName").caption = translate(game.niceMapName);
	Engine.GetGUIObjectByName("sgNbPlayers").caption = game.nbp + "/" + game.tnbp;
	Engine.GetGUIObjectByName("sgPlayersNames").caption = game.players;
	Engine.GetGUIObjectByName("sgMapSize").caption = translateMapSize(game.mapSize);
	let mapTypeIdx = g_MapTypes.Name.indexOf(game.mapType);
	Engine.GetGUIObjectByName("sgMapType").caption = mapTypeIdx != -1 ? g_MapTypes.Title[mapTypeIdx] : "";

	var mapData = getMapDescriptionAndPreview(game.mapType, game.mapName);
	Engine.GetGUIObjectByName("sgMapDescription").caption = mapData.description;
	setMapPreviewImage("sgMapPreview", mapData.preview);
}

/**
 * Start the joining process on the currectly selected game.
 */
function joinSelectedGame()
{
	var gamesBox = Engine.GetGUIObjectByName("gamesBox");
	if (gamesBox.selected < 0)
		return;

	var ipAddress = g_GameList[gamesBox.list_data[gamesBox.selected]].ip;
	if (ipAddress.split('.').length != 4)
	{
		addChatMessage({ "from": "system", "text": sprintf(translate("This game's address '%(ip)s' does not appear to be valid."), { "ip": ipAddress }) });
		return;
	}

	Engine.PushGuiPage("page_gamesetup_mp.xml", { "multiplayerGameType": "join", "name": g_Username, "ip": ipAddress, "rating": g_UserRating });
}

/**
 * Open the dialog box to enter the game name.
 */
function hostGame()
{
	Engine.PushGuiPage("page_gamesetup_mp.xml", { "multiplayerGameType": "host", "name": g_Username, "rating": g_UserRating });
}

/**
 * Processes GUI messages sent by the XmppClient.
 */
function onTick()
{
	updateTimers();
	checkSpamMonitor();

	while (true)
	{
		let msg = Engine.LobbyGuiPollMessage();
		if (!msg)
			break;

		if (!g_NetMessageTypes[msg.type])
		{
			warn("Unrecognised message type: " + msg.type);
			continue;
		}
		if (!g_NetMessageTypes[msg.type][msg.level])
		{
			warn("Unrecognised message level: " + msg.level);
			continue;
		}
		g_NetMessageTypes[msg.type][msg.level](msg);

		// To improve performance, only update the playerlist GUI when the last update in the current stack is processed
		if (msg.type == "chat" && Engine.LobbyGetMucMessageCount() == 0)
			updatePlayerList();
	}
}

/**
 * Executes a lobby command or sends GUI input directly as chat.
 */
function submitChatInput()
{
	var input = Engine.GetGUIObjectByName("chatInput");
	var text = input.caption;
	if (!text.length)
		return;
	if (!handleSpecialCommand(text) && !isSpam(text, g_Username))
		Engine.LobbySendMessage(text);
	input.caption = "";
}

/**
 * Handle all '/' commands.
 *
 * @param {string} text - Text to be checked for commands.
 * @returns {boolean} true if more text processing is needed, false otherwise.
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
		returnToMainMenu();
		break;
	case "clear":
		clearChatMessages();
		break;
	case "say":
	case "me":
		return false;
	default:
		addChatMessage({
			"from": "system",
			"text": sprintf(translate("We're sorry, the '%(cmd)s' command is not supported."), { "cmd": cmd })
		});
	}
	return true;
}

/**
 * Process and if appropriate, display a formatted message.
 *
 * @param {Object} msg - The message to be processed.
 */
function addChatMessage(msg)
{
	if (msg.from)
	{
		if (Engine.LobbyGetPlayerRole(msg.from) == "moderator")
			msg.from = g_ModeratorPrefix + msg.from;

		// Highlight local user's nick
		if (g_Username != msg.from)
			msg.text = msg.text.replace(g_Username, colorPlayerName(g_Username));

		// Run spam test if it's not a historical message
		if (!msg.datetime)
		{
			updateSpamMonitor(msg.from);
			if (isSpam(msg.text, msg.from))
				return;
		}
	}

	var formatted = ircFormat(msg);
	if (!formatted)
		return;

	g_ChatMessages.push(formatted);
	Engine.GetGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}


/**
 * Splits given input into command and argument.
 *
 * @param {string} string
 * @returns {Array}
 */
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
 * @param {Object} msg - Received chat message.
 * @returns {string} - Formatted text.
 */
function ircFormat(msg)
{
	var senderString = "";
	var formattedMessage = "";
	var coloredFrom = !msg.from ? "" : (msg.color ? '[color="' + msg.color + '"]' + msg.from + "[/color]" : colorPlayerName(msg.from));

	// Handle commands allowed past handleSpecialCommand.
	if (msg.text[0] == '/')
	{
		var [command, message] = ircSplit(msg.text);
		switch (command)
		{
		case "me":
			// Translation: IRC message prefix when the sender uses the /me command.
			senderString = '[font="' + g_SenderFont + '"]' + sprintf(translate("* %(sender)s"), { "sender": coloredFrom }) + '[/font]';
			// Translation: IRC message issued using the ‘/me’ command.
			formattedMessage = sprintf(translate("%(sender)s %(action)s"), { "sender": senderString, "action": message });
			break;
		case "say":
			// Translation: IRC message prefix.
			senderString = '[font="' + g_SenderFont + '"]' + sprintf(translate("<%(sender)s>"), { "sender": coloredFrom }) + '[/font]';
			// Translation: IRC message.
			formattedMessage = sprintf(translate("%(sender)s %(message)s"), { "sender": senderString, "message": message });
			break;
		case "special":
			if (msg.isSpecial)
				// Translation: IRC system message.
				formattedMessage = '[font="' + g_SenderFont + '"]' + sprintf(translate("== %(message)s"), { "message": message }) + '[/font]';
			else
			{
				// Translation: IRC message prefix.
				senderString = '[font="' + g_SenderFont + '"]' + sprintf(translate("<%(sender)s>"), { "sender": coloredFrom }) + '[/font]';
				// Translation: IRC message.
				formattedMessage = sprintf(translate("%(sender)s %(message)s"), { "sender": senderString, "message": message });
			}
			break;
		}
	}
	else
	{
		// Translation: IRC message prefix.
		senderString = '[font="' + g_SenderFont + '"]' + sprintf(translate("<%(sender)s>"), { "sender": coloredFrom }) + '[/font]';
		// Translation: IRC message.
		formattedMessage = sprintf(translate("%(sender)s %(message)s"), { "sender": senderString, "message": msg.text });
	}

	// Add chat message timestamp
	if (!g_ShowTimestamp)
		return formattedMessage;

	var time;
	if (msg.datetime)
	{
		let dTime = msg.datetime.split("T");
		let parserDate = dTime[0].split("-");
		let parserTime = dTime[1].split(":");
		// See http://xmpp.org/extensions/xep-0082.html#sect-idp285136 for format of datetime
		// Date takes Year, Month, Day, Hour, Minute, Second
		time = new Date(Date.UTC(parserDate[0], parserDate[1], parserDate[2], parserTime[0], parserTime[1], parserTime[2].split("Z")[0]));
	}
	else
		time = new Date(Date.now());

	// Translation: Time as shown in the multiplayer lobby (when you enable it in the options page).
	// For a list of symbols that you can use, see:
	// https://sites.google.com/site/icuprojectuserguide/formatparse/datetime?pli=1#TOC-Date-Field-Symbol-Table
	var timeString = Engine.FormatMillisecondsIntoDateString(time.getTime(), translate("HH:mm"));

	// Translation: Time prefix as shown in the multiplayer lobby (when you enable it in the options page).
	var timePrefixString = '[font="' + g_SenderFont + '"]' + sprintf(translate("\\[%(time)s]"), { "time": timeString }) + '[/font]';

	// Translation: IRC message format when there is a time prefix.
	return sprintf(translate("%(time)s %(message)s"), { "time": timePrefixString, "message": formattedMessage });
 }

/**
 * Update the spam monitor.
 *
 * @param {string} from - User to update.
 */
function updateSpamMonitor(from)
{
	if (g_SpamMonitor[from])
		++g_SpamMonitor[from][0];
	else
		g_SpamMonitor[from] = [1, Math.floor(Date.now() / 1000), 0];
}

/**
 * Check if a message is spam.
 *
 * @param {string} text - Body of message.
 * @param {string} from - Sender of message.
 *
 * @returns {boolean} - True if message should be blocked.
 */
function isSpam(text, from)
{
	// Integer time in seconds.
	var time = Math.floor(Date.now() / 1000);

	// Initialize if not already in the database.
	if (!g_SpamMonitor[from])
		g_SpamMonitor[from] = [1, time, 0];

	// Block blank lines.
	if (!text.trim())
		return true;

	// Block users who are still within their spam block period.
	if (g_SpamMonitor[from][2] + g_SpamBlockDuration >= time)
		return true;

	// Block users who exceed the rate of 1 message per second for five seconds and are not already blocked.
	if (g_SpamMonitor[from][0] == 6)
	{
		g_SpamMonitor[from][2] = time;
		if (from == g_Username)
			addChatMessage({ "from": "system", "text": translate("Please do not spam. You have been blocked for thirty seconds.") });
		return true;
	}

	return false;
}

/**
 * Reset timer used to measure message send speed.
 * Clear message count every 5 seconds.
 */
function checkSpamMonitor()
{
	var time = Math.floor(Date.now() / 1000);
	for (let i in g_SpamMonitor)
	{
		let stats = g_SpamMonitor[i];
		if (stats[1] + 5 <= time)
		{
			stats[1] = time;
			stats[0] = 0;
		}
	}
}

/**
 *  Generate a (mostly) unique color for this player based on their name.
 *  See http://stackoverflow.com/questions/3426404/create-a-hexadecimal-colour-based-on-a-string-with-jquery-javascript
 *
 *  @param {string} playername
 */
function getPlayerColor(playername)
{
	// Generate a probably-unique hash for the player name and use that to create a color.
	var hash = 0;
	for (var i = 0; i < playername.length; ++i)
		hash = playername.charCodeAt(i) + ((hash << 5) - hash);

	// First create the color in RGB then HSL, clamp the lightness so it's not too dark to read, and then convert back to RGB to display.
	// The reason for this roundabout method is this algorithm can generate values from 0 to 255 for RGB but only 0 to 100 for HSL; this gives
	// us much more variety if we generate in RGB. Unfortunately, enforcing that RGB values are a certain lightness is very difficult, so
	// we convert to HSL to do the computation. Since our GUI code only displays RGB colors, we have to convert back.
	var [h, s, l] = rgbToHsl(hash >> 24 & 0xFF, hash >> 16 & 0xFF, hash >> 8 & 0xFF);
	return hslToRgb(h, s, Math.max(0.7, l)).join(" ");
}

/**
 * Returns the given playername wrapped in an appropriate color-tag.
 *
 *  @param {string} playername
 */
function colorPlayerName(playername)
{
	return '[color="' + getPlayerColor(playername.replace(g_ModeratorPrefix, "")) + '"]' + playername + '[/color]';
}
