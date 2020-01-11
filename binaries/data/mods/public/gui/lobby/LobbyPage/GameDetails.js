/**
 * The purpose of this class is to display information about the selected game.
 */
class GameDetails
{
	constructor(dialog, gameList, mapCache)
	{
		this.mapCache = mapCache;

		this.playernameArgs = {};
		this.playerCountArgs = {};
		this.gameStartArgs = {};

		this.lastGame = {};

		this.gameDetails = Engine.GetGUIObjectByName("gameDetails");

		this.sgMapName = Engine.GetGUIObjectByName("sgMapName");
		this.sgGame = Engine.GetGUIObjectByName("sgGame");
		this.sgPlayersNames = Engine.GetGUIObjectByName("sgPlayersNames");
		this.sgMapSize = Engine.GetGUIObjectByName("sgMapSize");
		this.sgMapPreview = Engine.GetGUIObjectByName("sgMapPreview");
		this.sgMapDescription = Engine.GetGUIObjectByName("sgMapDescription");

		gameList.registerSelectionChangeHandler(this.onGameListSelectionChange.bind(this));

		this.resize(dialog);
	}

	resize(dialog)
	{
		let bottom = Engine.GetGUIObjectByName(dialog ? "leaveButton" : "joinButton").size.top - 5;
		let size = this.gameDetails.size;
		size.bottom = bottom;
		this.gameDetails.size = size;
	}

	/**
	 * Populate the game info area with information on the current game selection.
	 */
	onGameListSelectionChange(game)
	{
		this.gameDetails.hidden = !game;
		if (!game)
			return;

		Engine.ProfileStart("GameDetails");

		let stanza = game.stanza;
		let displayData = game.displayData;

		if (stanza.mapType != this.lastGame.mapType || stanza.mapName != this.lastGame.mapName)
		{
			this.sgMapName.caption = displayData.mapName;
			this.sgMapPreview.sprite = this.mapCache.getMapPreview(stanza.mapType, stanza.mapName);
		}

		{
			let txt;
			if (game.isCompatible)
				txt =
					setStringTags(this.VictoryConditionsFormat, this.CaptionTags) + " " +
					(stanza.victoryConditions ?
						stanza.victoryConditions.split(",").map(translateVictoryCondition).join(this.Comma) :
						translateWithContext("victory condition", "Endless Game"));
			else
				txt =
					setStringTags(this.ModsFormat, this.CaptionTags) + " " +
					escapeText(modsToString(game.mods, Engine.GetEngineInfo().mods));

			txt +=
				"\n" + setStringTags(this.MapTypeFormat, this.CaptionTags) + " " + displayData.mapType +
				"\n" + setStringTags(this.MapSizeFormat, this.CaptionTags) + " " + displayData.mapSize +
				"\n" + setStringTags(this.MapDescriptionFormat, this.CaptionTags) + " " + displayData.mapDescription;

			this.sgMapDescription.caption = txt;
		}

		{
			let txt = escapeText(stanza.name);

			this.playernameArgs.playername = escapeText(stanza.hostUsername);
			txt += "\n" + sprintf(this.HostFormat, this.playernameArgs);

			this.playerCountArgs.current = escapeText(stanza.nbp);
			this.playerCountArgs.total = escapeText(stanza.maxnbp);
			txt += "\n" + sprintf(this.PlayerCountFormat, this.playerCountArgs);

			if (stanza.startTime)
			{
				this.gameStartArgs.time = Engine.FormatMillisecondsIntoDateStringLocal(+stanza.startTime * 1000, this.TimeFormat);
				txt += "\n" + sprintf(this.GameStartFormat, this.gameStartArgs);
			}

			this.sgGame.caption = txt;
		}

		{
			let textHeight = this.sgGame.getTextSize().height;

			let sgGameSize = this.sgGame.size;
			sgGameSize.bottom = textHeight;
			this.sgGame.size = sgGameSize;

			let sgPlayersNamesSize = this.sgPlayersNames.size;
			sgPlayersNamesSize.top = textHeight + 5;
			this.sgPlayersNames.size = sgPlayersNamesSize;
		}

		this.sgPlayersNames.caption = formatPlayerInfo(game.players);

		this.lastGame = game;
		Engine.ProfileStop();
	}
}

GameDetails.prototype.HostFormat = translate("Host: %(playername)s");

GameDetails.prototype.PlayerCountFormat = translate("Players: %(current)s/%(total)s");

GameDetails.prototype.VictoryConditionsFormat = translate("Victory Conditions:");

// Translation: Comma used to concatenate victory conditions
GameDetails.prototype.Comma = translate(", ");

GameDetails.prototype.ModsFormat = translate("Mods:");

// Translation: %(time)s is the hour and minute here.
GameDetails.prototype.GameStartFormat = translate("Game started at %(time)s");

GameDetails.prototype.TimeFormat = translate("HH:mm");

GameDetails.prototype.MapTypeFormat = translate("Map Type:");

GameDetails.prototype.MapSizeFormat = translate("Map Size:");

GameDetails.prototype.MapDescriptionFormat = translate("Map Description:");

GameDetails.prototype.CaptionTags = {
	"font": "sans-bold-14"
};
