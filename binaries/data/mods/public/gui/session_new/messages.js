// Chat data
const CHAT_TIMEOUT = 45000;
const MAX_NUM_CHAT_LINES = 20;
var chatMessages = [];
var chatTimers = [];

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
			// TODO: we need to give players some way to exit
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
		addChatMessage({ "type": "message", "username": message.username, "text": message.text });
		break;
	default:
		error("Unrecognised net message type "+message.type);
	}
}

function submitChatInput()
{
	toggleChatWindow();
	
	var input = getGUIObjectByName("chatInput");
	var text = input.caption;
	if (text.length)
	{
		if (g_IsNetworked)
			Engine.SendNetworkChat(text);
		else
			addChatMessage({ "type": "message", "username": g_Players[1].name, "text": text });

		input.caption = "";

		// Remove focus
		input.blur();
	}
}

function addChatMessage(msg)
{
	// TODO: we ought to escape all values before displaying them,
	// to prevent people inserting colours and newlines etc

	var playerColor = getColorByPlayerName(msg.username);
	var formatted;

	switch (msg.type)
	{
	case "disconnect":
		formatted = '<[font=\"serif-stroke-14\"][color="' + playerColor + '"]' + msg.username + '[/color][/font]> has left';
		break;

	case "message":
		formatted = '<[font=\"serif-stroke-14\"][color="' + playerColor + '"]' + msg.username + '[/color][/font]> ' + msg.text;
		break;

	default:
		error("Invalid chat message '" + uneval(msg) + "'");
		return;
	}

	var timerExpiredFunction = function () { removeOldChatMessages(); }
	
	chatMessages.push(formatted);
	chatTimers.push(setTimeout(timerExpiredFunction, CHAT_TIMEOUT));

	if (chatMessages.length < MAX_NUM_CHAT_LINES)
		getGUIObjectByName("chatText").caption = chatMessages.join("\n");
	else
		removeOldChatMessages();
}

function removeOldChatMessages()
{
	clearTimeout(chatTimers[0]); // The timer only needs to be cleared when new messages bump old messages off
	chatTimers.shift();
	chatMessages.shift();
	getGUIObjectByName("chatText").caption = chatMessages.join("\n");
}

function getColorByPlayerName(playerName)
{
	for (var i = 0; i < g_Players.length; i++)
		if (playerName == g_Players[i].name)
			return  g_Players[i].color.r + " " + g_Players[i].color.g + " " + g_Players[i].color.b;
			
	return "255 255 255";
}
