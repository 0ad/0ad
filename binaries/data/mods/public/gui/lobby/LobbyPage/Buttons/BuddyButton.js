/**
 * This class manages the button that enables the player to add or remove buddies.
 */
class BuddyButton
{
	constructor(xmppMessages)
	{
		this.buddyChangedHandlers = new Set();
		this.playerName = undefined;

		this.toggleBuddyButton = Engine.GetGUIObjectByName("toggleBuddyButton");
		this.toggleBuddyButton.onPress = this.onPress.bind(this);

		let rebuild = this.rebuild.bind(this);
		xmppMessages.registerXmppMessageHandler("system", "connected", rebuild);
		xmppMessages.registerXmppMessageHandler("system", "disconnected", rebuild);

		this.rebuild();
	}

	registerBuddyChangeHandler(handler)
	{
		this.buddyChangedHandlers.add(handler);
	}

	onPlayerSelectionChange(playerName)
	{
		this.playerName = playerName;
		this.rebuild();
	}

	rebuild()
	{
		this.toggleBuddyButton.caption =
			g_Buddies.indexOf(this.playerName) != -1 ?
				this.UnmarkString :
				this.MarkString;

		this.toggleBuddyButton.enabled = Engine.IsXmppClientConnected() && !!this.playerName && this.playerName != g_Nickname;
	}

	/**
	 * Toggle the buddy state of the selected player.
	 */
	onPress()
	{
		if (!this.playerName || this.playerName == g_Nickname || this.playerName.indexOf(g_BuddyListDelimiter) != -1)
			return;

		let index = g_Buddies.indexOf(this.playerName);
		if (index != -1)
			g_Buddies.splice(index, 1);
		else
			g_Buddies.push(this.playerName);

		Engine.ConfigDB_CreateAndWriteValueToFile(
			"user",
			"lobby.buddies",
			g_Buddies.filter(nick => nick).join(g_BuddyListDelimiter) || g_BuddyListDelimiter,
			"config/user.cfg");

		this.rebuild();

		for (let handler of this.buddyChangedHandlers)
			handler();
	}
}

BuddyButton.prototype.MarkString = translate("Mark as Buddy");
BuddyButton.prototype.UnmarkString = translate("Unmark as Buddy");
