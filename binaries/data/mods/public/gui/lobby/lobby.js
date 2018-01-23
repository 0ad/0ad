/**
 * Used for the gamelist-filtering.
 */
const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);

/**
 * Used for the gamelist-filtering.
 */
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);

/**
 *  Used for civ settings display of the selected game. 
 */
const g_CivData = loadCivData(false, false);

/**
 * A symbol which is prepended to the username of moderators.
 */
var g_ModeratorPrefix = "@";

/**
 * Current username. Cannot contain whitespace.
 */
const g_Username = Engine.LobbyGetNick();

/**
 * Lobby server address to construct host JID.
 */
const g_LobbyServer = Engine.ConfigDB_GetValue("user", "lobby.server");

/**
 * Current games will be listed in these colors.
 */
var g_GameColors = {
	"init": "0 219 0",
	"waiting": "255 127 0",
	"running": "219 0 0"
};

/**
 * Initial sorting order of the gamelist.
 */
var g_GameStatusOrder = ["init", "waiting", "running"];

/**
 * The playerlist will be assembled using these values.
 */
var g_PlayerStatuses = {
	"available": { "color": "0 219 0",     "status": translate("Online") },
	"away":      { "color": "229 76 13",   "status": translate("Away") },
	"playing":   { "color": "200 0 0",     "status": translate("Busy") },
	"offline":   { "color": "0 0 0",       "status": translate("Offline") },
	"unknown":   { "color": "178 178 178", "status": translateWithContext("lobby presence", "Unknown") }
};

var g_RoleNames = {
	"moderator": translate("Moderator"),
	"participant": translate("Player"),
	"visitor": translate("Muted Player")
};

/**
 * Color for error messages in the chat.
 */
var g_SystemColor = "150 0 0";

/**
 * Color for private messages in the chat.
 */
var g_PrivateMessageColor = "0 150 0";

/**
 * Used for highlighting the sender of chat messages.
 */
var g_SenderFont = "sans-bold-13";

/**
 * Color to highlight chat commands in the explanation.
 */
var g_ChatCommandColor = "200 200 255";

/**
 * Indicates if the lobby is opened as a dialog or window.
 */
var g_Dialog = false;

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
var g_GameList = [];

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
 * Whether the player was already asked to reconnect to the lobby.
 * Ensures that no more than one message box is opened at a time.
 */
var g_AskedReconnect = false;

/**
 * Processing of notifications sent by XmppClient.cpp.
 *
 * @returns true if the playerlist GUI must be updated.
 */
