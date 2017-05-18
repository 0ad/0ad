/**
 * All known cheat commands.
 * @type {Object}
 */
const g_Cheats = getCheatsData();

/**
 * Number of seconds after which chatmessages will disappear.
 */
const g_ChatTimeout = 30;

/**
 * Maximum number of lines to display simultaneously.
 */
const g_ChatLines = 20;

/**
 * The currently displayed strings, limited by the given timeframe and limit above.
 */
var g_ChatMessages = [];

/**
 * All unparsed chat messages received since connect, including timestamp.
 */
var g_ChatHistory = [];

/**
 * Holds the timer-IDs used for hiding the chat after g_ChatTimeout seconds.
 */
var g_ChatTimers = [];

/**
 * Command to send to the previously selected private chat partner.
 */
var g_LastChatAddressee = "";

/**
 * Handle all netmessage types that can occur.
 */
var g_NetMessageTypes = {
	"netstatus": msg => {
		handleNetStatusMessage(msg);
	},
	"netwarn": msg => {
		addNetworkWarning(msg);
	},
	"out-of-sync": msg => {
		onNetworkOutOfSync(msg);
	},
	"players": msg => {
		handlePlayerAssignmentsMessage(msg);
	},
	"paused": msg => {
		setClientPauseState(msg.guid, msg.pause);
	},
	"clients-loading": msg => {
		handleClientsLoadingMessage(msg.guids);
	},
	"rejoined": msg => {
		addChatMessage({
			"type": "rejoined",
			"guid": msg.guid
		});
	},
	"kicked": msg => {
		addChatMessage({
			"type": "kicked",
			"username": msg.username,
			"banned": msg.banned
		});
	},
	"chat": msg => {
		addChatMessage({
			"type": "message",
			"guid": msg.guid,
			"text": msg.text
		});
	},
	"aichat": msg => {
		addChatMessage({
			"type": "message",
			"guid": msg.guid,
			"text": msg.text,
			"translate": true
		});
	},
	"gamesetup": msg => {}, // Needed for autostart
	"start": msg => {}
};

var g_FormatChatMessage = {
	"system": msg => msg.text,
	"connect": msg =>
		sprintf(
			g_PlayerAssignments[msg.guid].player != -1 ?
				// Translation: A player that left the game joins again
				translate("%(player)s is starting to rejoin the game.") :
				// Translation: A player joins the game for the first time
				translate("%(player)s is starting to join the game."),
			{ "player": colorizePlayernameByGUID(msg.guid) }
		),
	"disconnect": msg =>
		sprintf(translate("%(player)s has left the game."), {
			"player": colorizePlayernameByGUID(msg.guid)
		}),
	"rejoined": msg =>
		sprintf(
			g_PlayerAssignments[msg.guid].player != -1 ?
				// Translation: A player that left the game joins again
				translate("%(player)s has rejoined the game.") :
				// Translation: A player joins the game for the first time
				translate("%(player)s has joined the game."),
			{ "player": colorizePlayernameByGUID(msg.guid) }
		),
	"kicked": msg =>
		sprintf(
			msg.banned ?
				translate("%(username)s has been banned") :
				translate("%(username)s has been kicked"),
			{
				"username": colorizePlayernameHelper(
					msg.username,
					g_Players.findIndex(p => p.name == msg.username)
				)
			}
		),
	"clientlist": msg => getUsernameList(),
	"message": msg => formatChatCommand(msg),
	"defeat": msg => formatDefeatMessage(msg),
	"won": msg => formatWinMessage(msg),
	"diplomacy": msg => formatDiplomacyMessage(msg),
	"tribute": msg => formatTributeMessage(msg),
	"barter": msg => formatBarterMessage(msg),
	"attack": msg => formatAttackMessage(msg),
	"phase": msg => formatPhaseMessage(msg)
};

/**
 * Show a label and grey overlay or hide both on connection change.
 */
var g_StatusMessageTypes = {
	"authenticated": msg => translate("Connection to the server has been authenticated."),
	"connected": msg => translate("Connected to the server."),
	"disconnected": msg => translate("Connection to the server has been lost.") + "\n" +
		// Translation: States the reason why the client disconnected from the server.
		sprintf(translate("Reason: %(reason)s."), {
			"reason": getDisconnectReason(msg.reason, true)
		}),
	"waiting_for_players": msg => translate("Waiting for players to connect:"),
	"join_syncing": msg => translate("Synchronising gameplay with other players..."),
	"active": msg => ""
};

/**
 * Chatmessage shown after commands like /me or /enemies.
 */
var g_ChatCommands = {
	"regular": {
		"context": translate("(%(context)s) %(userTag)s %(message)s"),
		"no-context": translate("%(userTag)s %(message)s")
	},
	"me": {
		"context": translate("(%(context)s) * %(user)s %(message)s"),
		"no-context": translate("* %(user)s %(message)s")
	}
};

