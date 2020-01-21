/**
 * Properties of this prototype are classes that subscribe to one or more events and
 * construct a formatted chat message to be displayed on that event.
 *
 * Important: Apply escapeText on player provided input to avoid players breaking the game for everybody.
 */
class ChatMessageEvents
{
}

class ChatPanel
{
	constructor(setupWindow, gameSettingControlManager, gameSettingsPanel)
	{
		this.statusMessageFormat = new StatusMessageFormat();

		this.chatMessagesPanel = new ChatMessagesPanel(gameSettingsPanel);

		this.chatInputAutocomplete = new ChatInputAutocomplete(
			gameSettingControlManager, setupWindow.controls.gameSettingsControl, setupWindow.controls.playerAssignmentsControl);

		this.chatInputPanel = new ChatInputPanel(
			setupWindow.controls.netMessages, this.chatInputAutocomplete);

		this.chatMessageEvents = [];
		for (let name in ChatMessageEvents)
			this.chatMessageEvents.push(new ChatMessageEvents[name](setupWindow, this.chatMessagesPanel));
	}
}
