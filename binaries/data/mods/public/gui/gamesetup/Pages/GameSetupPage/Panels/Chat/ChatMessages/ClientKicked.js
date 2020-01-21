ChatMessageEvents.ClientKicked = class
{
	constructor(setupWindow, chatMessagesPanel)
	{
		this.chatMessagesPanel = chatMessagesPanel;

		this.messageArgs = {};

		setupWindow.controls.netMessages.registerNetMessageHandler("kicked", this.onClientKicked.bind(this));
	}

	onClientKicked(message)
	{
		this.messageArgs.username = message.username;
		this.chatMessagesPanel.addStatusMessage(sprintf(
			message.banned ? this.BannedMessage : this.KickedMessage,
			this.messageArgs));
	}
};

ChatMessageEvents.ClientKicked.prototype.KickedMessage =
	translate("%(username)s has been kicked");

ChatMessageEvents.ClientKicked.prototype.BannedMessage =
	translate("%(username)s has been banned");