var g_NetMessageTypes = {
	"system": {
		// Three cases are handled in prelobby.js
		"registered": msg => false,
		"connected": msg => {

			g_AskedReconnect = false;
			updateConnectedState();
			return false;
		},
		"disconnected": msg => {

			updateGameList();
			updateLeaderboard();
			updateConnectedState();

			if (!g_Kicked)
			{
				addChatMessage({
					"from": "system",
					"time": msg.time,
					"text": translate("Disconnected.") + " " + msg.reason
				});
				reconnectMessageBox();
			}

			return true;
		},
		"error": msg => {
			addChatMessage({
				"from": "system",
				"time": msg.time,
				"text": msg.text
			});
			return false;
		}
	},
	"chat": {
		"subject": msg => {
			updateSubject(msg.subject);

			if (msg.nick)
				addChatMessage({
					"text": "/special " + sprintf(translate("%(nick)s changed the lobby subject to %(subject)s"), {
						"nick": msg.nick,
						"subject": msg.subject
					}),
					"time": msg.time,
					"isSpecial": true
				});
			return false;
		},
		"join": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(nick)s has joined."), {
					"nick": msg.nick
				}),
				"time": msg.time,
				"isSpecial": true
			});
			return true;
		},
		"leave": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(nick)s has left."), {
					"nick": msg.nick
				}),
				"time": msg.time,
				"isSpecial": true
			});

			if (msg.nick == g_Username)
				Engine.DisconnectXmppClient();

			return true;
		},
		"presence": msg => true,
		"role": msg => {
			Engine.GetGUIObjectByName("chatInput").hidden = Engine.LobbyGetPlayerRole(g_Username) == "visitor";

			let me = g_Username == msg.nick;
			let newrole = Engine.LobbyGetPlayerRole(msg.nick);
			let txt =
				newrole == "visitor" ?
					me ?
						translate("You have been muted.") :
						translate("%(nick)s has been muted.") :
				newrole == "moderator" ?
					me ?
						translate("You are now a moderator.") :
						translate("%(nick)s is now a moderator.") :
				msg.oldrole == "visitor" ?
					me ?
						translate("You have been unmuted.") :
						translate("%(nick)s has been unmuted.") :
					me ?
						translate("You are not a moderator anymore.") :
						translate("%(nick)s is not a moderator anymore.");

			addChatMessage({
				"text": "/special " + sprintf(txt, { "nick": msg.nick }),
				"time": msg.time,
				"isSpecial": true
			});

			if (g_SelectedPlayer == msg.nick)
				updateUserRoleText(g_SelectedPlayer);

			return false;
		},
		"nick": msg => {
			addChatMessage({
				"text": "/special " + sprintf(translate("%(oldnick)s is now known as %(newnick)s."), {
					"oldnick": msg.oldnick,
					"newnick": msg.newnick
				}),
				"time": msg.time,
				"isSpecial": true
			});
			return true;
		},
		"kicked": msg => {
			handleKick(false, msg.nick, msg.reason, msg.time, msg.historic);
			return true;
		},
		"banned": msg => {
			handleKick(true, msg.nick, msg.reason, msg.time, msg.historic);
			return true;
		},
		"room-message": msg => {
			addChatMessage({
				"from": escapeText(msg.from),
				"text": escapeText(msg.text),
				"time": msg.time,
				"historic": msg.historic
			});
			return false;
		},
		"private-message": msg => {
			// Announcements and the Message of the Day are sent by the server directly
			if (!msg.from)
				messageBox(
					400, 250,
					msg.text.trim(),
					translate("Notice")
				);

			// We intend to not support private messages between users
			if (!msg.from || Engine.LobbyGetPlayerRole(msg.from) == "moderator")
				// some XMPP clients send trailing whitespace
				addChatMessage({
					"from": escapeText(msg.from || "system"),
					"text": escapeText(msg.text.trim()),
					"time": msg.time,
					"historic": msg.historic,
					"private": true
				});
			return false;
		}
	},
	"game": {
		"gamelist": msg => {
			updateGameList();
			return false;
		},
		"profile": msg => {
			updateProfile();
			return false;
		},
		"leaderboard": msg => {
			updateLeaderboard();
			return false;
		},
		"ratinglist": msg => {
			return true;
		}
	}
};

/**
 * Commands that can be entered by clients via chat input.
 * A handler returns true if the user input should be sent as a chat message.
 */
var g_ChatCommands = {
	"away": {
		"description": translate("Set your state to 'Away'."),
		"handler": args => {
			Engine.LobbySetPlayerPresence("away");
			return false;
		}
	},
	"back": {
		"description": translate("Set your state to 'Online'."),
		"handler": args => {
			Engine.LobbySetPlayerPresence("available");
			return false;
		}
	},
	"kick": {
		"description": translate("Kick a specified user from the lobby. Usage: /kick nick reason"),
		"handler": args => {
			Engine.LobbyKick(args[0] || "", args[1] || "");
			return false;
		},
		"moderatorOnly": true
	},
	"ban": {
		"description": translate("Ban a specified user from the lobby. Usage: /ban nick reason"),
		"handler": args => {
			Engine.LobbyBan(args[0] || "", args[1] || "");
			return false;
		},
		"moderatorOnly": true
	},
	"help": {
		"description": translate("Show this help."),
		"handler": args => {
			let isModerator = Engine.LobbyGetPlayerRole(g_Username) == "moderator";
			let text = translate("Chat commands:");
			for (let command in g_ChatCommands)
				if (!g_ChatCommands[command].moderatorOnly || isModerator)
					// Translation: Chat command help format
					text += "\n" + sprintf(translate("%(command)s - %(description)s"), {
						"command": coloredText(command, g_ChatCommandColor),
						"description": g_ChatCommands[command].description
					});

			addChatMessage({
				"from": "system",
				"text": text
			});
			return false;
		}
	},
	"me": {
		"description": translate("Send a chat message about yourself. Example: /me goes swimming."),
		"handler": args => true
	},
	"say": {
		"description": translate("Send text as a chat message (even if it starts with slash). Example: /say /help is a great command."),
		"handler": args => true
	},
	"clear": {
		"description": translate("Clear all chat scrollback."),
		"handler": args => {
			clearChatMessages();
			return false;
		}
	},
	"quit": {
		"description": translate("Return to the main menu."),
		"handler": args => {
			leaveLobby();
			return false;
		}
	}
};

