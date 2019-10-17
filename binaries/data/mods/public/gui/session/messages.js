/**
 * All known cheat commands.
 */
const g_Cheats = getCheatsData();

/**
 * All tutorial messages received so far.
 */
var g_TutorialMessages = [];

/**
 * GUI tags applied to the most recent tutorial message.
 */
var g_TutorialNewMessageTags = { "color": "yellow" };

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
		g_PauseControl.setClientPauseState(msg.guid, msg.pause);
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

/**
 * Show a label and grey overlay or hide both on connection change.
 */
var g_StatusMessageTypes = {
	"authenticated": msg => translate("Connection to the server has been authenticated."),
	"connected": msg => translate("Connected to the server."),
	"disconnected": msg => translate("Connection to the server has been lost.") + "\n" +
		getDisconnectReason(msg.reason, true),
	"waiting_for_players": msg => translate("Waiting for players to connect:"),
	"join_syncing": msg => translate("Synchronizing gameplay with other players…"),
	"active": msg => ""
};

var g_PlayerStateMessages = {
	"won": translate("You have won!"),
	"defeated": translate("You have been defeated!")
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
		playersFinished(notification.allies, notification.message, false);
	},
	"won": function(notification, player)
	{
		playersFinished(notification.allies, notification.message, true);
	},
	"diplomacy": function(notification, player)
	{
		updatePlayerData();
		g_DiplomacyColors.onDiplomacyChange();

		addChatMessage({
			"type": "diplomacy",
			"sourcePlayer": player,
			"targetPlayer": notification.targetPlayer,
			"status": notification.status
		});
	},
	"ceasefire-ended": function(notification, player)
	{
		updatePlayerData();
		g_DiplomacyColors.OnCeasefireEnded();
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
		g_DiplomacyDialog.onSpyResponse(notification, player);

		if (notification.entity && g_ViewedPlayer == player)
		{
			g_DiplomacyDialog.close();
			setCameraFollow(notification.entity);
		}
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

		// Ignore rallypoint commands of trained animals
		let entState = cmd.entities && cmd.entities[0] && GetEntityState(cmd.entities[0]);
		if (g_ViewedPlayer != 0 &&
		    entState && entState.identity && entState.identity.classes &&
		    entState.identity.classes.indexOf("Animal") != -1)
			return;

		// Focus the building to construct
		if (cmd.type == "repair")
		{
			let targetState = GetEntityState(cmd.target);
			if (targetState)
				Engine.CameraMoveTo(targetState.position.x, targetState.position.z);
		}
		else if (cmd.type == "delete-entities" && notification.position)
			Engine.CameraMoveTo(notification.position.x, notification.position.y);
		// Focus commanded entities, but don't lose previous focus when training units
		else if (cmd.type != "train" && cmd.type != "research" && entState)
			setCameraFollow(cmd.entities[0]);

		if (["walk", "attack-walk", "patrol"].indexOf(cmd.type) != -1)
			DrawTargetMarker(cmd);

		// Select units affected by that command
		let selection = [];
		if (cmd.entities)
			selection = cmd.entities;
		if (cmd.target)
			selection.push(cmd.target);

		// Allow gaia in selection when gathering
		g_Selection.reset();
		g_Selection.addList(selection, false, cmd.type == "gather");
	},
	"play-tracks": function(notification, player)
	{
		if (notification.lock)
		{
			global.music.storeTracks(notification.tracks.map(track => ({ "Type": "custom", "File": track })));
			global.music.setState(global.music.states.CUSTOM);
		}

		global.music.setLocked(notification.lock);
	}
};

/**
 * Loads all known cheat commands.
 */
