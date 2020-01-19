/**
 * This class stores and displays the chat history since the login and
 * displays timestamps if enabled.
 */
class ChatMessagesPanel
{
	constructor(gameSettingsPanel)
	{
		this.gameSettingsPanel = gameSettingsPanel;

		this.chatHistory = "";

		this.statusMessageFormat = new StatusMessageFormat();

		if (Engine.ConfigDB_GetValue("user", this.ConfigTimestamp) == "true")
			this.timestampWrapper = new TimestampWrapper();

		this.chatText = Engine.GetGUIObjectByName("chatText");
		this.chatPanel = Engine.GetGUIObjectByName("chatPanel");
		this.chatPanel.onWindowResized = this.onWindowResized.bind(this);
		gameSettingsPanel.registerGameSettingsPanelResizeHandler(this.onGameSettingsPanelResize.bind(this));

		// TODO: Remove global requirements by gui/common/network.js
		g_NetworkCommands["/list"] = () => { this.addText(getUsernameList()); };
		g_NetworkCommands["/clear"] = this.clearChatMessages.bind(this);
		global.kickError = () => {};
	}

	addText(text)
	{
		if (this.timestampWrapper)
			text = this.timestampWrapper.format(text);

		this.chatHistory += this.chatHistory ? "\n" + text : text;
		this.chatText.caption = this.chatHistory;
	}

	addStatusMessage(text)
	{
		this.addText(this.statusMessageFormat.format(text));
	}

	clearChatMessages()
	{
		this.chatHistory = "";
		this.chatText.caption = "";
	}

	updateHidden()
	{
		let size = this.chatPanel.getComputedSize();
		this.chatPanel.hidden = !g_IsNetworked || size.right - size.left < this.MinimumWidth;
	}

	onWindowResized()
	{
		this.updateHidden();
	}

	onGameSettingsPanelResize(settingsPanel)
	{
		let size = this.chatPanel.size;
		size.right = settingsPanel.size.left + this.gameSettingsPanel.MaxColumnWidth + this.Margin;
		this.chatPanel.size = size;

		this.updateHidden();
	}
}

/**
 * Minimum amount of pixels required for the chat panel to be visible.
 */
ChatMessagesPanel.prototype.MinimumWidth = 96;

/**
 * Horizontal space between the chat window and the settings panel.
 */
ChatMessagesPanel.prototype.Margin = 10;

ChatMessagesPanel.prototype.ConfigTimestamp =
	"chat.timestamp";
