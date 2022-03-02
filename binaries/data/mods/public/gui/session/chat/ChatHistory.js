/**
 * The objective of this class is to build a message type filter selection and
 * to store and display the chat history according to that selection.
 */
class ChatHistory
{
	constructor()
	{
		/**
		 * All unparsed chat messages received since connect, including timestamp.
		 */
		this.chatMessages = [];

		this.selectionChangeHandlers = [];

		this.chatHistoryFilterCaption = Engine.GetGUIObjectByName("chatHistoryFilterCaption");
		resizeGUIObjectToCaption(this.chatHistoryFilterCaption, { "horizontal": "right" });

		this.chatHistoryFilter = Engine.GetGUIObjectByName("chatHistoryFilter");
		let filters = prepareForDropdown(this.Filters.filter(chatFilter => !chatFilter.hidden));
		this.chatHistoryFilter.list = filters.text.map(text => translateWithContext("chat history filter", text));
		this.chatHistoryFilter.list_data = filters.key;
		this.chatHistoryFilter.selected = 0;
		this.chatHistoryFilter.onSelectionChange = this.onSelectionChange.bind(this);

		const chatHistoryFilterSize = this.chatHistoryFilter.size;
		chatHistoryFilterSize.left = this.chatHistoryFilterCaption.size.right + this.FilterMargin;
		this.chatHistoryFilter.size = chatHistoryFilterSize;

		this.chatHistoryText = Engine.GetGUIObjectByName("chatHistoryText");
	}

	registerSelectionChangeHandler(handler)
	{
		this.selectionChangeHandlers.push(handler);
	}

	/**
	 * Called each time the history filter changes.
	 */
	onSelectionChange()
	{
		this.displayChatHistory();

		for (let handler of this.selectionChangeHandlers)
			handler();
	}

	displayChatHistory()
	{
		let selected = this.chatHistoryFilter.list_data[this.chatHistoryFilter.selected];

		this.chatHistoryText.caption =
			this.chatMessages.filter(msg => msg.filter[selected]).map(msg =>
				Engine.ConfigDB_GetValue("user", "chat.timestamp") == "true" ?
					sprintf(translate("%(time)s %(message)s"), {
						"time": msg.timePrefix,
						"message": msg.txt
					}) :
					msg.txt).join("\n");
	}

	onChatMessage(msg, formatted)
	{
		// Save to chat history
		let historical = {
			"txt": formatted.text,
			"timePrefix": sprintf(translate("\\[%(time)s]"), {
				"time": Engine.FormatMillisecondsIntoDateStringLocal(Date.now(), translate("HH:mm"))
			}),
			"filter": {}
		};

		// Apply the filters now before diplomacies or playerstates change
		let senderID = msg.guid && g_PlayerAssignments[msg.guid] ? g_PlayerAssignments[msg.guid].player : 0;
		for (let filter of this.Filters)
			historical.filter[filter.key] = filter.filter(msg, senderID);

		this.chatMessages.push(historical);
	}
}

/**
 * Notice only messages will be filtered that are visible to the player in the first place.
 */
ChatHistory.prototype.Filters = [
	{
		"key": "all",
		"text": markForTranslationWithContext("chat history filter", "Chat and notifications"),
		"filter": (msg, senderID) => true
	},
	{
		"key": "chat",
		"text": markForTranslationWithContext("chat history filter", "Chat messages"),
		"filter": (msg, senderID) => msg.type == "message"
	},
	{
		"key": "player",
		"text": markForTranslationWithContext("chat history filter", "Players chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			senderID > 0 && !isPlayerObserver(senderID)
	},
	{
		"key": "ally",
		"text": markForTranslationWithContext("chat history filter", "Ally chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			msg.cmd && msg.cmd == "/allies"
	},
	{
		"key": "enemy",
		"text": markForTranslationWithContext("chat history filter", "Enemy chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			msg.cmd && msg.cmd == "/enemies"
	},
	{
		"key": "observer",
		"text": markForTranslationWithContext("chat history filter", "Observer chat"),
		"filter": (msg, senderID) =>
			msg.type == "message" &&
			msg.cmd && msg.cmd == "/observers"
	},
	{
		"key": "private",
		"text": markForTranslationWithContext("chat history filter", "Private chat"),
		"filter": (msg, senderID) => !!msg.isVisiblePM
	},
	{
		"key": "gamenotifications",
		"text": markForTranslationWithContext("chat history filter", "Game notifications"),
		"filter": (msg, senderID) => msg.type != "message" && msg.guid === undefined
	},
	{
		"key": "chatnotifications",
		"text": markForTranslationWithContext("chat history filter", "Network notifications"),
		"filter": (msg, senderID) => msg.type != "message" && msg.guid !== undefined,
		"hidden": !Engine.HasNetClient()
	}
];

ChatHistory.prototype.FilterMargin = 5;
