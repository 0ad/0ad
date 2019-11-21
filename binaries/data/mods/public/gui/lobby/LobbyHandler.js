/**
 * This class owns the page handlers.
 */
class LobbyHandler
{
	constructor(dialog)
	{
		this.xmppMessages = new XmppMessages();

		this.profilePage = new ProfilePage(this.xmppMessages);
		this.leaderboardPage = new LeaderboardPage(this.xmppMessages);
		this.lobbyPage = new LobbyPage(dialog, this.xmppMessages, this.leaderboardPage, this.profilePage);

		this.xmppMessages.processHistoricMessages();

		if (Engine.LobbyGetPlayerPresence(g_Nickname) != "available")
			Engine.LobbySetPlayerPresence("available");

		if (!dialog)
		{
			initMusic();
			global.music.setState(global.music.states.MENU);
		}
	}
}