function getCheatsData()
{
	let cheats = {};
	for (let fileName of Engine.ListDirectoryFiles("simulation/data/cheats/", "*.json", false))
	{
		let currentCheat = Engine.ReadJSONFile(fileName);
		if (cheats[currentCheat.Name])
			warn("Cheat name '" + currentCheat.Name + "' is already present");
		else
			cheats[currentCheat.Name] = currentCheat.Data;
	}
	return deepfreeze(cheats);
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
	let cheatCode = Object.keys(g_Cheats).find(code => text.indexOf(code) == 0);
	if (!cheatCode)
		return false;

	let cheat = g_Cheats[cheatCode];

	let parameter = text.substr(cheatCode.length + 1);
	if (cheat.isNumeric)
		parameter = +parameter;

	if (cheat.DefaultParameter && !parameter)
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
		Engine.GetGUIObjectByName("tutorialWarning").caption = coloredText(translate(notification.warning), "orange");
		return;
	}

	let notificationText =
		notification.instructions.reduce((instructions, item) =>
			instructions + (typeof item == "string" ? translate(item) : colorizeHotkey(translate(item.text), item.hotkey)),
			"");

	Engine.GetGUIObjectByName("tutorialText").caption = g_TutorialMessages.concat(setStringTags(notificationText, g_TutorialNewMessageTags)).join("\n");
	g_TutorialMessages.push(notificationText);

	if (notification.readyButton)
	{
		Engine.GetGUIObjectByName("tutorialReady").hidden = false;
		if (notification.leave)
		{
			Engine.GetGUIObjectByName("tutorialWarning").caption = translate("Click to quit this tutorial.");
			Engine.GetGUIObjectByName("tutorialReady").caption = translate("Quit");
			Engine.GetGUIObjectByName("tutorialReady").onPress = endGame;
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
	let notifications = Engine.GuiInterfaceCall("GetTimeNotifications", g_ViewedPlayer);
	let notificationText = "";
	for (let n of notifications)
	{
		let message = n.message;
		if (n.translateMessage)
			message = translate(message);

		let parameters = n.parameters || {};
		if (n.translateParameters)
			translateObjectKeys(parameters, n.translateParameters);

		parameters.time = timeToString(n.endTime - GetSimState().timeElapsed);

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
		updateCinemaPath();
		closeOpenDialogs();
	}
}

function handleClientsLoadingMessage(guids)
{
	let loadingClientsText = Engine.GetGUIObjectByName("loadingClientsText");
	loadingClientsText.caption = guids.map(guid => colorizePlayernameByGUID(guid)).join(translateWithContext("Separator for a list of client loading messages", ", "));
}

function onNetworkOutOfSync(msg)
{
	let txt = [
		sprintf(translate("Out-Of-Sync error on turn %(turn)s."), {
			"turn": msg.turn
		}),

		sprintf(translateWithContext("Out-Of-Sync", "Players: %(players)s"), {
			"players": msg.players.join(translateWithContext("Separator for a list of players", ", "))
		}),

		msg.hash == msg.expectedHash ?
			translateWithContext("Out-Of-Sync", "Your game state is identical to the hosts game state.") :
			translateWithContext("Out-Of-Sync", "Your game state differs from the hosts game state."),

		""
	];

	if (msg.turn > 1 && g_GameAttributes.settings.PlayerData.some(pData => pData && pData.AI))
		txt.push(translateWithContext("Out-Of-Sync", "Rejoining Multiplayer games with AIs is not supported yet!"));
	else
		txt.push(
			translateWithContext("Out-Of-Sync", "Ensure all players use the same mods."),
			translateWithContext("Out-Of-Sync", 'Click on "Report a Bug" in the main menu to help fix this.'),
			sprintf(translateWithContext("Out-Of-Sync", "Replay saved to %(filepath)s"), {
				"filepath": escapeText(msg.path_replay)
			}),
			sprintf(translateWithContext("Out-Of-Sync", "Dumping current state to %(filepath)s"), {
				"filepath": escapeText(msg.path_oos_dump)
			})
		);

	messageBox(
		600, 280,
		txt.join("\n"),
		translate("Out of Sync")
	);
}

function onReplayOutOfSync(turn, hash, expectedHash)
{
	messageBox(
		500, 140,
		sprintf(translate("Out-Of-Sync error on turn %(turn)s."), {
			"turn": turn
		}) + "\n" +
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
	g_Chat.onUpdatePlayers();
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
	g_PauseControl.setClientPauseState(guid, false);

	for (let id in g_Players)
		if (g_Players[id].guid == guid)
			g_Players[id].offline = true;

	addChatMessage({
		"type": "disconnect",
		"guid": guid
	});
}

function addChatMessage(msg)
{
	g_Chat.ChatMessageHandler.handleMessage(msg);
}

function clearChatMessages()
{
	g_Chat.ChatOverlay.clearChatMessages();
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
	let playerColor = playerID > -1 ? g_DiplomacyColors.getPlayerColor(playerID) : "white";
	return coloredText(username || translate("Unknown Player"), playerColor);
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

/**
 * Custom dialog response handling, usable by trigger maps.
 */
function sendDialogAnswer(guiObject, dialogName)
{
	Engine.GetGUIObjectByName(dialogName + "-dialog").hidden = true;

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

	g_PauseControl.implicitPause();
}