/**
 * Called after the XmppConnection succeeded and when returning from a game.
 *
 * @param {Object} attribs
 */
function init(attribs)
{
	g_Dialog = attribs && attribs.dialog;

	if (!g_Settings)
	{
		leaveLobby();
		return;
	}

	initMusic();
	global.music.setState(global.music.states.MENU);

	initDialogStyle();
	initGameFilters();
	updateConnectedState();

	Engine.LobbySetPlayerPresence("available");

	// When rejoining the lobby after a game, we don't need to process presence changes
	Engine.LobbyClearPresenceUpdates();
	updatePlayerList();
	updateSubject(Engine.LobbyGetRoomSubject());
	updateLobbyColumns();

	updateToggleBuddy();
	Engine.GetGUIObjectByName("chatInput").tooltip = colorizeAutocompleteHotkey();

	// Get all messages since the login
	for (let msg of Engine.LobbyGuiPollHistoricMessages())
		g_NetMessageTypes[msg.type][msg.level](msg);

	if (!Engine.IsXmppClientConnected())
		reconnectMessageBox();
}

function reconnectMessageBox()
{
	if (g_AskedReconnect)
		return;

	g_AskedReconnect = true;

	messageBox(
		400, 200,
		translate("You have been disconnected from the lobby. Do you want to reconnect?"),
		translate("Confirmation"),
		[translate("No"), translate("Yes")],
		[null, Engine.ConnectXmppClient]);
}

/**
 * Set style of GUI elements and the window style.
 */
function initDialogStyle()
{
	let lobbyWindow = Engine.GetGUIObjectByName("lobbyWindow");
	lobbyWindow.sprite = g_Dialog ? "ModernDialog" : "ModernWindow";
	lobbyWindow.size = g_Dialog ? "42 42 100%-42 100%-42" : "0 0 100% 100%";
	Engine.GetGUIObjectByName("lobbyWindowTitle").size = g_Dialog ? "50%-128 -16 50%+128 16" : "50%-128 4 50%+128 36";

	Engine.GetGUIObjectByName("leaveButton").caption = g_Dialog ?
		translateWithContext("previous page", "Back") :
		translateWithContext("previous page", "Main Menu");

	Engine.GetGUIObjectByName("hostButton").hidden = g_Dialog;
	Engine.GetGUIObjectByName("joinGameButton").hidden = g_Dialog;
	Engine.GetGUIObjectByName("gameInfoEmpty").size = "0 0 100% 100%-30" + (g_Dialog ? "" : "-30");
	Engine.GetGUIObjectByName("gameInfo").size = "0 0 100% 100%-30" + (g_Dialog ? "" : "-60");

	if (g_Dialog)
	{
		Engine.GetGUIObjectByName("lobbyDialogToggle").onPress = leaveLobby;
		Engine.GetGUIObjectByName("cancelDialog").onPress = leaveLobby;
	}
}

/**
 * Set style of GUI elements according to the connection state of the lobby.
 */
function updateConnectedState()
{
	Engine.GetGUIObjectByName("chatInput").hidden = !Engine.IsXmppClientConnected();

	for (let button of ["host", "leaderboard", "userprofile", "toggleBuddy"])
		Engine.GetGUIObjectByName(button + "Button").enabled = Engine.IsXmppClientConnected();
}

function updateLobbyColumns()
{
	let gameRating = Engine.ConfigDB_GetValue("user", "lobby.columns.gamerating") == "true";

	// Only show the selected columns
	let gamesBox = Engine.GetGUIObjectByName("gamesBox");
	gamesBox.hidden_mapType = gameRating;
	gamesBox.hidden_gameRating = !gameRating;

	// Only show the filters of selected columns
	let mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
	mapTypeFilter.hidden = gameRating;
	let gameRatingFilter = Engine.GetGUIObjectByName("gameRatingFilter");
	gameRatingFilter.hidden = !gameRating;

	// Keep filters right above the according column
	let playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
	let size = playersNumberFilter.size;
	size.rleft = gameRating ? 74 : 90;
	size.rright = gameRating ? 84 : 100;
	playersNumberFilter.size = size;
}

