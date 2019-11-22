/**
 * The purpose of this class is to test if a given textual input of the current player
 * is not a chat message to be sent but a command to be performed locally or on the
 * server, and if so perform it.
 */
class ChatCommandHandler
{
	constructor(chatMessagesPanel, systemMessageFormat)
	{
		this.chatMessagesPanel = chatMessagesPanel;
		this.systemMessageFormat = systemMessageFormat;
	}

	/**
	 * @returns {boolean} true if the input was successfully parsed as a chat command.
	 */
	handleChatCommand(text)
	{
		if (!text.startsWith('/'))
			return false;

		let index = text.indexOf(" ");
		let command = text.substr(1, index == -1 ? undefined : index - 1);
		let args = index == -1 ? "" : text.substr(index + 1);

		let commandObj = this.ChatCommands[command] || undefined;
		if (!commandObj)
		{
			this.chatMessagesPanel.addText(
				Date.now() / 1000,
				this.systemMessageFormat.format(
					sprintf(translate("The command '%(cmd)s' is not supported."), {
						"cmd": setStringTags(escapeText(command), this.ChatCommandTags)
					})));
			this.chatMessagesPanel.flushMessages();
			return true;
		}

		if (commandObj.moderatorOnly && Engine.LobbyGetPlayerRole(g_Nickname) != "moderator")
		{
			this.chatMessagesPanel.addText(
				Date.now() / 1000,
				this.systemMessageFormat.format(
					sprintf(translate("The command '%(cmd)s' is restricted to moderators."), {
						"cmd": setStringTags(escapeText(command), this.ChatCommandTags)
					})));
			this.chatMessagesPanel.flushMessages();
			return true;
		}

		let handler = commandObj && commandObj.handler || undefined;
		if (!handler)
			return false;

		return handler.call(this, args);
	}

	argumentCount(commandName, args)
	{
		if (args.trim())
			return false;

		this.chatMessagesPanel.addText(
			Date.now() / 1000,
			this.systemMessageFormat.format(
				sprintf(translate("The command '%(cmd)s' requires at least one argument."), {
					"cmd": setStringTags(commandName, this.ChatCommandTags)
				})));
		this.chatMessagesPanel.flushMessages();
		return true;
	}
}

/**
 * Color to highlight chat commands in the explanation.
 */
ChatCommandHandler.prototype.ChatCommandTags = {
	"color": "200 200 255"
};

/**
 * Commands that can be entered by clients via chat input.
 * A handler returns true if the user input should be sent as a chat message.
 */
ChatCommandHandler.prototype.ChatCommands = {
	"away": {
		"description": translate("Set your state to 'Away'."),
		"handler": function(args) {
			Engine.LobbySetPlayerPresence("away");
			return true;
		}
	},
	"back": {
		"description": translate("Set your state to 'Online'."),
		"handler": function(args) {
			Engine.LobbySetPlayerPresence("available");
			return true;
		}
	},
	"kick": {
		"description": translate("Kick a specified user from the lobby. Usage: /kick nick reason"),
		"handler": function(args) {
			let index = args.indexOf(" ");
			if (index == -1)
				Engine.LobbyKick(args, "");
			else
				Engine.LobbyKick(args.substr(0, index), args.substr(index + 1));
			return true;
		},
		"moderatorOnly": true
	},
	"ban": {
		"description": translate("Ban a specified user from the lobby. Usage: /ban nick reason"),
		"handler": function(args) {
			let index = args.indexOf(" ");
			if (index == -1)
				Engine.LobbyBan(args, "");
			else
				Engine.LobbyBan(args.substr(0, index), args.substr(index + 1));
			return true;
		},
		"moderatorOnly": true
	},
	"help": {
		"description": translate("Show this help."),
		"handler": function(args) {
			let isModerator = Engine.LobbyGetPlayerRole(g_Nickname) == "moderator";
			let txt = translate("Chat commands:");
			for (let command in this.ChatCommands)
				if (!this.ChatCommands[command].moderatorOnly || isModerator)
					// Translation: Chat command help format
					txt += "\n" + sprintf(translate("%(command)s - %(description)s"), {
						"command": setStringTags(command, this.ChatCommandTags),
						"description": this.ChatCommands[command].description
					});

			this.chatMessagesPanel.addText(
				Date.now() / 1000,
				this.systemMessageFormat.format(txt));

			this.chatMessagesPanel.flushMessages();
			return true;
		}
	},
	"me": {
		"description": translate("Send a chat message about yourself. Example: /me goes swimming."),
		"handler": function(args) {
			// Translation: Chat command
			return this.argumentCount(translate("/me"), args);
		}
	},
	"say": {
		"description": translate("Send text as a chat message (even if it starts with slash). Example: /say /help is a great command."),
		"handler": function(args) {
			// Translation: Chat command
			return this.argumentCount(translate("/say"), args);
		}
	},
	"clear": {
		"description": translate("Clear all chat scrollback."),
		"handler": function(args) {
			this.chatMessagesPanel.clearChatMessages();
			return true;
		}
	}
};
