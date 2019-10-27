/**
 * This class determines the player colors to be displayed.
 * If diplomacy color mode is disabled, it picks the player color chosen by the map or the players.
 * If diplomacy color mode is enabled, it choses the player chosen color based on diplomatic stance.
 * Observers that didn't chose a specific player perspective see each team in one representative color.
 */
class DiplomacyColors
{
	constructor()
	{
		this.enabled = false;

		// The array of displayed player colors (either the diplomacy color or regular color for each player).
		this.displayedPlayerColors = undefined;

		this.diplomacyColorsChangeHandlers = [];

		registerPlayersInitHandler(this.onPlayersInit.bind(this));
		registerConfigChangeHandler(this.onConfigChange.bind(this));
		registerCeasefireEndedHandler(this.onCeasefireEnded.bind(this));
	}

	registerDiplomacyColorsChangeHandler(handler)
	{
		this.diplomacyColorsChangeHandlers.push(handler);
	}

	onPlayersInit()
	{
		this.computeTeamColors();
	}

	onDiplomacyChange()
	{
		if (this.enabled)
			this.updateDisplayedPlayerColors();
	}

	onCeasefireEnded()
	{
		if (this.enabled)
			this.updateDisplayedPlayerColors();
	}

	onConfigChange(changes)
	{
		for (let change of changes)
			if (change.startsWith("gui.session.diplomacycolors."))
			{
				this.updateDisplayedPlayerColors();
				return;
			}
	}

	isEnabled()
	{
		return this.enabled;
	}

	toggle()
	{
		this.enabled = !this.enabled;
		this.updateDisplayedPlayerColors();
	}

	getPlayerColor(playerID, alpha)
	{
		return rgbToGuiColor(this.displayedPlayerColors[playerID], alpha);
	}

	/**
	 * Updates the displayed colors of players in the simulation and GUI.
	 */
	updateDisplayedPlayerColors()
	{
		this.computeTeamColors();

		Engine.GuiInterfaceCall("UpdateDisplayedPlayerColors", {
			"displayedPlayerColors": this.displayedPlayerColors,
			"displayDiplomacyColors": this.enabled,
			"showAllStatusBars": g_ShowAllStatusBars,
			"selected": g_Selection.toList()
		});

		for (let handler of this.diplomacyColorsChangeHandlers)
			handler(this.enabled);
	}

	computeTeamColors()
	{
		if (!this.enabled)
		{
			this.displayedPlayerColors = g_Players.map(player => player.color);
			return;
		}

		let teamRepresentatives = {};
		for (let i = 1; i < g_Players.length; ++i)
			if (g_ViewedPlayer <= 0)
			{
				// Observers and gaia see team colors
				let team = g_Players[i].team;
				this.displayedPlayerColors[i] = g_Players[teamRepresentatives[team] || i].color;
				if (team != -1 && !teamRepresentatives[team])
					teamRepresentatives[team] = i;
			}
			else
				// Players see colors depending on diplomacy
				this.displayedPlayerColors[i] =
					g_ViewedPlayer == i ? this.getDiplomacyColor("self") :
					g_Players[g_ViewedPlayer].isAlly[i] ? this.getDiplomacyColor("ally") :
					g_Players[g_ViewedPlayer].isNeutral[i] ? this.getDiplomacyColor("neutral") :
					this.getDiplomacyColor("enemy");

		this.displayedPlayerColors[0] = g_Players[0].color;
	}

	getDiplomacyColor(stance)
	{
		return guiToRgbColor(Engine.ConfigDB_GetValue("user", "gui.session.diplomacycolors." + stance)) ||
		       guiToRgbColor(Engine.ConfigDB_GetValue("default", "gui.session.diplomacycolors." + stance));
	}
}