function leaveLobby()
{
	if (g_Dialog)
	{
		Engine.LobbySetPlayerPresence("playing");
		Engine.PopGuiPage();
	}
	else
	{
		Engine.StopXmppClient();
		Engine.SwitchGuiPage("page_pregame.xml");
	}
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

	let gameRatingOptions = [">1500", ">1400", ">1300", ">1200", "<1200", "<1100", "<1000"];
	gameRatingOptions = prepareForDropdown(gameRatingOptions.map(r => ({
		"value": r,
		"label": sprintf(
			r[0] == ">" ?
				translateWithContext("gamelist filter", "> %(rating)s") :
				translateWithContext("gamelist filter", "< %(rating)s"),
			{ "rating": r.substr(1) })
	})));

	let gameRatingFilter = Engine.GetGUIObjectByName("gameRatingFilter");
	gameRatingFilter.list = [translateWithContext("map", "Any")].concat(gameRatingOptions.label);
	gameRatingFilter.list_data = [""].concat(gameRatingOptions.value);

	resetFilters();
}

function resetFilters()
{
	Engine.GetGUIObjectByName("mapSizeFilter").selected = 0;
	Engine.GetGUIObjectByName("playersNumberFilter").selected = 0;
	Engine.GetGUIObjectByName("mapTypeFilter").selected = g_MapTypes.Default;
	Engine.GetGUIObjectByName("gameRatingFilter").selected = 0;
	Engine.GetGUIObjectByName("filterOpenGames").checked = false;

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
	let gameRatingFilter = Engine.GetGUIObjectByName("gameRatingFilter");
	let filterOpenGames = Engine.GetGUIObjectByName("filterOpenGames");

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

	if (filterOpenGames.checked && (game.nbp >= game.maxnbp || game.state != "init"))
		return true;

	if (gameRatingFilter.selected > 0)
	{
		let selected = gameRatingFilter.list_data[gameRatingFilter.selected];
		if (selected.startsWith(">") && +selected.substr(1) >= game.gameRating ||
		    selected.startsWith("<") && +selected.substr(1) <= game.gameRating)
			return true;
	}

	return false;
}

