/**
 * This class fetches and displays player profile data,
 * where the player had been selected in the playerlist or leaderboard.
 */
class ProfilePanel
{
	constructor(xmppMessages, playerList, leaderboardPage)
	{
		// Playerlist or leaderboard selection
		this.requestedPlayer = undefined;

		// Playerlist selection
		this.selectedPlayer = undefined;

		this.roleText = Engine.GetGUIObjectByName("roleText");
		this.ratioText = Engine.GetGUIObjectByName("ratioText");
		this.lossesText = Engine.GetGUIObjectByName("lossesText");
		this.winsText = Engine.GetGUIObjectByName("winsText");
		this.totalGamesText = Engine.GetGUIObjectByName("totalGamesText");
		this.highestRatingText = Engine.GetGUIObjectByName("highestRatingText");
		this.rankText = Engine.GetGUIObjectByName("rankText");
		this.fade = Engine.GetGUIObjectByName("fade");
		this.playernameText = Engine.GetGUIObjectByName("playernameText");
		this.profileArea = Engine.GetGUIObjectByName("profileArea");

		xmppMessages.registerXmppMessageHandler("game", "profile", this.onProfile.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "role", this.onRoleChange.bind(this));

		playerList.registerSelectionChangeHandler(this.onPlayerListSelection.bind(this));

		leaderboardPage.registerOpenPageHandler(this.onLeaderboardOpenPage.bind(this));
		leaderboardPage.registerClosePageHandler(this.onLeaderboardClosePage.bind(this));
		leaderboardPage.leaderboardList.registerSelectionChangeHandler(this.onLeaderboardSelectionChange.bind(this));
	}

	onPlayerListSelection(playerName)
	{
		this.selectedPlayer = playerName;
		this.requestProfile(playerName);
	}

	onRoleChange(message)
	{
		if (message.nick == this.requestedPlayer)
			this.updatePlayerRoleText(this.requestedPlayer);
	}

	onLeaderboardOpenPage(playerName)
	{
		this.requestProfile(playerName);
	}

	onLeaderboardSelectionChange(playerName)
	{
		this.requestProfile(playerName);
	}

	onLeaderboardClosePage()
	{
		this.requestProfile(this.selectedPlayer);
	}

	updatePlayerRoleText(playerName)
	{
		this.roleText.caption = this.RoleNames[Engine.LobbyGetPlayerRole(playerName) || "participant"];
	}

	requestProfile(playerName)
	{
		this.profileArea.hidden = !playerName && !this.playernameText.caption;
		this.requestedPlayer = playerName;
		if (!playerName)
			return;

		this.playernameText.caption = playerName;
		this.updatePlayerRoleText(playerName);

		this.rankText.caption = this.NotAvailable;
		this.highestRatingText.caption = this.NotAvailable;
		this.totalGamesText.caption = this.NotAvailable;
		this.winsText.caption = this.NotAvailable;
		this.lossesText.caption = this.NotAvailable;
		this.ratioText.caption = this.NotAvailable;

		Engine.SendGetProfile(playerName);
	}

	onProfile()
	{
		let attributes = Engine.GetProfile()[0];
		if (attributes.rating == "-2" || attributes.player != this.requestedPlayer)
			return;

		this.playernameText.caption = attributes.player;
		this.updatePlayerRoleText(attributes.player);

		this.rankText.caption = attributes.rank;
		this.highestRatingText.caption = attributes.highestRating;
		this.totalGamesText.caption = attributes.totalGamesPlayed;
		this.winsText.caption = attributes.wins;
		this.lossesText.caption = attributes.losses;
		this.ratioText.caption = ProfilePanel.FormatWinRate(attributes);
	}
}

ProfilePanel.prototype.NotAvailable = translate("N/A");

/**
 * These role names correspond to the names constructed by the XmppClient.
 */
ProfilePanel.prototype.RoleNames = {
	"moderator": translate("Moderator"),
	"participant": translate("Player"),
	"visitor": translate("Muted Player")
};

ProfilePanel.FormatWinRate = function(attr)
{
	if (!attr.totalGamesPlayed)
		return translateWithContext("Used for an undefined winning rate", "-");

	return sprintf(translate("%(percentage)s%%"), {
		"percentage": (attr.wins / attr.totalGamesPlayed * 100).toFixed(2)
	});
};
