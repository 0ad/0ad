/**
 * This class represents a multiplayer match hosted by a player in the lobby.
 * Having this represented as a class allows to leverage significant performance
 * gains by caching computed, escaped, translated strings and sorting keys.
 *
 * Additionally class representation allows implementation of events such as
 * a new match being hosted, a match having ended, or a buddy having joined a match.
 *
 * Ensure that escapeText is applied to player controlled data for display.
 *
 * Users of the properties of this class:
 * GameList, GameDetails, MapFilters, JoinButton, any user of GameList.selectedGame()
 */
class Game
{
	constructor(mapCache)
	{
		this.mapCache = mapCache;

		// Stanza data, object with exclusively string values
		// Used to compare which part of the stanza data changed,
		// perform partial updates and trigger event notifications.
		this.stanza = {};
		for (let name of this.StanzaKeys)
			this.stanza[name] = "";

		// This will be displayed in the GameList and GameDetails
		// Important: Player input must be processed with escapeText
		this.displayData = {
			"tags": {}
		};

		// Cache the values used for sorting
		this.sortValues = {
			"state": "",
			"compatibility": "",
			"hasBuddyString": ""
		};

		// Array of objects, result of stringifiedTeamListToPlayerData
		this.players = [];

		// Whether the current player has the same mods launched as the host of this game
		this.isCompatible = undefined;

		// Used to display which mods are missing if the player attempts a join
		this.mods = undefined;

		// Used by the rating column and rating filer
		this.gameRating = undefined;

		// 'Persistent temporary' sprintf arguments object to avoid repeated object construction
		this.playerCountArgs = {};
	}

	/**
	 * Called from GameList to ensure call order.
	 */
	onBuddyChange()
	{
		this.updatePlayers(this.stanza);
	}

	/**
	 * This function computes values that will either certainly or
	 * most likely be used later (i.e. by filtering, sorting and gamelist display).
	 *
	 * The performance benefit arises from the fact that for a new gamelist stanza
	 * many if not most games and game properties did not change.
	 */
	update(newStanza, sortKey)
	{
		let oldStanza = this.stanza;
		let displayData = this.displayData;
		let sortValues = this.sortValues;

		if (oldStanza.name != newStanza.name)
		{
			Engine.ProfileStart("gameName");
			sortValues.gameName = newStanza.name.toLowerCase();
			this.updateGameName(newStanza);
			Engine.ProfileStop();
		}

		if (oldStanza.state != newStanza.state)
		{
			Engine.ProfileStart("gameState");
			this.updateGameTags(newStanza);
			sortValues.state = this.GameStatusOrder.indexOf(newStanza.state);
			Engine.ProfileStop();
		}

		if (oldStanza.niceMapName != newStanza.niceMapName)
		{
			Engine.ProfileStart("niceMapName");
			displayData.mapName = escapeText(this.mapCache.translateMapName(newStanza.niceMapName));
			displayData.mapDescription = this.mapCache.getTranslatedMapDescription(newStanza.mapType, newStanza.mapName);
			Engine.ProfileStop();
		}

		if (oldStanza.mapName != newStanza.mapName)
		{
			Engine.ProfileStart("mapName");
			sortValues.mapName = displayData.mapName;
			Engine.ProfileStop();
		}

		if (oldStanza.mapType != newStanza.mapType)
		{
			Engine.ProfileStart("mapType");
			displayData.mapType = g_MapTypes.Title[g_MapTypes.Name.indexOf(newStanza.mapType)] || "";
			sortValues.mapType = newStanza.mapType;
			Engine.ProfileStop();
		}

		if (oldStanza.mapSize != newStanza.mapSize)
		{
			Engine.ProfileStart("mapSize");
			displayData.mapSize = translateMapSize(newStanza.mapSize);
			sortValues.mapSize = newStanza.mapSize;
			Engine.ProfileStop();
		}

		let playersChanged = oldStanza.players != newStanza.players;
		if (playersChanged)
		{
			Engine.ProfileStart("playerData");
			this.updatePlayers(newStanza);
			Engine.ProfileStop();
		}

		if (oldStanza.nbp != newStanza.nbp ||
		    oldStanza.maxnbp != newStanza.maxnbp ||
		    playersChanged)
		{
			Engine.ProfileStart("playerCount");
			displayData.playerCount = this.getTranslatedPlayerCount(newStanza);
			sortValues.maxnbp = newStanza.maxnbp;
			Engine.ProfileStop();
		}

		if (oldStanza.mods != newStanza.mods)
		{
			Engine.ProfileStart("mods");
			this.updateMods(newStanza);
			Engine.ProfileStop();
		}

		this.stanza = newStanza;
		this.sortValue = this.sortValues[sortKey];
	}

