ChatMessageEvents.ClientChat = class
{
	constructor(setupWindow, chatMessagesPanel)
	{
		this.chatMessagesPanel = chatMessagesPanel;

		this.usernameArgs = {};
		this.messageArgs = {};

		// TODO: Remove this global required by gui/common/
		global.colorizePlayernameByGUID = this.colorizePlayernameByGUID.bind(this);

		setupWindow.controls.netMessages.registerNetMessageHandler("chat", this.onClientChat.bind(this));
	}

	onClientChat(message)
	{
		this.usernameArgs.username = this.colorizePlayernameByGUID(message.guid);
		this.messageArgs.username = setStringTags(sprintf(this.SenderFormat, this.usernameArgs), this.SenderTags);
		this.messageArgs.message = escapeText(message.text);
		this.chatMessagesPanel.addText(sprintf(this.MessageFormat, this.messageArgs));
	}

	colorizePlayernameByGUID(guid)
	{
		// TODO: Controllers should have the moderator-prefix
		let username = g_PlayerAssignments[guid] ? escapeText(g_PlayerAssignments[guid].name) : translate("Unknown Player");
		let playerID = g_PlayerAssignments[guid] ? g_PlayerAssignments[guid].player : -1;

		let color = "white";
		if (playerID > 0)
		{
			color = g_GameAttributes.settings.PlayerData[playerID - 1].Color;

			// Enlighten playercolor to improve readability
			let [h, s, l] = rgbToHsl(color.r, color.g, color.b);
			let [r, g, b] = hslToRgb(h, s, Math.max(0.6, l));

			color = rgbToGuiColor({ "r": r, "g": g, "b": b });
		}

		return coloredText(username, color);
	}
};

ChatMessageEvents.ClientChat.prototype.SenderFormat =
	translate("<%(username)s>");

ChatMessageEvents.ClientChat.prototype.MessageFormat =
	translate("%(username)s %(message)s");

ChatMessageEvents.ClientChat.prototype.SenderTags = {
	"font": "sans-bold-13"
};
