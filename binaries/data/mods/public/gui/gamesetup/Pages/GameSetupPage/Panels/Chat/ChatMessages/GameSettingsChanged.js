/**
 * The purpose of this message is to indicate to the local player when settings they had agreed on changed.
 */
ChatMessageEvents.GameSettingsChanged = class
{
	constructor(setupWindow, chatMessagesPanel)
	{
		this.readyControl = setupWindow.controls.readyControl;
		this.chatMessagesPanel = chatMessagesPanel;

		this.readyControl.registerResetReadyHandler(this.onResetReady.bind(this));
	}

	onResetReady()
	{
		if (this.readyControl.getLocalReadyState() == this.readyControl.Ready)
			this.chatMessagesPanel.addStatusMessage(this.MessageText);
	}
};

ChatMessageEvents.GameSettingsChanged.prototype.MessageText =
	translate("Game settings have been changed");
