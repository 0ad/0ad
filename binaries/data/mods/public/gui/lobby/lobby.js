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
 * Remembers how to sort the columns in the lobby playerlist / gamelist.
 * TODO: move logic to c++ / xml
 */
var g_GameListSortBy = "name";
var g_PlayerListSortBy = "name";
var g_GameListOrder = 1; // 1 for ascending sort, and -1 for descending
var g_PlayerListOrder = 1;

function init(attribs)
{
	if (!g_Settings)
	{
		returnToMainMenu();
		return;
	}

	// Play menu music
	initMusic();
	global.music.setState(global.music.states.MENU);

	// Init mapsize filter
	var mapSizes = initMapSizes();
	mapSizes.shortNames.splice(0, 0, translateWithContext("map size", "Any"));
	mapSizes.tiles.splice(0, 0, "");
	var mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	mapSizeFilter.list = mapSizes.shortNames;
	mapSizeFilter.list_data = mapSizes.tiles;

	// Setup number-of-players filter
	var playersArray = Array(g_MaxPlayers).fill(0).map((v, i) => i + 1); // 1, 2, ... MaxPlayers
	var playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
	playersNumberFilter.list = [translateWithContext("player number", "Any")].concat(playersArray);
	playersNumberFilter.list_data = [""].concat(playersArray);

	var mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
	mapTypeFilter.list = [translateWithContext("map", "Any")].concat(g_MapTypes.Title);
	mapTypeFilter.list_data = [""].concat(g_MapTypes.Name);

	Engine.LobbySetPlayerPresence("available");
	Engine.SendGetGameList();
	Engine.SendGetBoardList();

	// When rejoining the lobby after a game, we don't need to process presence changes
	Engine.LobbyClearPresenceUpdates();
	updatePlayerList();

	updateSubject(Engine.LobbyGetRoomSubject());

	resetFilters();
}