var g_ChatAddresseeContext = {
	"/team": translate("Team"),
	"/allies": translate("Ally"),
	"/enemies": translate("Enemy"),
	"/observers": translate("Observer"),
	"/msg": translate("Private")
};

/**
 * Returns true if the current player is an addressee, given the chat message type and sender.
 */
var g_IsChatAddressee = {
	"/team": senderID =>
		g_Players[senderID] &&
		g_Players[Engine.GetPlayerID()] &&
		g_Players[Engine.GetPlayerID()].team != -1 &&
		g_Players[Engine.GetPlayerID()].team == g_Players[senderID].team,

	"/allies": senderID =>
		g_Players[senderID] &&
		g_Players[Engine.GetPlayerID()] &&
		g_Players[senderID].isMutualAlly[Engine.GetPlayerID()],

	"/enemies": senderID =>
		g_Players[senderID] &&
		g_Players[Engine.GetPlayerID()] &&
		g_Players[senderID].isEnemy[Engine.GetPlayerID()],

	"/observers": senderID =>
		g_IsObserver,

	"/msg": (senderID, addresseeGUID) =>
		addresseeGUID == Engine.GetPlayerGUID()
};

/**
 * Notice only messages will be filtered that are visible to the player in the first place.
 */
var g_ChatHistoryFilters = [
	{
		"key": "all",
		"text": translateWithContext("chat history filter", "Chat and notifications"),
		"filter": (msg, senderID) => true
	},
	{
		"key": "chat",
		"text": translateWithContext("chat history filter", "Chat messages"),
		"filter": (msg, senderID) => msg.type == "message"
	},
	{
		"key": "player",
		"text": translateWithContext("chat history filter", "Players chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			senderID > 0 && !isPlayerObserver(senderID)
	},
	{
		"key": "ally",
		"text": translateWithContext("chat history filter", "Ally chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			msg.cmd && msg.cmd == "/allies"
	},
	{
		"key": "enemy",
		"text": translateWithContext("chat history filter", "Enemy chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			msg.cmd && msg.cmd == "/enemies"
	},
	{
		"key": "observer",
		"text": translateWithContext("chat history filter", "Observer chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			msg.cmd && msg.cmd == "/observers"
	},
	{
		"key": "private",
		"text": translateWithContext("chat history filter", "Private chat"),
		"filter": (msg, senderID) => !!msg.isVisiblePM
	}
];

var g_PlayerStateMessages = {
	"won": translate("You have won!"),
	"defeated": translate("You have been defeated!")
};

/**
 * Chatmessage shown on diplomacy change.
 */
var g_DiplomacyMessages = {
	"active": {
		"ally": translate("You are now allied with %(player)s."),
		"enemy": translate("You are now at war with %(player)s."),
		"neutral": translate("You are now neutral with %(player)s.")
	},
	"passive": {
		"ally": translate("%(player)s is now allied with you."),
		"enemy": translate("%(player)s is now at war with you."),
		"neutral": translate("%(player)s is now neutral with you.")
	},
	"observer": {
		"ally": translate("%(player)s is now allied with %(player2)s."),
		"enemy": translate("%(player)s is now at war with %(player2)s."),
		"neutral": translate("%(player)s is now neutral with %(player2)s.")
	}
};

/**
 * Defines how the GUI reacts to notifications that are sent by the simulation.
 * Don't open new pages (message boxes) here! Otherwise further notifications
 * handled in the same turn can't access the GUI objects anymore.
 */
var g_NotificationsTypes =
{
	"chat": function(notification, player)
	{
		let message = {
			"type": "message",
			"guid": findGuidForPlayerID(player) || -1,
			"text": notification.message
		};
		if (message.guid == -1)
			message.player = player;

		addChatMessage(message);
	},
	"aichat": function(notification, player)
	{
		let message = {
			"type": "message",
			"text": notification.message,
			"guid": findGuidForPlayerID(player) || -1,
			"player": player,
			"translate": true
		};

		if (notification.translateParameters)
		{
			message.translateParameters = notification.translateParameters;
			message.parameters = notification.parameters;
			colorizePlayernameParameters(notification.parameters);
		}

		addChatMessage(message);
	},
	"defeat": function(notification, player)
	{
		addChatMessage({
			"type": "defeat",
			"guid": findGuidForPlayerID(player),
			"player": player,
			"resign": !!notification.resign
		});
		playerFinished(player, false);
		sendLobbyPlayerlistUpdate();
	},
	"won": function(notification, player)
	{
		addChatMessage({
			"type": "won",
			"guid": findGuidForPlayerID(player),
			"player": player
		});
		playerFinished(player, true);
		sendLobbyPlayerlistUpdate();
	},
	"diplomacy": function(notification, player)
	{
		addChatMessage({
			"type": "diplomacy",
			"sourcePlayer": player,
			"targetPlayer": notification.targetPlayer,
			"status": notification.status
		});
		updatePlayerData();
	},
	"ceasefire-ended": function(notification, player)
	{
		updatePlayerData();
	},
	"tutorial": function(notification, player)
	{
		updateTutorial(notification);
	},
	"tribute": function(notification, player)
	{
		addChatMessage({
			"type": "tribute",
			"sourcePlayer": notification.donator,
			"targetPlayer": player,
			"amounts": notification.amounts
		});
	},
	"barter": function(notification, player)
	{
		addChatMessage({
			"type": "barter",
			"player": player,
			"amountsSold": notification.amountsSold,
			"amountsBought": notification.amountsBought,
			"resourceSold": notification.resourceSold,
			"resourceBought": notification.resourceBought
		});
	},
	"spy-response": function(notification, player)
	{
		if (g_ViewedPlayer == player)
			setCameraFollow(notification.entity);
	},
	"attack": function(notification, player)
	{
		if (player != g_ViewedPlayer)
			return;

		// Focus camera on attacks
		if (g_FollowPlayer)
		{
			setCameraFollow(notification.target);

			g_Selection.reset();
			if (notification.target)
				g_Selection.addList([notification.target]);
		}

		if (Engine.ConfigDB_GetValue("user", "gui.session.notifications.attack") !== "true")
			return;

		addChatMessage({
			"type": "attack",
			"player": player,
			"attacker": notification.attacker,
			"targetIsDomesticAnimal": notification.targetIsDomesticAnimal
		});
	},
	"phase": function(notification, player)
	{
		addChatMessage({
			"type": "phase",
			"player": player,
			"phaseName": notification.phaseName,
			"phaseState": notification.phaseState
		});
	},
	"dialog": function(notification, player)
	{
		if (player == Engine.GetPlayerID())
			openDialog(notification.dialogName, notification.data, player);
	},
	"resetselectionpannel": function(notification, player)
	{
		if (player != Engine.GetPlayerID())
			return;
		g_Selection.rebuildSelection({});
	},
	"playercommand": function(notification, player)
	{
		// For observers, focus the camera on units commanded by the selected player
		if (!g_FollowPlayer || player != g_ViewedPlayer)
			return;

		let cmd = notification.cmd;

		// Ignore boring animals
		let entState = cmd.entities && cmd.entities[0] && GetEntityState(cmd.entities[0]);
		if (entState && entState.identity && entState.identity.classes &&
				entState.identity.classes.indexOf("Animal") != -1)
			return;

		// Focus the building to construct
		if (cmd.type == "repair")
		{
			let targetState = GetEntityState(cmd.target);
			if (targetState)
				Engine.CameraMoveTo(targetState.position.x, targetState.position.z);
		}
		// Focus commanded entities, but don't lose previous focus when training units
		else if (cmd.type != "train" && cmd.type != "research" && entState)
			setCameraFollow(cmd.entities[0]);

		// Select units affected by that command
		let selection = [];
		if (cmd.entities)
			selection = cmd.entities;
		if (cmd.target)
			selection.push(cmd.target);

		// Allow gaia in selection when gathering
		g_Selection.reset();
		g_Selection.addList(selection, false, cmd.type == "gather");
	}
};

/**
 * Loads all known cheat commands.
 *
 * @returns {Object}
 */
function getCheatsData()
{
	let cheats = {};
	for (let fileName of getJSONFileList("simulation/data/cheats/"))
	{
		let currentCheat = Engine.ReadJSONFile("simulation/data/cheats/"+fileName+".json");
		if (!currentCheat)
			continue;
		if (Object.keys(cheats).indexOf(currentCheat.Name) !== -1)
			warn("Cheat name '" + currentCheat.Name + "' is already present");
		else
			cheats[currentCheat.Name] = currentCheat.Data;
	}
	return cheats;
}

/**
 * Reads userinput from the chat and sends a simulation command in case it is a known cheat.
 *
 * @returns {boolean} - True if a cheat was executed.
 */
function executeCheat(text)
{
	if (!controlsPlayer(Engine.GetPlayerID()) ||
	    !g_Players[Engine.GetPlayerID()].cheatsEnabled)
		return false;

	// Find the cheat code that is a prefix of the user input
	let cheatCode = Object.keys(g_Cheats).find(cheatCode => text.indexOf(cheatCode) == 0);
	if (!cheatCode)
		return false;

	let cheat = g_Cheats[cheatCode];

	let parameter = text.substr(cheatCode.length);
	if (cheat.isNumeric)
		parameter = +parameter;

	if (cheat.DefaultParameter && (isNaN(parameter) || parameter <= 0))
		parameter = cheat.DefaultParameter;

	Engine.PostNetworkCommand({
		"type": "cheat",
		"action": cheat.Action,
		"text": cheat.Type,
		"player": Engine.GetPlayerID(),
		"parameter": parameter,
		"templates": cheat.Templates,
		"selected": g_Selection.toList()
	});

	return true;
}

function findGuidForPlayerID(playerID)
{
	return Object.keys(g_PlayerAssignments).find(guid => g_PlayerAssignments[guid].player == playerID);
}

/**
 * Processes all pending notifications sent from the GUIInterface simulation component.
 */
function handleNotifications()
{
	for (let notification of Engine.GuiInterfaceCall("GetNotifications"))
	{
		if (!notification.players || !notification.type || !g_NotificationsTypes[notification.type])
		{
			error("Invalid GUI notification: " + uneval(notification));
			continue;
		}

		for (let player of notification.players)
			g_NotificationsTypes[notification.type](notification, player);
	}
}

/**
 * Updates the tutorial panel when a new goal.
 */
function updateTutorial(notification)
{
	// Show the tutorial panel if not yet done
	Engine.GetGUIObjectByName("tutorialPanel").hidden = false;

	if (notification.warning)
	{
		Engine.GetGUIObjectByName("tutorialWarning").caption = "[color=\"orange\"]" + notification.message + "[/color]";
		return;
	}

	let tutorialText = Engine.GetGUIObjectByName("tutorialText");
	let caption = tutorialText.caption.replace('[color=\"yellow\"]', '').replace('[/color]', '');
	if (caption)
		caption += "\n";
	tutorialText.caption = caption + "[color=\"yellow\"]" + notification.message + "[/color]";
	if (notification.readyButton)
	{
		Engine.GetGUIObjectByName("tutorialReady").hidden = false;
		if (notification.leave)
		{
			Engine.GetGUIObjectByName("tutorialWarning").caption = translate("Click to quit this tutorial.");
			Engine.GetGUIObjectByName("tutorialReady").caption = translate("Quit");
			Engine.GetGUIObjectByName("tutorialReady").onPress = leaveGame;
		}
		else
			Engine.GetGUIObjectByName("tutorialWarning").caption = translate("Click when ready.");
	}
	else
	{
		Engine.GetGUIObjectByName("tutorialWarning").caption = translate("Follow the instructions.");
		Engine.GetGUIObjectByName("tutorialReady").hidden = true;
	}
}

/**
 * Displays all active counters (messages showing the remaining time) for wonder-victory, ceasefire etc.
 */
function updateTimeNotifications()
{
	let notifications =  Engine.GuiInterfaceCall("GetTimeNotifications", g_ViewedPlayer);
	let notificationText = "";
	for (let n of notifications)
	{
		let message = n.message;
		if (n.translateMessage)
			message = translate(message);

		let parameters = n.parameters || {};
		if (n.translateParameters)
			translateObjectKeys(parameters, n.translateParameters);

		parameters.time = timeToString(n.endTime - g_SimState.timeElapsed);

		colorizePlayernameParameters(parameters);

		notificationText += sprintf(message, parameters) + "\n";
	}
	Engine.GetGUIObjectByName("notificationText").caption = notificationText;
}

/**
 * Process every CNetMessage (see NetMessage.h, NetMessages.h) sent by the CNetServer.
 * Saves the received object to mainlog.html.
 */
function handleNetMessages()
{
	while (true)
	{
		let msg = Engine.PollNetworkClient();
		if (!msg)
			return;

		log("Net message: " + uneval(msg));

		if (g_NetMessageTypes[msg.type])
			g_NetMessageTypes[msg.type](msg);
		else
			error("Unrecognised net message type '" + msg.type + "'");
	}
}

/**
 * @param {Object} message
 */
function handleNetStatusMessage(message)
{
	if (g_Disconnected)
		return;

	if (!g_StatusMessageTypes[message.status])
	{
		error("Unrecognised netstatus type '" + message.status + "'");
		return;
	}

	g_IsNetworkedActive = message.status == "active";

	let netStatus = Engine.GetGUIObjectByName("netStatus");
	let statusMessage = g_StatusMessageTypes[message.status](message);
	netStatus.caption = statusMessage;
	netStatus.hidden = !statusMessage;

	let loadingClientsText = Engine.GetGUIObjectByName("loadingClientsText");
	loadingClientsText.hidden = message.status != "waiting_for_players";

	if (message.status == "disconnected")
	{
		// Hide the pause overlay, and pause animations.
		Engine.GetGUIObjectByName("pauseOverlay").hidden = true;
		Engine.SetPaused(true, false);

		g_Disconnected = true;
		closeOpenDialogs();
	}
}

function handleClientsLoadingMessage(guids)
{
	let loadingClientsText = Engine.GetGUIObjectByName("loadingClientsText");
	loadingClientsText.caption = guids.map(guid => colorizePlayernameByGUID(guid)).join(translate(", "));
}

function onNetworkOutOfSync(msg)
{
	let txt = [
		sprintf(translate("Out-Of-Sync error on turn %(turn)s."), {
			"turn": msg.turn
		}),

		sprintf(translateWithContext("Out-Of-Sync", "Players: %(players)s"), {
			"players": msg.players.join(translate(", "))
		}),

		msg.hash == msg.expectedHash ?
			translateWithContext("Out-Of-Sync", "Your game state is identical to the hosts game state.") :
			translateWithContext("Out-Of-Sync", "Your game state differs from the hosts game state."),

		""
	];

	if (msg.turn > 1 && g_GameAttributes.settings.PlayerData.some(pData => pData && pData.AI))
		txt.push(translateWithContext("Out-Of-Sync", "Rejoining Multiplayer games with AIs is not supported yet!"));
	else
	{
		txt.push(
			translateWithContext("Out-Of-Sync", "Ensure all players use the same mods."),
			translateWithContext("Out-Of-Sync", 'Click on "Report Bugs" in the main menu to help fix this.'),
			sprintf(translateWithContext("Out-Of-Sync", "Replay saved to %(filepath)s"), {
				"filepath": escapeText(msg.path_replay)
			}),
			sprintf(translateWithContext("Out-Of-Sync", "Dumping current state to %(filepath)s"), {
				"filepath": escapeText(msg.path_oos_dump)
			})
		);
	}

	messageBox(
		600, 280,
		txt.join("\n"),
		translate("Out of Sync")
	);
}

function onReplayOutOfSync()
{
	messageBox(
		500, 140,
		translate("Out-Of-Sync error!") + "\n" +
			// Translation: This is shown if replay is out of sync
			translateWithContext("Out-Of-Sync", "The current game state is different from the original game state."),
		translate("Out of Sync")
	);
}

function handlePlayerAssignmentsMessage(message)
{
	for (let guid in g_PlayerAssignments)
		if (!message.newAssignments[guid])
			onClientLeave(guid);

	let joins = Object.keys(message.newAssignments).filter(guid => !g_PlayerAssignments[guid]);

	g_PlayerAssignments = message.newAssignments;

	joins.forEach(guid => {
		onClientJoin(guid);
	});

	updateGUIObjects();
	updateChatAddressees();
	sendLobbyPlayerlistUpdate();
}

function onClientJoin(guid)
{
	let playerID = g_PlayerAssignments[guid].player;

	if (g_Players[playerID])
	{
		g_Players[playerID].guid = guid;
		g_Players[playerID].name = g_PlayerAssignments[guid].name;
		g_Players[playerID].offline = false;
	}

	addChatMessage({
		"type": "connect",
		"guid": guid
	});
}

function onClientLeave(guid)
{
	setClientPauseState(guid, false);

	for (let id in g_Players)
		if (g_Players[id].guid == guid)
			g_Players[id].offline = true;

	addChatMessage({
		"type": "disconnect",
		"guid": guid
	});
}

function updateChatAddressees()
{
	// Remember previously selected item
	let chatAddressee = Engine.GetGUIObjectByName("chatAddressee");
	let selectedName = chatAddressee.list_data[chatAddressee.selected] || "";
	selectedName = selectedName.substr(0, 4) == "/msg" && selectedName.substr(5);

	let addressees = [
		{
			"label": translateWithContext("chat addressee", "Everyone"),
			"cmd": ""
		}
	];

	if (!g_IsObserver)
	{
		addressees.push({
			"label": translateWithContext("chat addressee", "Allies"),
			"cmd": "/allies"
		});
		addressees.push({
			"label": translateWithContext("chat addressee", "Enemies"),
			"cmd": "/enemies"
		});
	}

	addressees.push({
		"label": translateWithContext("chat addressee", "Observers"),
		"cmd": "/observers"
	});

	// Add playernames for private messages
	let guids = sortGUIDsByPlayerID();
	for (let guid of guids)
	{
		if (guid == Engine.GetPlayerGUID())
			continue;

		let playerID = g_PlayerAssignments[guid].player;

		// Don't provide option for PM from observer to player
		if (g_IsObserver && !isPlayerObserver(playerID))
			continue;

		let colorBox = isPlayerObserver(playerID) ? "" : colorizePlayernameHelper("■", playerID) + " ";

		addressees.push({
			"cmd": "/msg " + g_PlayerAssignments[guid].name,
			"label": colorBox + g_PlayerAssignments[guid].name
		});
	}

	// Select mock item if the selected addressee went offline
	if (selectedName && guids.every(guid => g_PlayerAssignments[guid].name != selectedName))
		addressees.push({
			"cmd": "/msg " + selectedName,
			"label": sprintf(translate("\\[OFFLINE] %(player)s"), { "player": selectedName })
		});

	let oldChatAddressee = chatAddressee.list_data[chatAddressee.selected];
	chatAddressee.list = addressees.map(adressee => adressee.label);
	chatAddressee.list_data = addressees.map(adressee => adressee.cmd);
	chatAddressee.selected = Math.max(0, chatAddressee.list_data.indexOf(oldChatAddressee));
}

/**
 * Send text as chat. Don't look for commands.
 *
 * @param {string} text
 */
function submitChatDirectly(text)
{
	if (!text.length)
		return;

	if (g_IsNetworked)
		Engine.SendNetworkChat(text);
	else
		addChatMessage({ "type": "message", "guid": "local", "text": text });
}

/**
 * Loads the text from the GUI window, checks if it is a local command
 * or cheat and executes it. Otherwise sends it as chat.
 */
function submitChatInput()
{
	let text = Engine.GetGUIObjectByName("chatInput").caption;

	closeChat();

	if (!text.length)
		return;

	if (executeNetworkCommand(text))
		return;

	if (executeCheat(text))
		return;

	let chatAddressee = Engine.GetGUIObjectByName("chatAddressee");
	if (chatAddressee.selected > 0 && (text.indexOf("/") != 0 || text.indexOf("/me ") == 0))
		text = chatAddressee.list_data[chatAddressee.selected] + " " + text;

	let selectedChat = chatAddressee.list_data[chatAddressee.selected];
	if (selectedChat.startsWith("/msg"))
		g_LastChatAddressee = selectedChat;

	submitChatDirectly(text);
}

/**
 * Displays the prepared chatmessage.
 *
 * @param msg {Object}
 */
function addChatMessage(msg)
{
	if (!g_FormatChatMessage[msg.type])
		return;

	let formatted = g_FormatChatMessage[msg.type](msg);
	if (!formatted)
		return;

	// Update chat overlay
	g_ChatMessages.push(formatted);
	g_ChatTimers.push(setTimeout(removeOldChatMessage, g_ChatTimeout * 1000));

	if (g_ChatMessages.length > g_ChatLines)
		removeOldChatMessage();
	else
		Engine.GetGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");

	// Save to chat history
	let historical = {
		"txt": formatted,
		"timePrefix": sprintf(translate("\\[%(time)s]"), {
			"time": Engine.FormatMillisecondsIntoDateStringLocal(new Date().getTime(), translate("HH:mm"))
		}),
		"filter": {}
	};

	// Apply the filters now before diplomacies or playerstates change
	let senderID = msg.guid && g_PlayerAssignments[msg.guid] ? g_PlayerAssignments[msg.guid].player : 0;
	for (let filter of g_ChatHistoryFilters)
		historical.filter[filter.key] = filter.filter(msg, senderID);

	g_ChatHistory.push(historical);
	updateChatHistory();
}

/**
 * Called when the timer has run out for the oldest chatmessage or when the message limit is reached.
 */
function removeOldChatMessage()
{
	clearTimeout(g_ChatTimers[0]);
	g_ChatTimers.shift();
	g_ChatMessages.shift();
	Engine.GetGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}

/**
 * This function is used for AIs, whose names don't exist in g_PlayerAssignments.
 */
function colorizePlayernameByID(playerID)
{
	let username = g_Players[playerID] && escapeText(g_Players[playerID].name);
	return colorizePlayernameHelper(username, playerID);
}

function colorizePlayernameByGUID(guid)
{
	let username = g_PlayerAssignments[guid] ? g_PlayerAssignments[guid].name : "";
	let playerID = g_PlayerAssignments[guid] ? g_PlayerAssignments[guid].player : -1;
	return colorizePlayernameHelper(username, playerID);
}

function colorizePlayernameHelper(username, playerID)
{
	let playerColor = playerID > -1 ? rgbToGuiColor(g_Players[playerID].color) : "white";
	return '[color="' + playerColor + '"]' + (username || translate("Unknown Player")) + "[/color]";
}

/**
 * Insert the colorized playername to chat messages sent by the AI and time notifications.
 */
function colorizePlayernameParameters(parameters)
{
	for (let param in parameters)
		if (param.startsWith("_player_"))
			parameters[param] = colorizePlayernameByID(parameters[param]);
}

function formatDefeatMessage(msg)
{
	return sprintf(
		msg.resign ?
			translate("%(player)s has resigned.") :
			translate("%(player)s has been defeated."),
		{ "player": colorizePlayernameByID(msg.player) }
	);
}

function formatWinMessage(msg)
{
	return sprintf(translate("%(player)s has won."), {
		"player": colorizePlayernameByID(msg.player)
	});
}

function formatDiplomacyMessage(msg)
{
	let messageType;

	if (g_IsObserver)
		messageType = "observer";
	else if (Engine.GetPlayerID() == msg.sourcePlayer)
		messageType = "active";
	else if (Engine.GetPlayerID() == msg.targetPlayer)
		messageType = "passive";
	else
		return "";

	return sprintf(g_DiplomacyMessages[messageType][msg.status], {
		"player": colorizePlayernameByID(messageType == "active" ? msg.targetPlayer : msg.sourcePlayer),
		"player2": colorizePlayernameByID(messageType == "active" ? msg.sourcePlayer : msg.targetPlayer)
	});
}

/**
 * Optionally show all tributes sent in observer mode and tributes sent between allied players.
 * Otherwise, only show tributes sent directly to us, and tributes that we send.
 */
function formatTributeMessage(msg)
{
	let message = "";
	if (msg.targetPlayer == Engine.GetPlayerID())
		message = translate("%(player)s has sent you %(amounts)s.");
	else if (msg.sourcePlayer == Engine.GetPlayerID())
		message = translate("You have sent %(player2)s %(amounts)s.");
	else if (Engine.ConfigDB_GetValue("user", "gui.session.notifications.tribute") == "true" &&
	        (g_IsObserver || g_GameAttributes.settings.LockTeams &&
	           g_Players[msg.sourcePlayer].isMutualAlly[Engine.GetPlayerID()] &&
	           g_Players[msg.targetPlayer].isMutualAlly[Engine.GetPlayerID()]))
			message = translate("%(player)s has sent %(player2)s %(amounts)s.");

	return sprintf(message, {
		"player": colorizePlayernameByID(msg.sourcePlayer),
		"player2": colorizePlayernameByID(msg.targetPlayer),
		"amounts": getLocalizedResourceAmounts(msg.amounts)
	});
}

function formatBarterMessage(msg)
{
	if (!g_IsObserver || Engine.ConfigDB_GetValue("user", "gui.session.notifications.barter") != "true")
		return "";

	let amountsSold = {};
	amountsSold[msg.resourceSold] = msg.amountsSold;

	let amountsBought = {};
	amountsBought[msg.resourceBought] = msg.amountsBought;

	return sprintf(translate("%(player)s bartered %(amountsBought)s for %(amountsSold)s."), {
		"player": colorizePlayernameByID(msg.player),
		"amountsBought": getLocalizedResourceAmounts(amountsBought),
		"amountsSold": getLocalizedResourceAmounts(amountsSold)
	});
}

function formatAttackMessage(msg)
{
	if (msg.player != g_ViewedPlayer)
		return "";

	let message = msg.targetIsDomesticAnimal ?
			translate("Your livestock has been attacked by %(attacker)s!") :
			translate("You have been attacked by %(attacker)s!");

	return sprintf(message, {
		"attacker": colorizePlayernameByID(msg.attacker)
	});
}

function formatPhaseMessage(msg)
{
	let notifyPhase = Engine.ConfigDB_GetValue("user", "gui.session.notifications.phase");
	if (notifyPhase == "none" || msg.player != g_ViewedPlayer && !g_IsObserver && !g_Players[msg.player].isMutualAlly[g_ViewedPlayer])
		return "";

	let message = "";
	if (notifyPhase == "all")
	{
		if (msg.phaseState == "started")
			message = translate("%(player)s is advancing to the %(phaseName)s.");
		else if (msg.phaseState == "aborted")
			message = translate("The %(phaseName)s of %(player)s has been aborted.");
	}
	if (msg.phaseState == "completed")
		message = translate("%(player)s has reached the %(phaseName)s.");

	return sprintf(message, {
		"player": colorizePlayernameByID(msg.player),
		"phaseName": getEntityNames(GetTechnologyData(msg.phaseName, g_Players[msg.player].civ))
	});
}

function formatChatCommand(msg)
{
	if (!msg.text)
		return "";

	let isMe = msg.text.indexOf("/me ") == 0;
	if (!isMe && !parseChatAddressee(msg))
		return "";

	isMe = msg.text.indexOf("/me ") == 0;
	if (isMe)
		msg.text = msg.text.substr("/me ".length);

	// Translate or escape text
	if (!msg.text)
		return "";

	if (msg.translate)
	{
		msg.text = translate(msg.text);
		if (msg.translateParameters)
		{
			let parameters = msg.parameters || {};
			translateObjectKeys(parameters, msg.translateParameters);
			msg.text = sprintf(msg.text, parameters);
		}
	}
	else
	{
		msg.text = escapeText(msg.text);

		let userName = g_PlayerAssignments[Engine.GetPlayerGUID()].name;

		if (userName != g_PlayerAssignments[msg.guid].name)
			notifyUser(userName, msg.text);
	}

	// GUID for players, playerID for AIs
	let coloredUsername = msg.guid != -1 ? colorizePlayernameByGUID(msg.guid) : colorizePlayernameByID(msg.player);

	return sprintf(g_ChatCommands[isMe ? "me" : "regular"][msg.context ? "context" : "no-context"], {
		"message": msg.text,
		"context": msg.context || undefined,
		"user": coloredUsername,
		"userTag": sprintf(translate("<%(user)s>"), { "user": coloredUsername })
	});
}

/**
 * Checks if the current user is an addressee of the chatmessage sent by another player.
 * Sets the context and potentially addresseeGUID of that message.
 * Returns true if the message should be displayed.
 *
 * @param {Object} msg
 */
function parseChatAddressee(msg)
{
	if (msg.text[0] != '/')
		return true;

	// Split addressee command and message-text
	msg.cmd = msg.text.split(/\s/)[0];
	msg.text = msg.text.substr(msg.cmd.length + 1);

	// GUID is "local" in singleplayer, some string in multiplayer.
	// Chat messages sent by the simulation (AI) come with the playerID.
	let senderID = msg.player ? msg.player : (g_PlayerAssignments[msg.guid] || msg).player;

	let isSender = msg.guid ?
		msg.guid == Engine.GetPlayerGUID() :
		senderID == Engine.GetPlayerID();

	// Parse private message
	let isPM = msg.cmd == "/msg";
	let addresseeGUID;
	let addresseeIndex;
	if (isPM)
	{
		addresseeGUID = matchUsername(msg.text);
		let addressee = g_PlayerAssignments[addresseeGUID];
		if (!addressee)
		{
			if (isSender)
				warn("Couldn't match username: " + msg.text);
			return false;
		}

		// Prohibit PM if addressee and sender are identical
		if (isSender && addresseeGUID == Engine.GetPlayerGUID())
			return false;

		msg.text = msg.text.substr(addressee.name.length + 1);
		addresseeIndex = addressee.player;
	}

	// Set context string
	if (!g_ChatAddresseeContext[msg.cmd])
	{
		if (isSender)
			warn("Unknown chat command: " + msg.cmd);
		return false;
	}
	msg.context = g_ChatAddresseeContext[msg.cmd];

	// For observers only permit public- and observer-chat and PM to observers
	if (isPlayerObserver(senderID) &&
	    (isPM && !isPlayerObserver(addresseeIndex) || !isPM && msg.cmd != "/observers"))
		return false;
	let visible = isSender || g_IsChatAddressee[msg.cmd](senderID, addresseeGUID);
	msg.isVisiblePM = isPM && visible;

	return visible;
}

/**
 * Returns the guid of the user with the longest name that is a prefix of the given string.
 */
function matchUsername(text)
{
	if (!text)
		return "";

	let match = "";
	let playerGUID = "";
	for (let guid in g_PlayerAssignments)
	{
		let pName = g_PlayerAssignments[guid].name;
		if (text.indexOf(pName + " ") == 0 && pName.length > match.length)
		{
			match = pName;
			playerGUID = guid;
		}
	}
	return playerGUID;
}

/**
 * Custom dialog response handling, usable by trigger maps.
 */
function sendDialogAnswer(guiObject, dialogName)
{
	Engine.GetGUIObjectByName(dialogName+"-dialog").hidden = true;

	Engine.PostNetworkCommand({
		"type": "dialog-answer",
		"dialog": dialogName,
		"answer": guiObject.name.split("-").pop(),
	});

	resumeGame();
}

/**
 * Custom dialog opening, usable by trigger maps.
 */
function openDialog(dialogName, data, player)
{
	let dialog = Engine.GetGUIObjectByName(dialogName + "-dialog");
	if (!dialog)
	{
		warn("messages.js: Unknow dialog with name " + dialogName);
		return;
	}
	dialog.hidden = false;

	for (let objName in data)
	{
		let obj = Engine.GetGUIObjectByName(dialogName + "-dialog-" + objName);
		if (!obj)
		{
			warn("messages.js: Key '" + objName + "' not found in '" + dialogName + "' dialog.");
			continue;
		}

		for (let key in data[objName])
		{
			let n = data[objName][key];
			if (typeof n == "object" && n.message)
			{
				let message = n.message;
				if (n.translateMessage)
					message = translate(message);
				let parameters = n.parameters || {};
				if (n.translateParameters)
					translateObjectKeys(parameters, n.translateParameters);
				obj[key] = sprintf(message, parameters);
			}
			else
				obj[key] = n;
		}
	}

	pauseGame();
}
