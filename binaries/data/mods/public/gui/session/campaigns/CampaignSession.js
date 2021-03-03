class CampaignSession
{
	constructor(data)
	{
		this.run = new CampaignRun(data.run).load();
		this.levelID = data.levelID;
		registerPlayersFinishedHandler(this.onFinish.bind(this));
		this.endGameData = {
			"levelID": data.levelID,
			"won": false,
			"custom": {}
		};
	}

	onFinish(players, won)
	{
		let playerID = Engine.GetPlayerID();
		if (players.indexOf(playerID) === -1)
			return;

		this.endGameData.custom = Engine.GuiInterfaceCall("GetCampaignGameEndData", {
			"player": playerID
		});
		this.endGameData.won = won;

		// Run the endgame script.
		Engine.PushGuiPage(this.getEndGame(), this.endGameData);
		Engine.PopGuiPage();
	}

	getMenu()
	{
		return this.run.getMenuPath();
	}

	getEndGame()
	{
		return this.run.getEndGamePath();
	}
}

var g_CampaignSession;
