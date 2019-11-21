/**
 * Formats a chat message sent by a player (i.e. not a chat notification),
 * accounting for chat format commands such as /me or /say and private messages.
 *
 * Plays an acoustic notification if the playername was mentioned
 */
class ChatMessageFormat
{
	constructor()
	{
		this.chatMessageFormatMe = new ChatMessageFormatMe();
		this.chatMessageFormatSay = new ChatMessageFormatSay();
		this.chatMessagePrivateWrapper = new ChatMessagePrivateWrapper();
	}

	/**
	 * Message properties: from, text, historic, optionally private
	 */
	format(message)
	{
		let text = escapeText(message.text);
		if (g_Nickname != message.from)
		{
			// Highlight nicknames, assume they do not contain escapaped characters
			text = text.replace(g_Nickname, PlayerColor.ColorPlayerName(g_Nickname));

			// Notify local player
			if (!message.historic && text.toLowerCase().indexOf(g_Nickname.toLowerCase()) != -1)
				soundNotification("nick");
		}

		let sender = PlayerColor.ColorPlayerName(message.from, undefined, Engine.LobbyGetPlayerRole(message.from));

		// Handle chat format commands
		let formattedMessage;
		let index = text.indexOf(" ");
		if (text.startsWith("/") && index != -1)
		{
			let command = text.substr(1, index - 1);
			let commandText = text.substr(index + 1);

			switch (command)
			{
			case "me":
			{
				formattedMessage = this.chatMessageFormatMe.format(sender, commandText);
				break;
			}
			case "say":
			{
				formattedMessage = this.chatMessageFormatSay.format(sender, commandText);
				break;
			}
			default:
			{
				formattedMessage = this.chatMessageFormatSay.format(sender, text);
				break;
			}
			}
		}
		else
			formattedMessage = this.chatMessageFormatSay.format(sender, text);

		if (message.level == "private-message")
			formattedMessage = this.chatMessagePrivateWrapper.format(formattedMessage);

		return formattedMessage;
	}
}