function returnToMainMenu()
{
	lobbyStop();
	Engine.SwitchGuiPage("page_pregame.xml");
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

function updateGameListOrderSelection()
{
	g_GameListSortBy = Engine.GetGUIObjectByName("gamesBox").selected_column;
	g_GameListOrder = Engine.GetGUIObjectByName("gamesBox").selected_column_order;

	applyFilters();
}

function updatePlayerListOrderSelection()
{
	g_PlayerListSortBy = Engine.GetGUIObjectByName("playersBox").selected_column;
	g_PlayerListOrder = Engine.GetGUIObjectByName("playersBox").selected_column_order;

	updatePlayerList();
}

function resetFilters()
{
	// Reset states of gui objects
	Engine.GetGUIObjectByName("mapSizeFilter").selected = 0;
	Engine.GetGUIObjectByName("playersNumberFilter").selected = 0;
	Engine.GetGUIObjectByName("mapTypeFilter").selected = g_MapTypes.Default;
	Engine.GetGUIObjectByName("showFullFilter").checked = false;

	applyFilters();
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
	subject.caption = newSubject;
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
 * Do a full update of the player listing, including ratings from cached C++ information.
 *
 * @return Array containing the player, presence, nickname, and rating listings.
 */
function updatePlayerList()
{
	var playersBox = Engine.GetGUIObjectByName("playersBox");
	var playerList = [];
	var presenceList = [];
	var nickList = [];
	var ratingList = [];
	var cleanPlayerList = Engine.GetPlayerList();

	// Sort the player list, ignoring case.
	cleanPlayerList.sort((a, b) => {
		switch (g_PlayerListSortBy)
		{
		case 'rating':
			if (+a.rating < +b.rating)
				return -g_PlayerListOrder;
			else if (+a.rating > +b.rating)
				return g_PlayerListOrder;
			return 0;
		case 'status':
			let order = ["available", "away", "playing", "offline"];
			let presenceA = order.indexOf(a.presence);
			let presenceB = order.indexOf(b.presence);
			if (presenceA < presenceB)
				return -g_PlayerListOrder;
			else if (presenceA > presenceB)
				return g_PlayerListOrder;
			return 0;
		case 'name':
		default:
			var aName = a.name.toLowerCase();
			var bName = b.name.toLowerCase();
			if (aName < bName)
				return -g_PlayerListOrder;
			else if (aName > bName)
				return g_PlayerListOrder;
			return 0;
		}
	});

	for (var i = 0; i < cleanPlayerList.length; ++i)
	{
		// Identify current user's rating.
		if (cleanPlayerList[i].name == g_Username && cleanPlayerList[i].rating)
			g_UserRating = cleanPlayerList[i].rating;
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
		for (var index = 0; index < 4 - Math.ceil(Math.log(cleanPlayerList[i].rating) / Math.LN10); ++index)
			ratingSpaces += "  ";
		ratingList.push(String(ratingSpaces + rating));
	}
	playersBox.list_name = playerList;
	playersBox.list_status = presenceList;
	playersBox.list_rating = ratingList;
	playersBox.list = nickList;
	if (playersBox.selected >= playersBox.list.length)
		playersBox.selected = -1;
}

/**
 * Display the profile of the selected player.
 * Displays N/A for all stats until updateProfile is called when the stats
 * 	are actually received from the bot.
 *
 * @param caller From which screen is the user requesting data from?
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

	if (!playerList.list[playerList.selected])
	{
		Engine.GetGUIObjectByName("profileArea").hidden = true;
		return;
	}
	Engine.GetGUIObjectByName("profileArea").hidden = false;

	Engine.SendGetProfile(playerList.list[playerList.selected]);

	var user = playerList.list_name[playerList.selected];
	var role = Engine.LobbyGetPlayerRole(playerList.list[playerList.selected]);
	var userList = Engine.GetGUIObjectByName("playersBox");
	if (role && caller == "lobbylist")
	{
		// Make the role uppercase.
		role = role.charAt(0).toUpperCase() + role.slice(1);
		if (role == "Moderator")
			role = '[color="0 219 0"]' + translate(role) + '[/color]';
	}
	else
		role = "";

	Engine.GetGUIObjectByName("usernameText").caption = user;
	Engine.GetGUIObjectByName("roleText").caption = translate(role);
	Engine.GetGUIObjectByName("rankText").caption = translate("N/A");
	Engine.GetGUIObjectByName("highestRatingText").caption = translate("N/A");
	Engine.GetGUIObjectByName("totalGamesText").caption = translate("N/A");
	Engine.GetGUIObjectByName("winsText").caption = translate("N/A");
	Engine.GetGUIObjectByName("lossesText").caption = translate("N/A");
	Engine.GetGUIObjectByName("ratioText").caption = translate("N/A");
}

/**
 * Update the profile of the selected player with data from the bot.
 *
 */
function updateProfile()
{
	var playerList, user;
	var attributes = Engine.GetProfile();

	if (!Engine.GetGUIObjectByName("profileFetch").hidden)
	{
		user = attributes[0].player;
		if (attributes[0].rating == "-2") // Profile not found code
		{
			Engine.GetGUIObjectByName("profileWindowArea").hidden = true;
			Engine.GetGUIObjectByName("profileErrorText").hidden = false;
			return;
		}
		Engine.GetGUIObjectByName("profileWindowArea").hidden = false;
		Engine.GetGUIObjectByName("profileErrorText").hidden = true;

		if (attributes[0].rating != "")
			user = sprintf(translate("%(nick)s (%(rating)s)"), { nick: user, rating: attributes[0].rating });

		Engine.GetGUIObjectByName("profileUsernameText").caption = user;
		Engine.GetGUIObjectByName("profileRankText").caption = attributes[0].rank;
		Engine.GetGUIObjectByName("profileHighestRatingText").caption = attributes[0].highestRating;
		Engine.GetGUIObjectByName("profileTotalGamesText").caption = attributes[0].totalGamesPlayed;
		Engine.GetGUIObjectByName("profileWinsText").caption = attributes[0].wins;
		Engine.GetGUIObjectByName("profileLossesText").caption = attributes[0].losses;

		var winRate = (attributes[0].wins / attributes[0].totalGamesPlayed * 100).toFixed(2);
		if (attributes[0].totalGamesPlayed != 0)
			Engine.GetGUIObjectByName("profileRatioText").caption = sprintf(translate("%(percentage)s%%"), { percentage: winRate });
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
		user = sprintf(translate("%(nick)s (%(rating)s)"), { nick: user, rating: attributes[0].rating });

	Engine.GetGUIObjectByName("usernameText").caption = user;
	Engine.GetGUIObjectByName("rankText").caption = attributes[0].rank;
	Engine.GetGUIObjectByName("highestRatingText").caption = attributes[0].highestRating;
	Engine.GetGUIObjectByName("totalGamesText").caption = attributes[0].totalGamesPlayed;
	Engine.GetGUIObjectByName("winsText").caption = attributes[0].wins;
	Engine.GetGUIObjectByName("lossesText").caption = attributes[0].losses;

	var winRate = (attributes[0].wins / attributes[0].totalGamesPlayed * 100).toFixed(2);
	if (attributes[0].totalGamesPlayed != 0)
		Engine.GetGUIObjectByName("ratioText").caption = sprintf(translate("%(percentage)s%%"), { percentage: winRate });
	else
		Engine.GetGUIObjectByName("ratioText").caption = translateWithContext("Used for an undefined winning rate", "-");
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
	boardList.sort((a, b) => b.rating - a.rating);

	var list = [];
	var list_name = [];
	var list_rank = [];
	var list_rating = [];

	// Push changes
	for (var i = 0; i < boardList.length; ++i)
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

	// Sort the list of games to that games 'waiting' are displayed at the top, followed by 'init', followed by 'running'.
	var gameStatuses = ['waiting', 'init', 'running'];
	g_GameList.sort((a, b) => {
		switch (g_GameListSortBy)
		{
		case 'name':
		case 'mapSize':
			// mapSize contains the number of tiles for random maps
			// scenario maps always display default size
		case 'mapType':
			if (a[g_GameListSortBy] < b[g_GameListSortBy])
				return -g_GameListOrder;
			else if (a[g_GameListSortBy] > b[g_GameListSortBy])
				return g_GameListOrder;
			return 0;
		case 'mapName':
			if (translate(a.niceMapName) < translate(b.niceMapName))
				return -g_GameListOrder;
			else if (translate(a.niceMapName) > translate(b.niceMapName))
				return g_GameListOrder;
			return 0;
		case 'nPlayers':
			// Numerical comparison of player count ratio.
			if (a.nbp * b.tnbp < b.nbp * a.tnbp) // ratio a = a.nbp / a.tnbp, ratio b = b.nbp / b.tnbp
				return -g_GameListOrder;
			else if (a.nbp * b.tnbp > b.nbp * a.tnbp)
				return g_GameListOrder;
			return 0;
		default:
			if (gameStatuses.indexOf(a.state) < gameStatuses.indexOf(b.state))
				return -1;
			else if (gameStatuses.indexOf(a.state) > gameStatuses.indexOf(b.state))
				return 1;

			// Alphabetical comparison of names as tiebreaker.
			if (a.name < b.name)
				return -1;
			else if (a.name > b.name)
				return 1;
			return 0;
		}
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
	for (var g of gameList)
	{
		if (!filterGame(g))
		{
			// 'waiting' games are highlighted in orange, 'running' in red, and 'init' in green.
			let name = escapeText(g.name);
			if (g.state == 'init')
				name = '[color="0 219 0"]' + name + '[/color]';
			else if (g.state == 'waiting')
				name = '[color="255 127 0"]' + name + '[/color]';
			else
				name = '[color="219 0 0"]' + name + '[/color]';
			list_name.push(name);
			list_ip.push(g.ip);
			list_mapName.push(translate(g.niceMapName));
			list_mapSize.push(translateMapSize(g.mapSize));
			let mapTypeIdx = g_MapTypes.Name.indexOf(g.mapType);
			list_mapType.push(mapTypeIdx != -1 ? g_MapTypes.Title[mapTypeIdx] : "");
			list_nPlayers.push(g.nbp + "/" +g.tnbp);
			list.push(name);
			list_data.push(c);
		}
		c++;
	}

	gamesBox.list_name = list_name;
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
		color = "200 0 0";
		status = translate("Busy");
		break;
	case "away":
		color = "229 76 13";
		status = translate("Away");
		break;
	case "available":
		color = "0 219 0";
		status = translate("Online");
		break;
	case "offline":
		color = "0 0 0";
		status = translate("Offline");
		break;
	default:
		warn(sprintf("Unknown presence '%(presence)s'", { "presence": presence }));
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
		nickname = g_ModeratorPrefix + nickname;
	var formattedName = colorPlayerName(nickname);

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

	var g = Engine.GetGUIObjectByName("gamesBox").list_data[selected];

	// Show the game info panel and join button.
	Engine.GetGUIObjectByName("gameInfo").hidden = false;
	Engine.GetGUIObjectByName("joinGameButton").hidden = false;
	Engine.GetGUIObjectByName("gameInfoEmpty").hidden = true;

	// Display the map name, number of players, the names of the players, the map size and the map type.
	Engine.GetGUIObjectByName("sgMapName").caption = translate(g_GameList[g].niceMapName);
	Engine.GetGUIObjectByName("sgNbPlayers").caption = g_GameList[g].nbp + "/" + g_GameList[g].tnbp;
	Engine.GetGUIObjectByName("sgPlayersNames").caption = g_GameList[g].players;
	Engine.GetGUIObjectByName("sgMapSize").caption = translateMapSize(g_GameList[g].mapSize);
	let mapTypeIdx = g_MapTypes.Name.indexOf(g_GameList[g].mapType);
	Engine.GetGUIObjectByName("sgMapType").caption = mapTypeIdx != -1 ? g_MapTypes.Title[mapTypeIdx] : "";


	// Display map preview if it exists, otherwise display a placeholder.
	if (mapData && mapData.settings.Preview)
		var mapPreview = mapData.settings.Preview;
	else
		var mapPreview = "nopreview.png";

	// Display map description and preview (or placeholder)
	var mapData = getMapDescriptionAndPreview(g_GameList[g].mapType, g_GameList[g].mapName);
	Engine.GetGUIObjectByName("sgMapDescription").caption = mapData.description;
	Engine.GetGUIObjectByName("sgMapPreview").sprite = "cropped:(0.7812,0.5859)session/icons/mappreview/" + mapData.preview;
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
		var sname = g_Username;
		var sip = g_GameList[g].ip;

		// TODO: What about valid host names?
		// Check if it looks like an ip address
		if (sip.split('.').length != 4)
		{
			addChatMessage({ "from": "system", "text": sprintf(translate("This game's address '%(ip)s' does not appear to be valid."), { "ip": sip }) });
			return;
		}

		// Open Multiplayer connection window with join option.
		Engine.PushGuiPage("page_gamesetup_mp.xml", { "multiplayerGameType": "join", "name": sname, "ip": sip, "rating": g_UserRating });
	}
}

/**
 * Start the hosting process.
 */
function hostGame()
{
	// Open Multiplayer connection window with host option.
	Engine.PushGuiPage("page_gamesetup_mp.xml", { "multiplayerGameType": "host", "name": g_Username, "rating": g_UserRating });
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
			addChatMessage({ "from": from, "text": text, "datetime": message.datetime});
			break;
		case "message": // For private messages
			var from = escapeText(message.from);
			addChatMessage({ "from": from, "text": text, "datetime": message.datetime});
			break;
		case "muc":
			var nick = message.text;
			switch(message.level)
			{
			case "join":
				addChatMessage({ "text": "/special " + sprintf(translate("%(nick)s has joined."), { "nick": nick }), "isSpecial": true });
				Engine.SendGetRatingList();
				break;
			case "leave":
				addChatMessage({ "text": "/special " + sprintf(translate("%(nick)s has left."), { "nick": nick }), "isSpecial": true });
				break;
			case "nick":
				addChatMessage({ "text": "/special " + sprintf(translate("%(oldnick)s is now known as %(newnick)s."), { "oldnick": nick, "newnick": message.data }), "isSpecial": true });
				break;
			case "presence":
				break;
			case "subject":
				updateSubject(message.text);
				break;
			default:
				warn(sprintf("Unknown message.level '%(msglvl)s'", { "msglvl": message.level }));
				break;
			}
			// We might receive many join/leaves when returning to the lobby from a long game.
			// To improve performance, only update the playerlist GUI when the last update in the current stack is processed
			if (Engine.LobbyGetMucMessageCount() == 0)
				updatePlayerList();
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
				case "profile updated":
					updateProfile();
					break;
				}
				break;
			}
			break;
		default:
			error(sprintf("Unrecognised message type %(msgtype)s", { "msgtype": message.type }));
		}
	}
}

