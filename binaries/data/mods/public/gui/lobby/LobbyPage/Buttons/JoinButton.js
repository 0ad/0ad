/**
 * This class manages the button that enables the player to join a lobby game hosted by a remote player.
 */
class JoinButton
{
	constructor(dialog, gameList)
	{
		this.gameList = gameList;

		this.joinButton = Engine.GetGUIObjectByName("joinButton");
		this.joinButton.caption = this.Caption;
		this.joinButton.hidden = dialog;
		if (!dialog)
			this.joinButton.onPress = this.onPress.bind(this);

		gameList.gamesBox.onMouseLeftDoubleClickItem = this.onPress.bind(this);
		gameList.registerSelectionChangeHandler(this.onSelectedGameChange.bind(this, dialog));
	}

	onSelectedGameChange(dialog, selectedGame)
	{
		this.joinButton.hidden = dialog || !selectedGame;
	}

	/**
	 * Immediately rejoin and join gamesetups. Otherwise confirm late-observer join attempt.
	 */
	onPress()
	{
		let game = this.gameList.selectedGame();
		if (!game)
			return;

		let rating = this.getRejoinRating(game);
		let playername = rating ? g_Nickname + " (" + rating + ")" : g_Nickname;

		if (!game.isCompatible)
			messageBox(
				400, 200,
				translate("Your active mods do not match the mods of this game.") + "\n\n" +
					comparedModsString(game.mods, Engine.GetEngineInfo().mods) + "\n\n" +
					translate("Do you want to switch to the mod selection page?"),
				translate("Incompatible mods"),
				[translate("No"), translate("Yes")],
				[null, this.openModSelectionPage.bind(this)]
			);
		else if (game.stanza.state == "init" || game.players.some(player => player.Name == playername))
			this.joinSelectedGame();
		else
			messageBox(
				400, 200,
				translate("The game has already started. Do you want to join as observer?"),
				translate("Confirmation"),
				[translate("No"), translate("Yes")],
				[null, this.joinSelectedGame.bind(this)]);
	}

	/**
	 * Attempt to join the selected game without asking for confirmation.
	 */
	joinSelectedGame()
	{
		if (this.joinButton.hidden)
			return;

		let game = this.gameList.selectedGame();
		if (!game)
			return;

		let ip;
		let port;
		let stanza = game.stanza;
		if (stanza.stunIP)
		{
			ip = stanza.stunIP;
			port = stanza.stunPort;
		}
		else
		{
			ip = stanza.ip;
			port = stanza.port;
		}

		if (ip.split('.').length != 4)
		{
			messageBox(
				400, 250,
				sprintf(
					translate("This game's address '%(ip)s' does not appear to be valid."),
					{ "ip": escapeText(stanza.ip) }),
				translate("Error"));
			return;
		}

		Engine.PushGuiPage("page_gamesetup_mp.xml", {
			"multiplayerGameType": "join",
			"ip": ip,
			"port": port,
			"name": g_Nickname,
			"rating": this.getRejoinRating(stanza),
			"useSTUN": !!stanza.stunIP,
			"hostJID": stanza.hostUsername + "@" + Engine.ConfigDB_GetValue("user", "lobby.server") + "/0ad"
		});
	}

	openModSelectionPage()
	{
		Engine.StopXmppClient();
		Engine.SwitchGuiPage("page_modmod.xml", {
			"cancelbutton": true
		});
	}

	/**
	 * Rejoin games with the original playername, even if the rating changed meanwhile.
	 */
	getRejoinRating(game)
	{
		for (let player of game.players)
		{
			let playerNickRating = splitRatingFromNick(player.Name);
			if (playerNickRating.nick == g_Nickname)
				return playerNickRating.rating;
		}
		return Engine.LobbyGetPlayerRating(g_Nickname);
	}
}

// Translation: Join the game currently selected in the list.
JoinButton.prototype.Caption = translate("Join Game");
