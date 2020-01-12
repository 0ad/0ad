/**
 * This class stores the handlers for all GUI objects in the lobby page,
 * (excluding other pages in the same context such as leaderboard and profile page).
 */
class LobbyPage
{
	constructor(dialog, xmppMessages, leaderboardPage, profilePage)
	{
		Engine.ProfileStart("Create LobbyPage");
		let mapCache = new MapCache();
		let buddyButton = new BuddyButton(xmppMessages);
		let gameList = new GameList(xmppMessages, buddyButton, mapCache);
		let playerList = new PlayerList(xmppMessages, buddyButton, gameList);

		this.lobbyPage = {
			"buttons": {
				"buddyButton": buddyButton,
				"hostButton": new HostButton(dialog, xmppMessages),
				"joinButton": new JoinButton(dialog, gameList),
				"leaderboardButton": new LeaderboardButton(xmppMessages, leaderboardPage),
				"profileButton": new ProfileButton(xmppMessages, profilePage),
				"quitButton": new QuitButton(dialog, leaderboardPage, profilePage)
			},
			"panels": {
				"chatPanel": new ChatPanel(xmppMessages),
				"gameDetails": new GameDetails(dialog, gameList, mapCache),
				"gameList": gameList,
				"playerList": playerList,
				"profilePanel": new ProfilePanel(xmppMessages, playerList, leaderboardPage),
				"subject": new Subject(dialog, xmppMessages, gameList)
			},
			"eventHandlers": {
				"announcementHandler": new AnnouncementHandler(xmppMessages),
				"connectionHandler": new ConnectionHandler(xmppMessages),
			}
		};

		if (dialog)
			this.setDialogStyle();
		Engine.ProfileStop();
	}

	setDialogStyle()
	{
		{
			let lobbyPage = Engine.GetGUIObjectByName("lobbyPage");
			lobbyPage.sprite = "ModernDialog";

			let size = lobbyPage.size;
			size.left = this.WindowMargin;
			size.top = this.WindowMargin;
			size.right = -this.WindowMargin;
			size.bottom = -this.WindowMargin;
			lobbyPage.size = size;
		}

		{
			let lobbyPageTitle = Engine.GetGUIObjectByName("lobbyPageTitle");
			let size = lobbyPageTitle.size;
			size.top -= this.WindowMargin / 2;
			size.bottom -= this.WindowMargin / 2;
			lobbyPageTitle.size = size;
		}

		{
			let lobbyPanels = Engine.GetGUIObjectByName("lobbyPanels");
			let size = lobbyPanels.size;
			size.top -= this.WindowMargin / 2;
			lobbyPanels.size = size;
		}
	}
}

LobbyPage.prototype.WindowMargin = 40;
