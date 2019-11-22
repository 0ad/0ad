/**
 * The purpose of this class is to process the chat input of the local player and
 * either submit the input as chat or perform it as a local or remote command.
 */
class ChatInputPanel
{
	constructor(xmppMessages, chatMessagesPanel, systemMessageFormat)
	{
		this.chatCommandHandler = new ChatCommandHandler(chatMessagesPanel, systemMessageFormat);

		this.chatSubmit = Engine.GetGUIObjectByName("chatSubmit");
		this.chatSubmit.onPress = this.submitChatInput.bind(this);

		this.chatInput = Engine.GetGUIObjectByName("chatInput");
		this.chatInput.onPress = this.submitChatInput.bind(this);
		this.chatInput.onTab = this.autocomplete.bind(this);
		this.chatInput.tooltip = colorizeAutocompleteHotkey();

		let update = this.update.bind(this);
		xmppMessages.registerXmppMessageHandler("system", "connected", update);
		xmppMessages.registerXmppMessageHandler("system", "disconnected", update);
		xmppMessages.registerXmppMessageHandler("chat", "role", update);

		this.update();
	}

	update()
	{
		let hidden = !Engine.IsXmppClientConnected() || Engine.LobbyGetPlayerRole(g_Nickname) == "visitor";
		this.chatInput.hidden = hidden;
		this.chatSubmit.hidden = hidden;
	}

	submitChatInput()
	{
		let text = this.chatInput.caption;
		if (!text.length)
			return;

		if (!this.chatCommandHandler.handleChatCommand(text))
			Engine.LobbySendMessage(text);

		this.chatInput.caption = "";
	}

	autocomplete()
	{
		autoCompleteText(
			this.chatInput,
			Engine.GetPlayerList().map(player => player.name));
	}
}
