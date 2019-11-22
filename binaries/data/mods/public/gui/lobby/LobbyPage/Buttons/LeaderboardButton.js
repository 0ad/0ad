/**
 * This class deals with the button that opens the leaderboard page.
 */
class LeaderboardButton
{
	constructor(xmppMessages, leaderboardPage)
	{
		this.leaderboardButton = Engine.GetGUIObjectByName("leaderboardButton");
		this.leaderboardButton.caption = translate("Leaderboard");
		this.leaderboardButton.onPress = leaderboardPage.openPage.bind(leaderboardPage);

		let onConnectionStatusChange = this.onConnectionStatusChange.bind(this);
		xmppMessages.registerXmppMessageHandler("system", "connected", onConnectionStatusChange);
		xmppMessages.registerXmppMessageHandler("system", "disconnected", onConnectionStatusChange);
		this.onConnectionStatusChange();
	}

	onConnectionStatusChange()
	{
		this.leaderboardButton.enabled = Engine.IsXmppClientConnected();
	}
}
