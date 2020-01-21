ChatMessageEvents.ClientReady = class
{
	constructor(setupWindow, chatMessagesPanel)
	{
		this.chatMessagesPanel = chatMessagesPanel;

		this.args = {};

		setupWindow.controls.netMessages.registerNetMessageHandler("ready", this.onReadyMessage.bind(this));
	}

	onReadyMessage(message)
	{
		let playerAssignment = g_PlayerAssignments[message.guid];
		if (!playerAssignment || playerAssignment.player == -1)
			return;

		let text = this.ReadyMessage[message.status] || undefined;
		if (!text)
			return;

		this.args.username = playerAssignment.name;
		this.chatMessagesPanel.addText(sprintf(text, this.args));
	}
};

ChatMessageEvents.ClientReady.prototype.ReadyMessage = [
	translate("* %(username)s is not ready."),
	translate("* %(username)s is ready!")
];
