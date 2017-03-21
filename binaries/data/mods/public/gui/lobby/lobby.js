/**
 * Used for the gamelist-filtering.
 */
const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);

/**
 * Used for the gamelist-filtering.
 */
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);

/**
 * Mute clients who exceed the rate of 1 message per second for this time
 */
const g_SpamBlockTimeframe = 5;

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
 * Initial sorting order of the gamelist.
 */
const g_GameStatusOrder = ["init", "waiting", "running"];

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
 * Color for private messages in the chat.
 */
const g_PrivateMessageColor = "0 150 0";

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
 * Used to restore the selection after updating the gamelist.
 */
var g_SelectedGamePort = "";

/**
 * Whether the current user has been kicked or banned.
 */
var g_Kicked = false;

/**
 * Notifications sent by XmppClient.cpp
 */
var g_NetMessageTypes = {
	"system": {
		// Three cases are handled in prelobby.js
		"registered": msg => {
		},
		"connected": msg => {
		},
		"disconnected": msg => {

			updateGameList();
			updateLeaderboard();
			updatePlayerList();

			for (let button of ["host", "leaderboard", "userprofile"])
				Engine.GetGUIObjectByName(button + "Button").enabled = false;

			if (!g_Kicked)
				addChatMessage({
					"from": "system",
					"text": translate("Disconnected.") + " " + msg.text
				});
		},
		"error": msg => {
			addChatMessage({
				"from": "system",
				"text": msg.text
			});
		}
	},
	"chat": {
		"subject": msg => {
			updateSubject(msg.text);
		},
		"join": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(nick)s has joined."), {
					"nick": msg.text
				}),
				"isSpecial": true
			});
			Engine.SendGetRatingList();
		},
		"leave": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(nick)s has left."), {
					"nick": msg.text
				}),
				"isSpecial": true
			});

			if (msg.text == g_Username)
				Engine.DisconnectXmppClient();
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
		"kicked": msg => {
			handleKick(false, msg.text, msg.data || "");
		},
		"banned": msg => {
			handleKick(true, msg.text, msg.data || "");
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
				// some XMPP clients send trailing whitespace
				addChatMessage({
					"from": escapeText(msg.from),
					"text": escapeText(msg.text.trim()),
					"datetime": msg.datetime,
					"private" : true
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

	// When rejoining the lobby after a game, we don't need to process presence changes
	Engine.LobbyClearPresenceUpdates();
	updatePlayerList();
	updateSubject(Engine.LobbyGetRoomSubject());

	Engine.GetGUIObjectByName("chatInput").tooltip = colorizeAutocompleteHotkey();
}

function returnToMainMenu()
{
	Engine.StopXmppClient();
	Engine.SwitchGuiPage("page_pregame.xml");
}

function initGameFilters()
{
	let mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	mapSizeFilter.list = [translateWithContext("map size", "Any")].concat(g_MapSizes.Name);
	mapSizeFilter.list_data = [""].concat(g_MapSizes.Tiles);

	let playersArray = Array(g_MaxPlayers).fill(0).map((v, i) => i + 1); // 1, 2, ... MaxPlayers
	let playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
	playersNumberFilter.list = [translateWithContext("player number", "Any")].concat(playersArray);
	playersNumberFilter.list_data = [""].concat(playersArray);

	let mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
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
	let mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	let playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
	let mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
	let showFullFilter = Engine.GetGUIObjectByName("showFullFilter");

	// We assume index 0 means display all for any given filter.
	if (mapSizeFilter.selected != 0 &&
	    game.mapSize != mapSizeFilter.list_data[mapSizeFilter.selected])
		return true;

	if (playersNumberFilter.selected != 0 &&
	    game.maxnbp != playersNumberFilter.list_data[playersNumberFilter.selected])
		return true;

	if (mapTypeFilter.selected != 0 &&
	    game.mapType != mapTypeFilter.list_data[mapTypeFilter.selected])
		return true;

	if (!showFullFilter.checked && game.maxnbp <= game.nbp)
		return true;

	return false;
}

function handleKick(banned, nick, reason)
{
	let kickString = nick == g_Username ?
		banned ?
			translate("You have been banned from the lobby!") :
			translate("You have been kicked from the lobby!") :
		banned ?
			translate("%(nick)s has been banned from the lobby.") :
			translate("%(nick)s has been kicked from the lobby.");

	if (reason)
		reason = sprintf(translateWithContext("lobby kick", "Reason: %(reason)s"), {
			"reason": reason
		});

	if (nick != g_Username)
	{
		addChatMessage({
			"text": "/special " + sprintf(kickString, { "nick": nick }) + " " + reason,
			"isSpecial": true
		});
		return;
	}

	addChatMessage({
		"from": "system",
		"text": kickString + " " + reason,
	});

	g_Kicked = true;

	Engine.DisconnectXmppClient();

	messageBox(
		400, 250,
		kickString + "\n" + reason,
		banned ? translate("BANNED") : translate("KICKED")
	);
}

/**
 * Update the subject GUI object.
 *
 * @param {string} newSubject
 */
function updateSubject(newSubject)
{
	Engine.GetGUIObjectByName("subject").caption = newSubject;

	// If the subject is only whitespace, hide it and reposition the logo.
	let subjectBox = Engine.GetGUIObjectByName("subjectBox");
	subjectBox.hidden = !newSubject.trim();

	let logo = Engine.GetGUIObjectByName("logo");
	if (subjectBox.hidden)
		logo.size = "50%-110 50%-50 50%+110 50%+50";
	else
		logo.size = "50%-110 40 50%+110 140";
}

/**
 * Do a full update of the player listing, including ratings from cached C++ information.
 */
function updatePlayerList()
{
	let playersBox = Engine.GetGUIObjectByName("playersBox");
	let sortBy = playersBox.selected_column || "name";
	let sortOrder = playersBox.selected_column_order || 1;

	if (playersBox.selected > -1)
		g_SelectedPlayer = playersBox.list[playersBox.selected];

	let playerList = [];
	let presenceList = [];
	let nickList = [];
	let ratingList = [];

	let cleanPlayerList = Engine.GetPlayerList().sort((a, b) => {
		let sortA, sortB;
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

	// To reduce rating-server load, only send the GUI event if the selection actually changed
	if (playersBox.selected != playersBox.list.indexOf(g_SelectedPlayer))
		playersBox.selected = playersBox.list.indexOf(g_SelectedPlayer);
}

/**
 * Select the game listing the selected player when toggling the full games filter.
 */
function selectGameFromSelectedPlayername()
{
	let playerList = Engine.GetGUIObjectByName("playersBox");
	if (playerList.selected >= 0)
		selectGameFromPlayername(playerList.list[playerList.selected]);
}

/**
 * Select the game where the given player is currently playing, observing or offline.
 * Selects in that order to account for players that occur in multiple games.
 */
function selectGameFromPlayername(playerName)
{
	let gameList = Engine.GetGUIObjectByName("gamesBox");
	let foundAsObserver = false;

	for (let i = 0; i < g_GameList.length; ++i)
		for (let player of stringifiedTeamListToPlayerData(g_GameList[i].players))
		{
			let result = /^(\S+)\ \(\d+\)$/g.exec(player.Name);
			let nick = result ? result[1] : player.Name;

			if (playerName != nick)
				continue;

			if (player.Team == "observer")
			{
				foundAsObserver = true;
				gameList.selected = i;
			}
			else if (!player.Offline)
			{
				gameList.selected = i;
				return;
			}
			else if (!foundAsObserver)
				gameList.selected = i;
		}
}

/**
 * Display the profile of the selected player.
 * Displays N/A for all stats until updateProfile is called when the stats
 * are actually received from the bot.
 *
 * @param {string} caller - From which screen is the user requesting data from?
 */
function displayProfile(caller)
{
	let playerList;

	if (caller == "leaderboard")
		playerList = Engine.GetGUIObjectByName("leaderboardBox");
	else if (caller == "lobbylist")
	{
		playerList = Engine.GetGUIObjectByName("playersBox");
		if (playerList.selected != -1)
			selectGameFromPlayername(playerList.list[playerList.selected]);
	}
	else if (caller == "fetch")
	{
		Engine.SendGetProfile(Engine.GetGUIObjectByName("fetchInput").caption);
		return;
	}
	else
		return;

	let playerName = playerList.list[playerList.selected];
	Engine.GetGUIObjectByName("profileArea").hidden = !playerName;
	if (!playerName)
		return;

	Engine.SendGetProfile(playerName);

	let isModerator = Engine.LobbyGetPlayerRole(playerName) == "moderator";
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
	let attributes = Engine.GetProfile()[0];

	let user = colorPlayerName(attributes.player, attributes.rating);

	if (!Engine.GetGUIObjectByName("profileFetch").hidden)
	{
		let profileFound = attributes.rating != "-2";
		Engine.GetGUIObjectByName("profileWindowArea").hidden = !profileFound;
		Engine.GetGUIObjectByName("profileErrorText").hidden = profileFound;

		if (!profileFound)
		{
			Engine.GetGUIObjectByName("profileErrorText").caption = sprintf(
				translate("Player \"%(nick)s\" not found."),
				{ "nick": attributes.player }
			);
			return;
		}

		Engine.GetGUIObjectByName("profileUsernameText").caption = user;
		Engine.GetGUIObjectByName("profileRankText").caption = attributes.rank;
		Engine.GetGUIObjectByName("profileHighestRatingText").caption = attributes.highestRating;
		Engine.GetGUIObjectByName("profileTotalGamesText").caption = attributes.totalGamesPlayed;
		Engine.GetGUIObjectByName("profileWinsText").caption = attributes.wins;
		Engine.GetGUIObjectByName("profileLossesText").caption = attributes.losses;
		Engine.GetGUIObjectByName("profileRatioText").caption = formatWinRate(attributes);
		return;
	}

	let playerList;
	if (!Engine.GetGUIObjectByName("leaderboard").hidden)
		playerList = Engine.GetGUIObjectByName("leaderboardBox");
	else
		playerList = Engine.GetGUIObjectByName("playersBox");

	if (attributes.rating == "-2")
		return;

	// Make sure the stats we have received coincide with the selected player.
	if (attributes.player != playerList.list[playerList.selected])
		return;

	Engine.GetGUIObjectByName("usernameText").caption = user;
	Engine.GetGUIObjectByName("rankText").caption = attributes.rank;
	Engine.GetGUIObjectByName("highestRatingText").caption = attributes.highestRating;
	Engine.GetGUIObjectByName("totalGamesText").caption = attributes.totalGamesPlayed;
	Engine.GetGUIObjectByName("winsText").caption = attributes.wins;
	Engine.GetGUIObjectByName("lossesText").caption = attributes.losses;
	Engine.GetGUIObjectByName("ratioText").caption = formatWinRate(attributes);
}

/**
 * Update the leaderboard from data cached in C++.
 */
function updateLeaderboard()
{
	let leaderboard = Engine.GetGUIObjectByName("leaderboardBox");
	let boardList = Engine.GetBoardList().sort((a, b) => b.rating - a.rating);

	let list = [];
	let list_name = [];
	let list_rank = [];
	let list_rating = [];

	for (let i in boardList)
	{
		list_name.push(boardList[i].name);
		list_rating.push(boardList[i].rating);
		list_rank.push(+i+1);
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
	let gamesBox = Engine.GetGUIObjectByName("gamesBox");
	let sortBy = gamesBox.selected_column || "status";
	let sortOrder = gamesBox.selected_column_order || 1;

	if (gamesBox.selected > -1)
	{
		g_SelectedGameIP = g_GameList[gamesBox.selected].ip;
		g_SelectedGamePort = g_GameList[gamesBox.selected].port;
	}

	g_GameList = Engine.GetGameList().filter(game => !filterGame(game)).sort((a, b) => {
		let sortA, sortB;
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
			sortA = a.maxnbp;
			sortB = b.maxnbp;
			break;
		case 'status':
		default:
			sortA = g_GameStatusOrder.indexOf(a.state);
			sortB = g_GameStatusOrder.indexOf(b.state);
			break;
		}
		if (sortA < sortB) return -sortOrder;
		if (sortA > sortB) return +sortOrder;
		return 0;
	});

	let list_name = [];
	let list_mapName = [];
	let list_mapSize = [];
	let list_mapType = [];
	let list_nPlayers = [];
	let list = [];
	let list_data = [];
	let selectedGameIndex = -1;

	for (let i in g_GameList)
	{
		let game = g_GameList[i];
		let gameName = escapeText(game.name);
		let mapTypeIdx = g_MapTypes.Name.indexOf(game.mapType);

		if (game.ip == g_SelectedGameIP && game.port == g_SelectedGamePort)
			selectedGameIndex = +i;

		list_name.push('[color="' + g_GameColors[game.state] + '"]' + gameName);
		list_mapName.push(translateMapTitle(game.niceMapName));
		list_mapSize.push(translateMapSize(game.mapSize));
		list_mapType.push(g_MapTypes.Title[mapTypeIdx] || "");
		list_nPlayers.push(game.nbp + "/" + game.maxnbp);
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
	let game = selectedGame();

	Engine.GetGUIObjectByName("gameInfo").hidden = !game;
	Engine.GetGUIObjectByName("joinGameButton").hidden = !game;
	Engine.GetGUIObjectByName("gameInfoEmpty").hidden = game;

	if (!game)
		return;

	Engine.GetGUIObjectByName("sgMapName").caption = translateMapTitle(game.niceMapName);

	let sgGameStartTime = Engine.GetGUIObjectByName("sgGameStartTime");
	let sgNbPlayers = Engine.GetGUIObjectByName("sgNbPlayers");
	let sgPlayersNames = Engine.GetGUIObjectByName("sgPlayersNames");

	let playersNamesSize = sgPlayersNames.size;
	playersNamesSize.top = game.startTime ? sgGameStartTime.size.bottom : sgNbPlayers.size.bottom;
	playersNamesSize.rtop = game.startTime ? sgGameStartTime.size.rbottom : sgNbPlayers.size.rbottom;
	sgPlayersNames.size = playersNamesSize;

	sgGameStartTime.hidden = !game.startTime;
	if (game.startTime)
		sgGameStartTime.caption = sprintf(
			// Translation: %(time)s is the hour and minute here.
			translate("Game started at %(time)s"), {
				"time": Engine.FormatMillisecondsIntoDateStringLocal(+game.startTime*1000, translate("HH:mm"))
			});

	sgNbPlayers.caption = sprintf(
		translate("Players: %(current)s/%(total)s"), {
			"current": game.nbp,
			"total": game.maxnbp
		});

	sgPlayersNames.caption = formatPlayerInfo(stringifiedTeamListToPlayerData(game.players));
	Engine.GetGUIObjectByName("sgMapSize").caption = translateMapSize(game.mapSize);

	let mapTypeIdx = g_MapTypes.Name.indexOf(game.mapType);
	Engine.GetGUIObjectByName("sgMapType").caption = g_MapTypes.Title[mapTypeIdx] || "";

	let mapData = getMapDescriptionAndPreview(game.mapType, game.mapName);
	Engine.GetGUIObjectByName("sgMapDescription").caption = mapData.description;

	setMapPreviewImage("sgMapPreview", mapData.preview);
}

function selectedGame()
{
	let gamesBox = Engine.GetGUIObjectByName("gamesBox");
	if (gamesBox.selected < 0)
		return undefined;

	return g_GameList[gamesBox.list_data[gamesBox.selected]];
}

/**
 * Immediately rejoin and join gamesetups. Otherwise confirm late-observer join attempt.
 */
function joinButton()
{
	let game = selectedGame();
	if (!game)
		return;

	let username = g_UserRating ? g_Username + " (" + g_UserRating + ")" : g_Username;

	if (game.state == "init" || stringifiedTeamListToPlayerData(game.players).some(player => player.Name == username))
		joinSelectedGame();
	else
		messageBox(
			400, 200,
			translate("The game has already started. Do you want to join as observer?"),
			translate("Confirmation"),
			[translate("No"), translate("Yes")],
			[null, joinSelectedGame]
		);
}

/**
 * Attempt to join the selected game without asking for confirmation.
 */
function joinSelectedGame()
{
	let game = selectedGame();
	if (!game)
		return;

	if (game.ip.split('.').length != 4)
	{
		addChatMessage({
			"from": "system",
			"text": sprintf(
				translate("This game's address '%(ip)s' does not appear to be valid."),
				{ "ip": game.ip }
			)
		});
		return;
	}

	Engine.PushGuiPage("page_gamesetup_mp.xml", {
		"multiplayerGameType": "join",
		"ip": game.ip,
		"port": game.port,
		"name": g_Username,
		"rating": g_UserRating
	});
}

/**
 * Open the dialog box to enter the game name.
 */
function hostGame()
{
	Engine.PushGuiPage("page_gamesetup_mp.xml", {
		"multiplayerGameType": "host",
		"name": g_Username,
		"rating": g_UserRating
	});
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

		// To improve performance, only update the playerlist GUI when
		// the last update in the current stack is processed
		if (msg.type == "chat" && Engine.LobbyGetMucMessageCount() == 0)
			updatePlayerList();
	}
}

/**
 * Executes a lobby command or sends GUI input directly as chat.
 */
function submitChatInput()
{
	let input = Engine.GetGUIObjectByName("chatInput");
	let text = input.caption;

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

	let [cmd, args] = ircSplit(text);
	let [nick, reason] = ircSplit("/" + args);

	switch (cmd)
	{
	case "away":
		Engine.LobbySetPlayerPresence("away");
		break;
	case "back":
		Engine.LobbySetPlayerPresence("available");
		break;
	case "kick":
		Engine.LobbyKick(nick, reason);
		break;
	case "ban":
		Engine.LobbyBan(nick, reason);
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
			"text": sprintf(
				translate("We're sorry, the '%(cmd)s' command is not supported."),
				{ "cmd": cmd }
			)
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
		{
			msg.text = msg.text.replace(g_Username, colorPlayerName(g_Username));
			notifyUser(g_Username, msg.text);
		}

		// Run spam test if it's not a historical message
		if (!msg.datetime)
		{
			updateSpamMonitor(msg.from);
			if (isSpam(msg.text, msg.from))
				return;
		}
	}

	let formatted = ircFormat(msg);
	if (!formatted)
		return;

	g_ChatMessages.push(formatted);
	Engine.GetGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}

/**
 * Splits given input into command and argument.
 */
function ircSplit(string)
{
	let idx = string.indexOf(' ');

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
	let formattedMessage = "";
	let coloredFrom = msg.from && colorPlayerName(msg.from);

	// Handle commands allowed past handleSpecialCommand.
	if (msg.text[0] == '/')
	{
		let [command, message] = ircSplit(msg.text);
		switch (command)
		{
		case "me":
		{
			// Translation: IRC message prefix when the sender uses the /me command.
			let senderString = sprintf(translate("* %(sender)s"), {
				"sender": coloredFrom
			});

			// Translation: IRC message issued using the ‘/me’ command.
			formattedMessage = sprintf(translate("%(sender)s %(action)s"), {
				"sender": senderFont(senderString),
				"action": message
			});
			break;
		}
		case "say":
		{
			// Translation: IRC message prefix.
			let senderString = sprintf(translate("<%(sender)s>"), {
				"sender": coloredFrom
			});

			// Translation: IRC message.
			formattedMessage = sprintf(translate("%(sender)s %(message)s"), {
				"sender": senderFont(senderString),
				"message": message
			});
			break;
		}
		case "special":
		{
			if (msg.isSpecial)
				// Translation: IRC system message.
				formattedMessage = senderFont(sprintf(translate("== %(message)s"), {
					"message": message
				}));
			else
			{
				// Translation: IRC message prefix.
				let senderString = sprintf(translate("<%(sender)s>"), {
					"sender": coloredFrom
				});

				// Translation: IRC message.
				formattedMessage = sprintf(translate("%(sender)s %(message)s"), {
					"sender": senderFont(senderString),
					"message": message
				});
			}
			break;
		}
		}
	}
	else
	{
		let senderString;

		// Translation: IRC message prefix.
		if (msg.private)
			senderString = sprintf(translateWithContext("lobby private message", "(%(private)s) <%(sender)s>"), {
				"private": '[color="' + g_PrivateMessageColor + '"]' +
							translate("Private")  + '[/color]',
				"sender": coloredFrom
			});
		else
			senderString = sprintf(translate("<%(sender)s>"), {
				"sender": coloredFrom
			});

		// Translation: IRC message.
		formattedMessage = sprintf(translate("%(sender)s %(message)s"), {
			"sender": senderFont(senderString),
			"message": msg.text
		});
	}

	// Add chat message timestamp
	if (Engine.ConfigDB_GetValue("user", "chat.timestamp") != "true")
		return formattedMessage;

	let time;
	if (msg.datetime)
	{
		let dTime = msg.datetime.split("T");
		let parserDate = dTime[0].split("-");
		let parserTime = dTime[1].split(":");
		// See http://xmpp.org/extensions/xep-0082.html#sect-idp285136 for format of datetime
		// Date takes Year, Month, Day, Hour, Minute, Second
		time = new Date(Date.UTC(parserDate[0], parserDate[1], parserDate[2],
			parserTime[0], parserTime[1], parserTime[2].split("Z")[0]));
	}
	else
		time = new Date(Date.now());

	// Translation: Time as shown in the multiplayer lobby (when you enable it in the options page).
	// For a list of symbols that you can use, see:
	// https://sites.google.com/site/icuprojectuserguide/formatparse/datetime?pli=1#TOC-Date-Field-Symbol-Table
	let timeString = Engine.FormatMillisecondsIntoDateStringLocal(time.getTime(), translate("HH:mm"));

	// Translation: Time prefix as shown in the multiplayer lobby (when you enable it in the options page).
	let timePrefixString = sprintf(translate("\\[%(time)s]"), {
		"time": timeString
	});

	// Translation: IRC message format when there is a time prefix.
	return sprintf(translate("%(time)s %(message)s"), {
		"time": senderFont(timePrefixString),
		"message": formattedMessage
	});
 }

/**
 * Update the spam monitor.
 *
 * @param {string} from - User to update.
 */
function updateSpamMonitor(from)
{
	if (g_SpamMonitor[from])
		++g_SpamMonitor[from].count;
	else
		g_SpamMonitor[from] = {
			"count": 1,
			"lastSend": Math.floor(Date.now() / 1000),
			"lastBlock": 0
		};
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
	let time = Math.floor(Date.now() / 1000);

	// Initialize if not already in the database.
	if (!g_SpamMonitor[from])
		g_SpamMonitor[from] = {
			"count": 1,
			"lastSend": time,
			"lastBlock": 0
		};

	// Block blank lines.
	if (!text.trim())
		return true;

	// Block users who are still within their spam block period.
	if (g_SpamMonitor[from].lastBlock + g_SpamBlockDuration >= time)
		return true;

	// Block users who exceed the rate of 1 message per second for
	// five seconds and are not already blocked.
	if (g_SpamMonitor[from].count == g_SpamBlockTimeframe + 1)
	{
		g_SpamMonitor[from].lastBlock = time;

		if (from == g_Username)
			addChatMessage({
				"from": "system",
				"text": translate("Please do not spam. You have been blocked for thirty seconds.")
			});

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
	let time = Math.floor(Date.now() / 1000);

	for (let i in g_SpamMonitor)
	{
		// Reset the spam-status after being silent long enough
		if (g_SpamMonitor[i].lastSend + g_SpamBlockTimeframe <= time)
		{
			g_SpamMonitor[i].count = 0;
			g_SpamMonitor[i].lastSend = time;
		}
	}
}

/**
 *  Generate a (mostly) unique color for this player based on their name.
 *  @see http://stackoverflow.com/questions/3426404/create-a-hexadecimal-colour-based-on-a-string-with-jquery-javascript
 *  @param {string} playername
 */
function getPlayerColor(playername)
{
	if (playername == "system")
		return g_SystemColor;

	// Generate a probably-unique hash for the player name and use that to create a color.
	let hash = 0;
	for (let i in playername)
		hash = playername.charCodeAt(i) + ((hash << 5) - hash);

	// First create the color in RGB then HSL, clamp the lightness so it's not too dark to read, and then convert back to RGB to display.
	// The reason for this roundabout method is this algorithm can generate values from 0 to 255 for RGB but only 0 to 100 for HSL; this gives
	// us much more variety if we generate in RGB. Unfortunately, enforcing that RGB values are a certain lightness is very difficult, so
	// we convert to HSL to do the computation. Since our GUI code only displays RGB colors, we have to convert back.
	let [h, s, l] = rgbToHsl(hash >> 24 & 0xFF, hash >> 16 & 0xFF, hash >> 8 & 0xFF);
	return hslToRgb(h, s, Math.max(0.7, l)).join(" ");
}

/**
 * Returns the given playername wrapped in an appropriate color-tag.
 *
 *  @param {string} playername
 *  @param {string} rating
 */
function colorPlayerName(playername, rating)
{
	return '[color="' + getPlayerColor(playername.replace(g_ModeratorPrefix, "")) + '"]' +
		(rating ? sprintf(
			translate("%(nick)s (%(rating)s)"), {
				"nick": playername,
				"rating": rating
			}) :
		playername) + '[/color]';
}

function senderFont(text)
{
	return '[font="' + g_SenderFont + '"]' + text + "[/font]";
}

function formatWinRate(attr)
{
	if (!attr.totalGamesPlayed)
		return translateWithContext("Used for an undefined winning rate", "-");

	return sprintf(translate("%(percentage)s%%"), {
		"percentage": (attr.wins / attr.totalGamesPlayed * 100).toFixed(2)
	});
}