function handleKick(banned, nick, reason, time, historic)
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
			"time": time,
			"historic": historic,
			"isSpecial": true
		});
		return;
	}

	addChatMessage({
		"from": "system",
		"time": time,
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
 * Update the caption of the toggle buddy button.
 */
function updateToggleBuddy()
{
	let playerList = Engine.GetGUIObjectByName("playersBox");
	let playerName = playerList.list[playerList.selected];

	let toggleBuddyButton = Engine.GetGUIObjectByName("toggleBuddyButton");
	toggleBuddyButton.caption = g_Buddies.indexOf(playerName) != -1 ? translate("Unmark as Buddy") : translate("Mark as Buddy");
	toggleBuddyButton.enabled = playerName && playerName != g_Username;
}

/**
 * Do a full update of the player listing, including ratings from cached C++ information.
 */
function updatePlayerList()
{
	let playersBox = Engine.GetGUIObjectByName("playersBox");
	let sortBy = playersBox.selected_column || "name";
	let sortOrder = playersBox.selected_column_order || 1;

	let buddyStatusList = [];
	let playerList = [];
	let presenceList = [];
	let nickList = [];
	let ratingList = [];

	let cleanPlayerList = Engine.GetPlayerList().map(player => {
		player.isBuddy = g_Buddies.indexOf(player.name) != -1;
		return player;
	}).sort((a, b) => {
		let sortA, sortB;
		let statusOrder = Object.keys(g_PlayerStatuses);
		let statusA = statusOrder.indexOf(a.presence) + a.name.toLowerCase();
		let statusB = statusOrder.indexOf(b.presence) + b.name.toLowerCase();

		switch (sortBy)
		{
		case 'buddy':
			sortA = (a.isBuddy ? 1 : 2) + statusA;
			sortB = (b.isBuddy ? 1 : 2) + statusB;
			break;
		case 'rating':
			sortA = +a.rating;
			sortB = +b.rating;
			break;
		case 'status':
			sortA = statusA;
			sortB = statusB;
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
		buddyStatusList.push(player.isBuddy ? coloredText(g_BuddySymbol, statusColor) : "");
		playerList.push(colorPlayerName((player.role == "moderator" ? g_ModeratorPrefix : "") + player.name));
		presenceList.push(coloredText(g_PlayerStatuses[presence].status, statusColor));
		ratingList.push(coloredText(rating, statusColor));
		nickList.push(player.name);
	}

	playersBox.list_buddy = buddyStatusList;
	playersBox.list_name = playerList;
	playersBox.list_status = presenceList;
	playersBox.list_rating = ratingList;
	playersBox.list = nickList;

	playersBox.selected = playersBox.list.indexOf(g_SelectedPlayer);
}

/**
* Toggle buddy state for a player in playerlist within the user config
*/
function toggleBuddy()
{
	let playerList = Engine.GetGUIObjectByName("playersBox");
	let name = playerList.list[playerList.selected];

	if (!name || name == g_Username || name.indexOf(g_BuddyListDelimiter) != -1)
		return;

	let index = g_Buddies.indexOf(name);
	if (index != -1)
		g_Buddies.splice(index, 1);
	else
		g_Buddies.push(name);

	updateToggleBuddy();

	// Don't save empty strings to the config file
	let buddies = g_Buddies.filter(nick => nick).join(g_BuddyListDelimiter) || g_BuddyListDelimiter;
	Engine.ConfigDB_CreateValue("user", "lobby.buddies", buddies);
	Engine.ConfigDB_WriteValueToFile("user", "lobby.buddies", buddies, "config/user.cfg");

	updatePlayerList();
	updateGameList();
}

/**
 * Select the game where the selected player is currently playing, observing or offline.
 * Selects in that order to account for players that occur in multiple games.
 */
function selectGameFromPlayername()
{
	if (!g_SelectedPlayer)
		return;

	let gameList = Engine.GetGUIObjectByName("gamesBox");
	let foundAsObserver = false;

	for (let i = 0; i < g_GameList.length; ++i)
		for (let player of stringifiedTeamListToPlayerData(g_GameList[i].players))
		{
			if (g_SelectedPlayer != splitRatingFromNick(player.Name).nick)
				continue;

			gameList.auto_scroll = true;
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

function onPlayerListSelection()
{
	let playerList = Engine.GetGUIObjectByName("playersBox");
	if (playerList.selected == playerList.list.indexOf(g_SelectedPlayer))
		return;

	g_SelectedPlayer = playerList.list[playerList.selected];
	lookupSelectedUserProfile("playersBox");
	updateToggleBuddy();
	selectGameFromPlayername();
}

function setLeaderboardVisibility(visible)
{
	if (visible)
		Engine.SendGetBoardList();

	lookupSelectedUserProfile(visible ? "leaderboardBox" : "playersBox");
	Engine.GetGUIObjectByName("leaderboard").hidden = !visible;
	Engine.GetGUIObjectByName("fade").hidden = !visible;
}

function setUserProfileVisibility(visible)
{
	Engine.GetGUIObjectByName("profileFetch").hidden = !visible;
	Engine.GetGUIObjectByName("fade").hidden = !visible;
}

/**
 * Display the profile of the player in the user profile window.
 */
function lookupUserProfile()
{
	Engine.SendGetProfile(Engine.GetGUIObjectByName("fetchInput").caption);
}

/**
 * Display the profile of the selected player in the main window.
 * Displays N/A for all stats until updateProfile is called when the stats
 * are actually received from the bot.
 */
function lookupSelectedUserProfile(guiObjectName)
{
	let playerList = Engine.GetGUIObjectByName(guiObjectName);
	let playerName = playerList.list[playerList.selected];

	Engine.GetGUIObjectByName("profileArea").hidden = !playerName && !Engine.GetGUIObjectByName("usernameText").caption;
	if (!playerName)
		return;

	Engine.SendGetProfile(playerName);

	Engine.GetGUIObjectByName("usernameText").caption = playerName;
	Engine.GetGUIObjectByName("rankText").caption = translate("N/A");
	Engine.GetGUIObjectByName("highestRatingText").caption = translate("N/A");
	Engine.GetGUIObjectByName("totalGamesText").caption = translate("N/A");
	Engine.GetGUIObjectByName("winsText").caption = translate("N/A");
	Engine.GetGUIObjectByName("lossesText").caption = translate("N/A");
	Engine.GetGUIObjectByName("ratioText").caption = translate("N/A");

	updateUserRoleText(playerName);
}

function updateUserRoleText(playerName)
{
	Engine.GetGUIObjectByName("roleText").caption = g_RoleNames[Engine.LobbyGetPlayerRole(playerName) || "participant"];
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
		list_rank.push(+i + 1);
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
	let sortBy = gamesBox.selected_column;
	let sortOrder = gamesBox.selected_column_order;

	if (gamesBox.selected > -1)
	{
		g_SelectedGameIP = g_GameList[gamesBox.selected].ip;
		g_SelectedGamePort = g_GameList[gamesBox.selected].port;
	}

	g_GameList = Engine.GetGameList().map(game => {

		game.hasBuddies = 0;

		// Compute average rating of participating players
		let playerRatings = [];

		for (let player of stringifiedTeamListToPlayerData(game.players))
		{
			let playerNickRating = splitRatingFromNick(player.Name);

			if (player.Team != "observer")
				playerRatings.push(playerNickRating.rating || g_DefaultLobbyRating);

			// Sort games with playing buddies above games with spectating buddies
			if (game.hasBuddies < 2 && g_Buddies.indexOf(playerNickRating.nick) != -1)
				game.hasBuddies = player.Team == "observer" ? 1 : 2;
		}

		game.gameRating =
			playerRatings.length ?
				Math.round(playerRatings.reduce((sum, current) => sum + current) / playerRatings.length) :
				g_DefaultLobbyRating;

		return game;
	}).filter(game => !filterGame(game)).sort((a, b) => {
		let sortA, sortB;
		switch (sortBy)
		{
		case 'name':
			sortA = g_GameStatusOrder.indexOf(a.state) + a.name.toLowerCase();
			sortB = g_GameStatusOrder.indexOf(b.state) + b.name.toLowerCase();
			break;
		case 'gameRating':
		case 'mapSize':
		case 'mapType':
			sortA = a[sortBy];
			sortB = b[sortBy];
			break;
		case 'buddy':
			sortA = String(b.hasBuddies) + g_GameStatusOrder.indexOf(a.state) + a.name.toLowerCase();
			sortB = String(a.hasBuddies) + g_GameStatusOrder.indexOf(b.state) + b.name.toLowerCase();
			break;
		case 'mapName':
			sortA = translate(a.niceMapName);
			sortB = translate(b.niceMapName);
			break;
		case 'nPlayers':
			sortA = a.maxnbp;
			sortB = b.maxnbp;
			break;
		}
		if (sortA < sortB) return -sortOrder;
		if (sortA > sortB) return +sortOrder;
		return 0;
	});

	let list_buddy = [];
	let list_name = [];
	let list_mapName = [];
	let list_mapSize = [];
	let list_mapType = [];
	let list_nPlayers = [];
	let list_gameRating = [];
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

		list_buddy.push(game.hasBuddies ? coloredText(g_BuddySymbol, g_GameColors[game.state]) : "");
		list_name.push(coloredText(gameName, g_GameColors[game.state]));
		list_mapName.push(translateMapTitle(game.niceMapName));
		list_mapSize.push(translateMapSize(game.mapSize));
		list_mapType.push(g_MapTypes.Title[mapTypeIdx] || "");
		list_nPlayers.push(game.nbp + "/" + game.maxnbp);
		list_gameRating.push(game.gameRating);
		list.push(gameName);
		list_data.push(i);
	}

	gamesBox.list_buddy = list_buddy;
	gamesBox.list_name = list_name;
	gamesBox.list_mapName = list_mapName;
	gamesBox.list_mapSize = list_mapSize;
	gamesBox.list_mapType = list_mapType;
	gamesBox.list_nPlayers = list_nPlayers;
	gamesBox.list_gameRating = list_gameRating;

	// Change these last, otherwise crash
	gamesBox.list = list;
	gamesBox.list_data = list_data;

	gamesBox.auto_scroll = false;
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
	Engine.GetGUIObjectByName("joinGameButton").hidden = g_Dialog || !game;
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
				"time": Engine.FormatMillisecondsIntoDateStringLocal(+game.startTime * 1000, translate("HH:mm"))
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
	if (!game || g_Dialog)
		return;

	let rating = getRejoinRating(game);
	let username = rating ? g_Username + " (" + rating + ")" : g_Username;

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

	let ip;
	let port;
	if (game.stunIP)
	{
		ip = game.stunIP;
		port = game.stunPort;
	}
	else
	{
		ip = game.ip;
		port = game.port;
	}

	if (ip.split('.').length != 4)
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
		"ip": ip,
		"port": port,
		"name": g_Username,
		"rating": getRejoinRating(game),
		"useSTUN": !!game.stunIP,
		"hostJID": game.hostUsername + "@" + g_LobbyServer + "/0ad"
	});
}

/**
 * Rejoin games with the original playername, even if the rating changed meanwhile.
 */
function getRejoinRating(game)
{
	for (let player of stringifiedTeamListToPlayerData(game.players))
	{
		let playerNickRating = splitRatingFromNick(player.Name);
		if (playerNickRating.nick == g_Username)
			return playerNickRating.rating;
	}
	return g_UserRating;
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

	let updateList = false;

	while (true)
	{
		let msg = Engine.LobbyGuiPollNewMessage();
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

		if (g_NetMessageTypes[msg.type][msg.level](msg))
			updateList = true;
	}

	// To improve performance, only update the playerlist GUI when
	// the last update in the current stack is processed
	if (updateList)
		updatePlayerList();
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

	if (handleChatCommand(text))
		Engine.LobbySendMessage(text);

	input.caption = "";
}

/**
 * Handle all '/' commands.
 *
 * @param {string} text - Text to be checked for commands.
 * @returns {boolean} true if the text should be sent via chat.
 */
function handleChatCommand(text)
{
	if (text[0] != '/')
		return true;

	let [cmd, args] = ircSplit(text);
	args = ircSplit("/" + args);

	if (!g_ChatCommands[cmd])
	{
		addChatMessage({
			"from": "system",
			"text": sprintf(
				translate("The command '%(cmd)s' is not supported."), {
					"cmd": coloredText(cmd, g_ChatCommandColor)
				})
		});
		return false;
	}

	if (g_ChatCommands[cmd].moderatorOnly && Engine.LobbyGetPlayerRole(g_Username) != "moderator")
	{
		addChatMessage({
			"from": "system",
			"text": sprintf(
				translate("The command '%(cmd)s' is restricted to moderators."), {
					"cmd": coloredText(cmd, g_ChatCommandColor)
				})
		});
		return false;
	}

	return g_ChatCommands[cmd].handler(args);
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

			if (!msg.historic && msg.text.toLowerCase().indexOf(g_Username.toLowerCase()) != -1)
				soundNotification("nick");
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
		return [string.substr(1, idx - 1), string.substr(idx + 1)];

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

	// Handle commands allowed past handleChatCommand.
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
		default:
			return "";
		}
	}
	else
	{
		let senderString;

		// Translation: IRC message prefix.
		if (msg.private)
			senderString = sprintf(translateWithContext("lobby private message", "(%(private)s) <%(sender)s>"), {
				"private": coloredText(translate("Private"), g_PrivateMessageColor),
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

	// Translation: Time as shown in the multiplayer lobby (when you enable it in the options page).
	// For a list of symbols that you can use, see:
	// https://sites.google.com/site/icuprojectuserguide/formatparse/datetime?pli=1#TOC-Date-Field-Symbol-Table
	let timeString = Engine.FormatMillisecondsIntoDateStringLocal(msg.time ? msg.time * 1000 : Date.now(), translate("HH:mm"));

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
 * Generate a (mostly) unique color for this player based on their name.
 * @see http://stackoverflow.com/questions/3426404/create-a-hexadecimal-colour-based-on-a-string-with-jquery-javascript
 * @param {string} playername
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
 * @param {string} playername
 * @param {string} rating
 */
function colorPlayerName(playername, rating)
{
	return coloredText(
		(rating ? sprintf(
			translate("%(nick)s (%(rating)s)"), {
				"nick": playername,
				"rating": rating
			}) : playername
		),
		getPlayerColor(playername.replace(g_ModeratorPrefix, "")));
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