	updatePlayers(newStanza)
	{
		let players;
		{
			Engine.ProfileStart("stringifiedTeamListToPlayerData");
			players = stringifiedTeamListToPlayerData(newStanza.players);
			this.players = players;
			Engine.ProfileStop();
		}

		{
			Engine.ProfileStart("parsePlayers");
			let observerCount = 0;
			let hasBuddies = 0;

			let playerRatingTotal = 0;
			for (let player of players)
			{
				let playerNickRating = splitRatingFromNick(player.Name);

				if (player.Team == "observer")
					++observerCount;
				else
					playerRatingTotal += playerNickRating.rating || g_DefaultLobbyRating;

				// Sort games with playing buddies above games with spectating buddies
				if (hasBuddies < 2 && g_Buddies.indexOf(playerNickRating.nick) != -1)
					hasBuddies = player.Team == "observer" ? 1 : 2;
			}

			this.observerCount = observerCount;
			this.hasBuddies = hasBuddies;

			let displayData = this.displayData;
			let sortValues = this.sortValues;
			displayData.buddy = this.hasBuddies ? setStringTags(g_BuddySymbol, displayData.tags) : "";
			sortValues.hasBuddyString = String(hasBuddies);
			sortValues.buddy = sortValues.hasBuddyString + sortValues.gameName;

			let playerCount = players.length - observerCount;
			let gameRating =
				playerCount ?
					Math.round(playerRatingTotal / playerCount) :
					g_DefaultLobbyRating;
			this.gameRating = gameRating;
			sortValues.gameRating = gameRating;
			Engine.ProfileStop();
		}
	}

	updateMods(newStanza)
	{
		{
			Engine.ProfileStart("JSON.parse");
			try
			{
				this.mods = JSON.parse(newStanza.mods);
			}
			catch (e)
			{
				this.mods = [];
			}
			Engine.ProfileStop();
		}

		{
			Engine.ProfileStart("hasSameMods");
			let isCompatible = this.mods && hasSameMods(this.mods, Engine.GetEngineInfo().mods);
			if (this.isCompatible != isCompatible)
			{
				this.isCompatible = isCompatible;
				this.updateGameTags(newStanza);
				this.sortValues.compatibility = String(isCompatible);
			}
			Engine.ProfileStop();
		}
	}

	updateGameTags(newStanza)
	{
		let displayData = this.displayData;
		displayData.tags = this.isCompatible ? this.StateTags[newStanza.state] : this.IncompatibleTags;
		displayData.buddy = this.hasBuddies ? setStringTags(g_BuddySymbol, displayData.tags) : "";
		this.updateGameName(newStanza);
	}

	updateGameName(newStanza)
	{
		let displayData = this.displayData;
		displayData.gameName = setStringTags(escapeText(newStanza.name), displayData.tags);

		let sortValues = this.sortValues;
		sortValues.gameName = sortValues.compatibility + sortValues.state + sortValues.gameName;
		sortValues.buddy = sortValues.hasBuddyString + sortValues.gameName;
	}

	getTranslatedPlayerCount(newStanza)
	{
		let playerCountArgs = this.playerCountArgs;
		playerCountArgs.current = setStringTags(escapeText(newStanza.nbp), this.PlayerCountTags.CurrentPlayers);
		playerCountArgs.max = setStringTags(escapeText(newStanza.maxnbp), this.PlayerCountTags.MaxPlayers);

		let txt;
		if (this.observerCount)
		{
			playerCountArgs.observercount = setStringTags(this.observerCount, this.PlayerCountTags.Observers);
			txt = this.PlayerCountObservers;
		}
		else
			txt = this.PlayerCountNoObservers;

		return sprintf(txt, playerCountArgs);
	}
}

/**
 * These are all keys that occur in a gamelist stanza sent by XPartaMupp.
 */
Game.prototype.StanzaKeys = [
	"name",
	"ip",
	"port",
	"stunIP",
	"stunPort",
	"hostUsername",
	"state",
	"nbp",
	"maxnbp",
	"players",
	"mapName",
	"niceMapName",
	"mapSize",
	"mapType",
	"victoryConditions",
	"startTime",
	"mods"
];

/**
 * Initial sorting order of the gamelist.
 */
Game.prototype.GameStatusOrder = [
	"init",
	"waiting",
	"running"
];

// Translation: The number of players and observers in this game
Game.prototype.PlayerCountObservers = translate("%(current)s/%(max)s +%(observercount)s");

// Translation: The number of players in this game
Game.prototype.PlayerCountNoObservers = translate("%(current)s/%(max)s");

/**
 * Compatible games will be listed in these colors.
 */
Game.prototype.StateTags = {
	"init": {
		"color": "0 219 0"
	},
	"waiting": {
		"color": "255 127 0"
	},
	"running": {
		"color": "219 0 0"
	}
};

/**
 * Games that require different mods than the ones launched by the current player are grayed out.
 */
Game.prototype.IncompatibleTags = {
	"color": "gray"
};

/**
 * Color for the player count number in the games list.
 */
Game.prototype.PlayerCountTags = {
	"CurrentPlayers": {
		"color": "0 160 160"
	},
	"MaxPlayers": {
		"color": "0 160 160"
	},
	"Observers": {
		"color": "0 128 128"
	}
};
