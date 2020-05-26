/**
 * The leaderboard page allows the player to view the highest rated players and update that list.
 */
class LeaderboardPage
{
	constructor(xmppMessages)
	{
		this.openPageHandlers = new Set();
		this.closePageHandlers = new Set();

		this.leaderboardList = new LeaderboardList(xmppMessages);

		this.leaderboardPage = Engine.GetGUIObjectByName("leaderboardPage");

		Engine.GetGUIObjectByName("leaderboardUpdateButton").onPress = this.onPressUpdate.bind(this);
		Engine.GetGUIObjectByName("leaderboardPageBack").onPress = this.onPressClose.bind(this);
	}

	registerOpenPageHandler(handler)
	{
		this.openPageHandlers.add(handler);
	}

	registerClosePageHandler(handler)
	{
		this.closePageHandlers.add(handler);
	}

	openPage()
	{
		this.leaderboardPage.hidden = false;
		Engine.SetGlobalHotkey("cancel", "Press", this.onPressClose.bind(this));
		Engine.SendGetBoardList();

		let playerName = this.leaderboardList.selectedPlayer();
		for (let handler of this.openPageHandlers)
			handler(playerName);
	}

	onPressUpdate()
	{
		Engine.SendGetBoardList();
	}

	onPressClose()
	{
		this.leaderboardPage.hidden = true;

		for (let handler of this.closePageHandlers)
			handler();
	}
}
