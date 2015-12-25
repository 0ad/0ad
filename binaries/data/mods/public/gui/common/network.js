/**
 * Get a human readable version of the given disconnect reason.
 *
 * @param reason {number}
 * @returns {string}
 */
function getDisconnectReason(id)
{
	// Must be kept in sync with source/network/NetHost.h
	switch (id)
	{
	case 0: return translate("Unknown reason");
	case 1: return translate("Unexpected shutdown");
	case 2: return translate("Incorrect network protocol version");
	case 3: return translate("Game is loading, please try later");
	case 4: return translate("Game has already started, no observers allowed");
	case 5: return translate("You have been kicked");
	case 6: return translate("You have been banned");
	default: return sprintf(translate("\\[Invalid value %(id)s]"), { "id": id });
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

/**
 * Get a colorized list of usernames sorted by player slot, observers last.
 * Requires g_PlayerAssignments and colorizePlayernameByGUID.
 *
 * @returns {string[]}
 */
function getUsernameList()
{
	let usernames = Object.keys(g_PlayerAssignments).sort((guidA, guidB) => {

		let playerIdA = g_PlayerAssignments[guidA].player;
		let playerIdB = g_PlayerAssignments[guidB].player;

		// Sort observers last
		if (playerIdA == -1) return +1;
		if (playerIdB == -1) return -1;

		// Sort players
		return playerIdA - playerIdB;

	}).map(guid => colorizePlayernameByGUID(guid));

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

	var command = input.split(" ", 1)[0];
	var argument = input.substr(command.length + 1);

	switch (command)
	{
	case "/list":
		addChatMessage({ "type": "clientlist", "guid": "local" });
		return true;

	case "/kick":
		if (!Engine.KickPlayer(argument, false))
			addChatMessage({ "type": "system", "text": sprintf(translate("Could not kick %(name)s."), { "name": argument }) });
		return true;

	case "/ban":
		if (!Engine.KickPlayer(argument, true))
			addChatMessage({ "type": "system", "text": sprintf(translate("Could not ban %(name)s."), { "name": argument }) });
		return true;

	case "/clear":
		clearChatMessages();
		return true;
	}
	return false;
}
