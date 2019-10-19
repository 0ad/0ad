/**
 * This class manages the player selection dropdown.
 * This dropdown is available in observermode and when enabling the developers option.
 * For observers, the user can view the player but not send commands.
 * If the developer feature is enabled and cheats enabled, the player becomes
 * assigned to and can control the selected player.
 */
class PlayerViewControl
{
	constructor(diplomacyColors)
	{
		// State
		this.viewPlayer = Engine.GetGUIObjectByName("viewPlayer");
		this.observerText = Engine.GetGUIObjectByName("observerText");
		this.changePerspective = false;
		this.playerIDChangeHandlers = [];
		this.viewedPlayerChangeHandlers = [];
		this.preViewedPlayerChangeHandlers = [];

		// Events
		this.viewPlayer.onSelectionChange = this.onSelectionChange.bind(this);
		registerPlayersInitHandler(this.onPlayersInit.bind(this));
		registerPlayersFinishedHandler(this.onPlayersFinished.bind(this));
		this.registerViewedPlayerChangeHandler(this.rebuild.bind(this));
	}

	registerPlayerIDChangeHandler(handler)
	{
		this.playerIDChangeHandlers.push(handler);
	}

	registerViewedPlayerChangeHandler(handler)
	{
		this.viewedPlayerChangeHandlers.push(handler);
	}

	registerPreViewedPlayerChangeHandler(handler)
	{
		this.preViewedPlayerChangeHandlers.push(handler);
	}

	rebuild()
	{
		this.viewPlayer.list_data = [-1].concat(g_Players.map((player, i) => i));
		this.viewPlayer.list = [translate(this.ObserverTitle)].concat(g_Players.map(
			(player, i) => colorizePlayernameHelper("■", i) + " " + player.name
		));
		this.viewPlayer.hidden = !g_IsObserver && !this.changePerspective;
		this.observerText.hidden = g_ViewedPlayer > 0;
	}

	/**
	 * Select "observer" in the view player dropdown when rejoining as a defeated player.
	 */
	onPlayersInit()
	{
		this.rebuild();

		let playerState = g_Players[Engine.GetPlayerID()];
		this.selectViewPlayer(playerState && playerState.state == "defeated" ? 0 : Engine.GetPlayerID() + 1);
	}

	/**
	 * Select "observer" item on loss. On win enable observermode without changing perspective.
	 */
	onPlayersFinished(playerIDs, won)
	{
		if (playerIDs.indexOf(g_ViewedPlayer) != -1)
			this.selectViewPlayer(won ? g_ViewedPlayer + 1 : 0);
	}

	setChangePerspective(enabled)
	{
		this.changePerspective = enabled;
		this.rebuild();
		this.onSelectionChange();
	}

	selectViewPlayer(playerID)
	{
		this.viewPlayer.selected = playerID;
	}

	onSelectionChange()
	{
		let playerID = this.viewPlayer.selected - 1;
		if (playerID < -1 || playerID > g_Players.length - 1)
		{
			error("Can't assume invalid player ID: " + playerID);
			return;
		}

		for (let handler of this.preViewedPlayerChangeHandlers)
			handler();

		// TODO: should set this state variable only once in this scope
		g_IsObserver = isPlayerObserver(Engine.GetPlayerID());

		if (g_IsObserver || this.changePerspective)
		{
			if (g_ViewedPlayer != playerID)
				clearSelection();
			g_ViewedPlayer = playerID;
		}

		if (this.changePerspective)
		{
			Engine.SetPlayerID(g_ViewedPlayer);
			g_IsObserver = isPlayerObserver(g_ViewedPlayer);
		}
		Engine.SetViewedPlayer(g_ViewedPlayer);

		// Send events after all states were updated
		if (this.changePerspective)
			for (let handler of this.playerIDChangeHandlers)
				handler();

		for (let handler of this.viewedPlayerChangeHandlers)
			handler();
	}
}

PlayerViewControl.prototype.ObserverTitle = markForTranslation("Observer");
