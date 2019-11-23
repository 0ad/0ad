/**
 * This class interprets the given message as a chat text sent by a player to a selected addressee.
 * It supports the /me command, translation and acoustic notification.
 */
class ChatMessageFormatPlayer
{
	constructor()
	{
		this.AddresseeTypes = [];
	}

	registerAddresseeTypes(types)
	{
		this.AddresseeTypes = this.AddresseeTypes.concat(types);
	}

	parse(msg)
	{
		if (!msg.text)
			return "";

		let isMe = msg.text.startsWith("/me ");
		if (!isMe && !this.parseMessageAddressee(msg))
			return "";

		isMe = msg.text.startsWith("/me ");
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
			if (userName != g_PlayerAssignments[msg.guid].name &&
			    msg.text.toLowerCase().indexOf(splitRatingFromNick(userName).nick.toLowerCase()) != -1)
				soundNotification("nick");
		}

		// GUID for players, playerID for AIs
		let coloredUsername = msg.guid != -1 ? colorizePlayernameByGUID(msg.guid) : colorizePlayernameByID(msg.player);

		return sprintf(translate(this.strings[isMe ? "me" : "regular"][msg.context ? "context" : "no-context"]), {
			"message": msg.text,
			"context": msg.context ? translateWithContext("chat message context", msg.context) : "",
			"user": coloredUsername,
			"userTag": sprintf(translate("<%(user)s>"), { "user": coloredUsername })
		});
	}

	/**
	 * Checks if the current user is an addressee of the chatmessage sent by another player.
	 * Sets the context and potentially addresseeGUID of that message.
	 * Returns true if the message should be displayed.
	 */
	parseMessageAddressee(msg)
	{
		if (!msg.text.startsWith('/'))
			return true;

		// Split addressee command and message-text
		msg.cmd = msg.text.split(/\s/)[0];
		msg.text = msg.text.substr(msg.cmd.length + 1);

		// GUID is "local" in single-player, some string in multiplayer.
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
			addresseeGUID = this.matchUsername(msg.text);
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
		let addresseeType = this.AddresseeTypes.find(type => type.command == msg.cmd);
		if (!addresseeType)
		{
			if (isSender)
				warn("Unknown chat command: " + msg.cmd);
			return false;
		}
		msg.context = addresseeType.context;

		// For observers only permit public- and observer-chat and PM to observers
		if (isPlayerObserver(senderID) &&
		    (isPM && !isPlayerObserver(addresseeIndex) || !isPM && msg.cmd != "/observers"))
			return false;

		let visible = isSender || addresseeType.isAddressee(senderID, addresseeGUID);
		msg.isVisiblePM = isPM && visible;

		return visible;
	}

	/**
	 * Returns the guid of the user with the longest name that is a prefix of the given string.
	 */
	matchUsername(text)
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
}

/**
 * Chatmessage shown after commands like /me or /enemies.
 */
ChatMessageFormatPlayer.prototype.strings = {
	"regular": {
		"context": markForTranslation("(%(context)s) %(userTag)s %(message)s"),
		"no-context": markForTranslation("%(userTag)s %(message)s")
	},
	"me": {
		"context": markForTranslation("(%(context)s) * %(user)s %(message)s"),
		"no-context": markForTranslation("* %(user)s %(message)s")
	}
};
