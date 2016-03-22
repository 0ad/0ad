/**
 * Number of milliseconds to display network warnings.
 */
const g_NetworkWarningTimeout = 3000;

/**
 * Currently displayed network warnings. At most one message per user.
 */
var g_NetworkWarnings = {};

/**
 * Message-types to be displayed.
 */
var g_NetworkWarningTexts = {

	"server-timeout": (msg, username) =>
		sprintf(translate("Losing connection to server (%(seconds)ss)"), {
			"seconds": Math.ceil(msg.lastReceivedTime / 1000)
		}),

	"client-timeout": (msg, username) =>
		sprintf(translate("%(player)s losing connection (%(seconds)ss)"), {
			"player": username,
			"seconds": Math.ceil(msg.lastReceivedTime / 1000)
		}),

	"server-latency": (msg, username) =>
		sprintf(translate("Bad connection to server (%(milliseconds)sms)"), {
			"milliseconds": msg.meanRTT
		}),

	"client-latency": (msg, username) =>
		sprintf(translate("Bad connection to %(player)s (%(milliseconds)sms)"), {
			"player": username,
			"milliseconds": msg.meanRTT
		})
};

var g_NetworkCommands = {
	"/kick": argument => kickPlayer(argument, false),
	"/ban": argument => kickPlayer(argument, true),
	"/list": argument => addChatMessage({ "type": "clientlist" }),
	"/clear": argument => clearChatMessages()
};

/**
 * Must be kept in sync with source/network/NetHost.h
 */
function getDisconnectReason(id)
{
	switch (id)
	{
	case 0: return translate("Unknown reason");
	case 1: return translate("The host has ended the game");
	case 2: return translate("Incorrect network protocol version");
	case 3: return translate("Game is loading, please try later");
	case 4: return translate("Game has already started, no observers allowed");
	case 5: return translate("You have been kicked");
	case 6: return translate("You have been banned");
	case 7: return translate("Playername in use. If you were disconnected, retry in few seconds");
	case 8: return translate("Server full");
	default:
		warn("Unknown disconnect-reason ID received: " + id);
		return sprintf(translate("\\[Invalid value %(id)s]"), { "id": id });
	}
}

/**
 * Show the disconnect reason in a message box.
 *
 * @param {number} reason
 */
function reportDisconnect(reason)
{
	// Translation: States the reason why the client disconnected from the server.
	let reasonText = sprintf(translate("Reason: %(reason)s."), { "reason": getDisconnectReason(reason) });
	messageBox(400, 200, translate("Lost connection to the server.") + "\n\n" + reasonText, translate("Disconnected"), 2);
}

function kickPlayer(username, ban)
{
	if (!Engine.KickPlayer(username, ban))
		addChatMessage({
			"type": "system",
			"text": sprintf(ban ? translate("Could not ban %(name)s.") : translate("Could not kick %(name)s."), {
				"name": username
			})
		});
}

/**
 * Sort GUIDs of connected users sorted by playerindex, observers last.
 * Requires g_PlayerAssignments.
 */
function sortGUIDsByPlayerID()
{
	return Object.keys(g_PlayerAssignments).sort((guidA, guidB) => {

		let playerIdA = g_PlayerAssignments[guidA].player;
		let playerIdB = g_PlayerAssignments[guidB].player;

		if (playerIdA == -1) return +1;
		if (playerIdB == -1) return -1;

		return playerIdA - playerIdB;
	});
}

/**
 * Get a colorized list of usernames sorted by player slot, observers last.
 * Requires g_PlayerAssignments and colorizePlayernameByGUID.
 *
 * @returns {string}
 */
function getUsernameList()
{
	let usernames = sortGUIDsByPlayerID().map(guid => colorizePlayernameByGUID(guid));

	return sprintf(translate("Users: %(users)s"),
		// Translation: This comma is used for separating first to penultimate elements in an enumeration.
		{ "users": usernames.join(translate(", ")) });
}

/**
 * Execute a command locally. Requires addChatMessage.
 *
 * @param {string} input
 * @returns {Boolean} whether a command was executed
 */
function executeNetworkCommand(input)
{
	if (input.indexOf("/") != 0)
		return false;

	let command = input.split(" ", 1)[0];
	let argument = input.substr(command.length + 1);

	if (g_NetworkCommands[command])
		g_NetworkCommands[command](argument);

	return !!g_NetworkCommands[command];
}

/**
 * Remember this warning for a few seconds.
 * Overwrite previous warnings for this user.
 *
 * @param msg - GUI message sent by NetServer or NetClient
 */
function addNetworkWarning(msg)
{
	if (!g_NetworkWarningTexts[msg.warntype])
	{
		warn("Unknown network warning type received: " + uneval(msg));
		return;
	}

	if (Engine.ConfigDB_GetValue("user", "overlay.netwarnings") != "true")
		return;

	g_NetworkWarnings[msg.guid || "server"] = {
		"added": Date.now(),
		"msg": msg
	};
}

/**
 * Colorizes and concatenates all network warnings.
 * Returns text and textWidth.
 */
function getNetworkWarnings()
{
	// Remove outdated messages
	for (let guid in g_NetworkWarnings)
	{
		if (Date.now() > g_NetworkWarnings[guid].added + g_NetworkWarningTimeout)
			delete g_NetworkWarnings[guid];

		if (guid != "server" && !g_PlayerAssignments[guid])
			delete g_NetworkWarnings[guid];
	}

	// Show local messages first
	let guids = Object.keys(g_NetworkWarnings).sort(guid => guid != "server");

	let font = Engine.GetGUIObjectByName("gameStateNotifications").font;

	let messages = [];
	let maxTextWidth = 0;

	for (let guid of guids)
	{
		let msg = g_NetworkWarnings[guid].msg;

		// Add formatted text
		messages.push(g_NetworkWarningTexts[msg.warntype](msg, colorizePlayernameByGUID(guid)));

		// Add width of unformatted text
		let username = guid != "server" && g_PlayerAssignments[guid].name;
		let textWidth = Engine.GetTextWidth(font, g_NetworkWarningTexts[msg.warntype](msg, username));
		maxTextWidth = Math.max(textWidth, maxTextWidth);
	}

	return {
		"messages": messages,
		"maxTextWidth": maxTextWidth
	};
}
