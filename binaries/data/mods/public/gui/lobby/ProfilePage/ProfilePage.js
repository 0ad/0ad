/**
 * The profile page enables the player to lookup statistics of an arbitrary player.
 */
class ProfilePage
{
	constructor(xmppMessages)
	{
		this.requestedPlayer = undefined;
		this.closePageHandlers = new Set();

		this.profilePage = Engine.GetGUIObjectByName("profilePage");

		this.fetchInput = Engine.GetGUIObjectByName("fetchInput");
		this.fetchInput.onPress = this.onPressLookup.bind(this);

		Engine.GetGUIObjectByName("viewProfileButton").onPress = this.onPressLookup.bind(this);
		Engine.GetGUIObjectByName("profileBackButton").onPress = this.onPressClose.bind(this, true);

		this.profilePlayernameText = Engine.GetGUIObjectByName("profilePlayernameText");
		this.profileRankText = Engine.GetGUIObjectByName("profileRankText");
		this.profileHighestRatingText = Engine.GetGUIObjectByName("profileHighestRatingText");
		this.profileTotalGamesText = Engine.GetGUIObjectByName("profileTotalGamesText");
		this.profileWinsText = Engine.GetGUIObjectByName("profileWinsText");
		this.profileLossesText = Engine.GetGUIObjectByName("profileLossesText");
		this.profileRatioText = Engine.GetGUIObjectByName("profileRatioText");
		this.profileErrorText = Engine.GetGUIObjectByName("profileErrorText");
		this.profileWindowArea = Engine.GetGUIObjectByName("profileWindowArea");

		xmppMessages.registerXmppMessageHandler("game", "profile", this.onProfile.bind(this));
	}

	registerClosePageHandler(handler)
	{
		this.closePageHandlers.add(handler);
	}

	openPage()
	{
		this.profilePage.hidden = false;
		Engine.SetGlobalHotkey("cancel", "Press", this.onPressClose.bind(this));
	}

	onPressLookup()
	{
		this.requestedPlayer = this.fetchInput.caption;
		Engine.SendGetProfile(this.requestedPlayer);
	}

	onPressClose()
	{
		this.profilePage.hidden = true;

		for (let handler of this.closePageHandlers)
			handler();
	}

	onProfile()
	{
		let attributes = Engine.GetProfile()[0];
		if (this.profilePage.hidden || this.requestedPlayer != attributes.player)
			return;

		let profileFound = attributes.rating != "-2";
		this.profileWindowArea.hidden = !profileFound;
		this.profileErrorText.hidden = profileFound;

		if (!profileFound)
		{
			this.profileErrorText.caption =
				sprintf(translate("Player \"%(nick)s\" not found."), {
					"nick": escapeText(attributes.player)
				});
			return;
		}

		this.profilePlayernameText.caption = escapeText(attributes.player);
		this.profileRankText.caption = attributes.rank;
		this.profileHighestRatingText.caption = attributes.highestRating;
		this.profileTotalGamesText.caption = attributes.totalGamesPlayed;
		this.profileWinsText.caption = attributes.wins;
		this.profileLossesText.caption = attributes.losses;
		this.profileRatioText.caption = ProfilePanel.FormatWinRate(attributes);
	}
}
