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

	if (notification && notification.player ==  Engine.GetPlayerID())
	{
		var timerExpiredFunction = function () { removeOldNotifications(); }

		notifications.push(notification);
		notificationsTimers.push(setTimeout(timerExpiredFunction, NOTIFICATION_TIMEOUT));

		if (notifications.length > MAX_NUM_NOTIFICATION_LINES)
			removeOldNotifications();
		else
			displayNotifications();
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

//Messages
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
			obj.caption = "Waiting for other players to connect";
			obj.hidden = false;
			break;
		case "active":
			obj.caption = "";
			obj.hidden = true;
			break;
		case "disconnected":
			obj.caption = "Connection to the server has been lost";
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
				var obj = getGUIObjectByName("netStatus");
				obj.caption = g_PlayerAssignments[host].name + " has left\n\nConnection to the server has been lost";
				obj.hidden = false;
				getGUIObjectByName("disconnectedExitButton").hidden = false;
			}
		}
		break;
	case "chat":
		addChatMessage({ "type": "message", "guid": message.guid, "text": message.text });
		break;
	default:
		error("Unrecognised net message type "+message.type);
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
			addChatMessage({ "type": "message", "guid": 1, "text": text });

		input.caption = ""; // Clear chat input
		input.blur(); // Remove focus
	}
	
	toggleChatWindow();
}

function addChatMessage(msg)
{
	// TODO: we ought to escape all values before displaying them,
	// to prevent people inserting colours and newlines etc

	var n = msg.guid;
	var username = g_Players[n].name;
	var playerColor = g_Players[n].color.r + " " + g_Players[n].color.g + " " + g_Players[n].color.b;

	var formatted;

	switch (msg.type)
	{
	/*
	case "disconnect":
		formatted = "<[color=\"" + playerColor + "\"]" + username + "[/color]> has left";
		break;
	*/
	case "message":
		console.write("<" + username + "> " + msg.text);
		formatted = "<[color=\"" + playerColor + "\"]" + username + "[/color]> " + msg.text;
		break;
	default:
		error("Invalid chat message '" + uneval(msg) + "'");
		return;
	}

	var timerExpiredFunction = function () { removeOldChatMessages(); }
	
	chatMessages.push(formatted);
	chatTimers.push(setTimeout(timerExpiredFunction, CHAT_TIMEOUT));

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