/* Messages */
function submitChatInput()
{
	var input = Engine.GetGUIObjectByName("chatInput");
	var text = input.caption;
	if (text.length)
	{
		if (!handleSpecialCommand(text) && !isSpam(text, g_Username))
			Engine.LobbySendMessage(text);
		input.caption = "";
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
		returnToMainMenu();
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
			msg.from = g_ModeratorPrefix + msg.from;
	}
	else
		msg.from = null;
	if (!msg.color)
		msg.color = null;
	if (!msg.key)
		msg.key = null;
	if (!msg.datetime)
		msg.datetime = null;

	// Highlight local user's nick
	if (g_Username != msg.from)
		msg.text = msg.text.replace(g_Username, colorPlayerName(g_Username));

	// Run spam test if it's not a historical message
	if (!msg.datetime)
		updateSpamMonitor(msg.from);
	if (isSpam(msg.text, msg.from))
		return;

	// Format Text
	var formatted = ircFormat(msg.text, msg.from, msg.color, msg.key, msg.datetime, msg.isSpecial || false);

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
 * @param datetime Current date and time of message, only used for historical messages
 * @return Formatted text.
 */
function ircFormat(text, from, color, key, datetime, isSpecial)
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
				var senderString = '[font="sans-bold-13"]' + sprintf(translate("* %(sender)s"), { "sender": coloredFrom }) + '[/font]';
				// Translation: IRC message issued using the ‘/me’ command.
				var formattedMessage = sprintf(translate("%(sender)s %(action)s"), { "sender": senderString, "action": message });
				break;
			case "say":
				// Translation: IRC message prefix.
				var senderString = '[font="sans-bold-13"]' + sprintf(translate("<%(sender)s>"), { "sender": coloredFrom }) + '[/font]';
				// Translation: IRC message.
				var formattedMessage = sprintf(translate("%(sender)s %(message)s"), { "sender": senderString, "message": message });
				break;
			case "special":
				if (isSpecial)
					// Translation: IRC system message.
					var formattedMessage = '[font="sans-bold-13"]' + sprintf(translate("== %(message)s"), { "message": message }) + '[/font]';
				else
				{
					// Translation: IRC message prefix.
					var senderString = '[font="sans-bold-13"]' + sprintf(translate("<%(sender)s>"), { "sender": coloredFrom }) + '[/font]';
					// Translation: IRC message.
					var formattedMessage = sprintf(translate("%(sender)s %(message)s"), { "sender": senderString, "message": message });
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
		var senderString = '[font="sans-bold-13"]' + sprintf(translate("<%(sender)s>"), { "sender": coloredFrom }) + '[/font]';
		// Translation: IRC message.
		var formattedMessage = sprintf(translate("%(sender)s %(message)s"), { "sender": senderString, "message": text });
	}

	// Build time header if enabled
	if (g_ShowTimestamp)
	{

		var time;
		if (datetime)
		{
			var parserDate = datetime.split("T")[0].split("-");
			var parserTime = datetime.split("T")[1].split(":");
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
		var timePrefixString = '[font="sans-bold-13"]' + sprintf(translate("\\[%(time)s]"), { "time": timeString }) + '[/font]';

		// Translation: IRC message format when there is a time prefix.
		return sprintf(translate("%(time)s %(message)s"), { "time": timePrefixString, "message": formattedMessage });
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
	if (g_SpamMonitor[from])
		g_SpamMonitor[from][0]++;
	else
		g_SpamMonitor[from] = [1, time, 0];
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
	if (!g_SpamMonitor[from])
		g_SpamMonitor[from] = [1, time, 0];

	// Block blank lines.
	if (text == " ")
		return true;
	// Block users who are still within their spam block period.
	else if (g_SpamMonitor[from][2] + g_SpamBlockDuration >= time)
		return true;
	// Block users who exceed the rate of 1 message per second for five seconds and are not already blocked. TODO: Make this smarter and block profanity.
	else if (g_SpamMonitor[from][0] == 6)
	{
		g_SpamMonitor[from][2] = time;
		if (from == g_Username)
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
	for each (var stats in g_SpamMonitor)
	{
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
 */
function colorPlayerName(playername)
{
	return '[color="' + getPlayerColor(playername.replace(g_ModeratorPrefix, "")) + '"]' + playername + '[/color]';
}
