/**
 * This class parses network events sent from the NetClient, such as players connecting or disconnecting from the game.
 */
class ChatMessageFormatNetwork
{
}

ChatMessageFormatNetwork.clientlist = class
{
	parse()
	{
		return getUsernameList();
	}
};

ChatMessageFormatNetwork.connect = class
{
	parse(msg)
	{
		return sprintf(
			g_PlayerAssignments[msg.guid].player != -1 ?
				// Translation: A player that left the game joins again
				translate("%(player)s is starting to rejoin the game.") :
				// Translation: A player joins the game for the first time
				translate("%(player)s is starting to join the game."),
			{ "player": colorizePlayernameByGUID(msg.guid) });
	}
};

ChatMessageFormatNetwork.disconnect = class
{
	parse(msg)
	{
		return sprintf(translate("%(player)s has left the game."), {
			"player": colorizePlayernameByGUID(msg.guid)
		});
	}
};

ChatMessageFormatNetwork.kicked = class
{
	parse(msg)
	{
		return sprintf(
			msg.banned ?
				translate("%(username)s has been banned") :
				translate("%(username)s has been kicked"),
			{
				"username": colorizePlayernameHelper(
					msg.username,
					g_Players.findIndex(p => p.name == msg.username)
				)
			});
	}
};

ChatMessageFormatNetwork.rejoined = class
{
	parse(msg)
	{
		return sprintf(
			g_PlayerAssignments[msg.guid].player != -1 ?
				// Translation: A player that left the game joins again
				translate("%(player)s has rejoined the game.") :
				// Translation: A player joins the game for the first time
				translate("%(player)s has joined the game."),
			{ "player": colorizePlayernameByGUID(msg.guid) });
	}
};
