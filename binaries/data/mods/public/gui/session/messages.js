// Chat data
const CHAT_TIMEOUT = 30000;
const MAX_NUM_CHAT_LINES = 20;
var chatMessages = [];
var chatTimers = [];

// Notification Data
const NOTIFICATION_TIMEOUT = 10000;
const MAX_NUM_NOTIFICATION_LINES = 3;
var notifications = [];
var notificationsTimers = [];

// Notifications
function handleNotifications()
{
	var notification = Engine.GuiInterfaceCall("GetNextNotification");

	if (!notification)
		return;

	// Handle chat notifications specially
	if (notification.type == "chat")
	{
		addChatMessage({
			"type": "message",
			"guid": findGuidForPlayerID(g_PlayerAssignments, notification.player),
			"text": notification.message
		});
	}
	else
	{
		// Only display notifications directed to this player
		if (notification.player == Engine.GetPlayerID())
		{
			notifications.push(notification);
			notificationsTimers.push(setTimeout(removeOldNotifications, NOTIFICATION_TIMEOUT));

			if (notifications.length > MAX_NUM_NOTIFICATION_LINES)
				removeOldNotifications();
			else
				displayNotifications();
		}
	}
}

function removeOldNotifications()
{
	clearTimeout(notificationsTimers[0]); // The timer only needs to be cleared when new notifications bump old notifications off
	notificationsTimers.shift();
	notifications.shift();
	displayNotifications();
}

function displayNotifications()
{
	var messages = [];
	for each (var n in notifications)
		messages.push(n.message);
	getGUIObjectByName("notificationText").caption = messages.join("\n");
}

// Messages
function handleNetMessage(message)
{
	log("Net message: "+uneval(message));

	switch (message.type)
	{
	case "netstatus":
		var obj = getGUIObjectByName("netStatus");
		switch (message.status)
		{
		case "waiting_for_players":
			obj.caption = "Waiting for other players to connect...";
			obj.hidden = false;
			break;
		case "active":
			obj.caption = "";
			obj.hidden = true;
			break;
		case "connected":
			obj.caption = "Connected to the server.";
			obj.hidden = false;
		case "authenticated":
			obj.caption = "Connection to the server has been authenticated.";
			obj.hidden = false;
		case "disconnected":
			obj.caption = "Connection to the server has been lost.\n\nThe game has ended.";
			obj.hidden = false;
			getGUIObjectByName("disconnectedExitButton").hidden = false;
			break;
		default:
			error("Unrecognised netstatus type "+message.status);
			break;
		}
		break;

	case "players":
		// Find and report all leavings
		for (var host in g_PlayerAssignments)
		{
			if (! message.hosts[host])
			{
				// Tell the user about the disconnection
				addChatMessage({ "type": "disconnect", "guid": host });

				// Update the cached player data, so we can display the disconnection status
				updatePlayerDataRemove(g_Players, host);
			}
		}

		// Find and report all joinings
		for (var host in message.hosts)
		{
			if (! g_PlayerAssignments[host])
			{
				// Update the cached player data, so we can display the correct name
				updatePlayerDataAdd(g_Players, host, message.hosts[host]);
			}
		}

		g_PlayerAssignments = message.hosts;

		break;

	case "chat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text });
		break;

	// To prevent errors, ignore these message types that occur during autostart
	case "gamesetup":
	case "start":
		break;

	default:
		error("Unrecognised net message type "+message.type);
	}
}

function submitChatDirectly(text)
{
	if (text.length)
	{
		if (g_IsNetworked)
			Engine.SendNetworkChat(text);
		else
			addChatMessage({ "type": "message", "guid": "local", "text": text });
	}
}

function submitChatInput()
{
	var input = getGUIObjectByName("chatInput");
	var text = input.caption;
	if (text.length)
	{
		if (g_IsNetworked)
			Engine.SendNetworkChat(text);
		else
			addChatMessage({ "type": "message", "guid": "local", "text": text });

		input.caption = ""; // Clear chat input
	}

	input.blur(); // Remove focus

	toggleChatWindow();
}

function addChatMessage(msg)
{
	var playerColor, username;
	if (g_PlayerAssignments[msg.guid])
	{
		var n = g_PlayerAssignments[msg.guid].player;
		playerColor = g_Players[n].color.r + " " + g_Players[n].color.g + " " + g_Players[n].color.b;
		username = escapeText(g_PlayerAssignments[msg.guid].name);
	}
	else
	{
		playerColor = "255 255 255";
		username = "Unknown player";
	}

	var message = escapeText(msg.text);

	var formatted;

	switch (msg.type)
	{
	case "disconnect":
		formatted = "[color=\"" + playerColor + "\"]" + username + "[/color] has left the game.";
		break;
	case "message":
		console.write("<" + username + "> " + message);
		formatted = "<[color=\"" + playerColor + "\"]" + username + "[/color]> " + message;
		break;
	default:
		error("Invalid chat message '" + uneval(msg) + "'");
		return;
	}

	chatMessages.push(formatted);
	chatTimers.push(setTimeout(removeOldChatMessages, CHAT_TIMEOUT));

	if (chatMessages.length > MAX_NUM_CHAT_LINES)
		removeOldChatMessages();
	else
		getGUIObjectByName("chatText").caption = chatMessages.join("\n");
}

function removeOldChatMessages()
{
	clearTimeout(chatTimers[0]); // The timer only needs to be cleared when new messages bump old messages off
	chatTimers.shift();
	chatMessages.shift();
	getGUIObjectByName("chatText").caption = chatMessages.join("\n");
}
