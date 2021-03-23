/**
 * The purpose of this message is to indicate to the local player when settings they had agreed on changed.
 */
ChatMessageEvents.GameSettingsChanged = class
{
	constructor(setupWindow, chatMessagesPanel)
	{
		this.readyController = setupWindow.controls.readyController;
		this.chatMessagesPanel = chatMessagesPanel;

		this.readyController.registerResetReadyHandler(this.onResetReady.bind(this));
	}

	onResetReady()
	{
		if (this.readyController.getLocalReadyState() == this.readyController.Ready)
			this.chatMessagesPanel.addStatusMessage(this.MessageText);
	}
};

ChatMessageEvents.GameSettingsChanged.prototype.MessageText =
	translate("Game settings have been changed");
