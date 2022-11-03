/**
 * This class is concerned with opening, closing the chat page, and
 * resizing it depending on whether the chat history is shown.
 */
class ChatWindow
{
	constructor()
	{
		this.chatInput = Engine.GetGUIObjectByName("chatInput");
		this.closeChat = Engine.GetGUIObjectByName("closeChat");

		this.extendedChat = Engine.GetGUIObjectByName("extendedChat");
		this.chatHistoryText = Engine.GetGUIObjectByName("chatHistoryText");
		this.chatHistoryPage = Engine.GetGUIObjectByName("chatHistoryPage");

		this.chatDialogPanel = Engine.GetGUIObjectByName("chatDialogPanel");
		this.chatDialogPanelSmallSize = Engine.GetGUIObjectByName("chatDialogPanelSmall").size;
		this.chatDialogPanelLargeSize = Engine.GetGUIObjectByName("chatDialogPanelLarge").size;

		// Adjust the width so that the chat history is in the golden ratio
		this.aspectRatio = (1 + Math.sqrt(5)) / 2;

		this.initPage();
	}

	initPage()
	{
		this.closeChat.onPress = this.closePage.bind(this);

		this.extendedChat.onPress = () => {
			Engine.ConfigDB_CreateAndSaveValue("user", "chat.session.extended", String(this.isExtended()));
			this.resizeChatWindow();
			this.chatInput.focus();
		};

		this.extendedChat.checked = Engine.ConfigDB_GetValue("user", "chat.session.extended") == "true";

		this.chatDialogPanel.onWindowResized = this.resizeChatWindow.bind(this);
		this.resizeChatWindow();
	}

	/**
	 * Called if the addressee or history filter selection changed.
	 */
	onSelectionChange()
	{
		if (this.isOpen())
			this.chatInput.focus();
	}

	isOpen()
	{
		return !this.chatDialogPanel.hidden;
	}

	isExtended()
	{
		return this.extendedChat.checked;
	}

	openPage(command)
	{
		this.chatInput.focus();
		this.chatDialogPanel.hidden = false;
	}

	closePage()
	{
		this.chatInput.caption = "";
		this.chatInput.blur();
		this.chatDialogPanel.hidden = true;
	}

	resizeChatWindow()
	{
		// Hide/show the panel
		this.chatHistoryPage.hidden = !this.isExtended();

		// Resize the window
		if (this.isExtended())
		{
			this.chatDialogPanel.size = this.chatDialogPanelLargeSize;

			let chatHistoryTextSize = this.chatHistoryText.getComputedSize();
			let width = this.aspectRatio * (chatHistoryTextSize.bottom - chatHistoryTextSize.top);

			let size = this.chatDialogPanel.size;
			size.left = -width / 2 - this.chatHistoryText.size.left;
			size.right = width / 2 + this.chatHistoryText.size.left;
			this.chatDialogPanel.size = size;
		}
		else
			this.chatDialogPanel.size = this.chatDialogPanelSmallSize;
	}
}
