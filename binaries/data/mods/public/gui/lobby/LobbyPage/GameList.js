/**
 * Each property of this class handles one specific map filter and is defined in external files.
 */
class GameListFilters
{
}

/**
 * This class displays the list of multiplayer matches that are currently being set up or running,
 * filtered and sorted depending on player selection.
 */
class GameList
{
	constructor(xmppMessages, buddyButton, mapCache)
	{
		this.mapCache = mapCache;

		// Array of Game class instances, where the keys are ip+port strings, used for quick lookups
		this.games = {};

		// Array of Game class instances sorted by display order
		this.gameList = [];

		this.selectionChangeHandlers = new Set();

		this.gamesBox = Engine.GetGUIObjectByName("gamesBox");
		this.gamesBox.onSelectionChange = this.onSelectionChange.bind(this);
		this.gamesBox.onSelectionColumnChange = this.onFilterChange.bind(this);
		let ratingColumn = Engine.ConfigDB_GetValue("user", "lobby.columns.gamerating") == "true";
		this.gamesBox.hidden_mapType = ratingColumn;
		this.gamesBox.hidden_gameRating = !ratingColumn;

		// Avoid repeated array construction
		this.list_buddy = [];
		this.list_gameName = [];
		this.list_mapName = [];
		this.list_mapSize = [];
		this.list_mapType = [];
		this.list_maxnbp = [];
		this.list_gameRating = [];
		this.list = [];

		this.filters = [];
		for (let name in GameListFilters)
			this.filters.push(new GameListFilters[name](this.onFilterChange.bind(this)));

		xmppMessages.registerXmppMessageHandler("game", "gamelist", this.rebuildGameList.bind(this));
		xmppMessages.registerXmppMessageHandler("system", "disconnected", this.rebuildGameList.bind(this));

		buddyButton.registerBuddyChangeHandler(this.onBuddyChange.bind(this));

		this.rebuildGameList();
	}

	registerSelectionChangeHandler(handler)
	{
		this.selectionChangeHandlers.add(handler);
	}

	onFilterChange()
	{
		this.rebuildGameList();
	}

	onBuddyChange()
	{
		for (let name in this.games)
			this.games[name].onBuddyChange();

		this.rebuildGameList();
	}

	onSelectionChange()
	{
		let game = this.selectedGame();
		for (let handler of this.selectionChangeHandlers)
			handler(game);
	}

	selectedGame()
	{
		return this.gameList[this.gamesBox.selected] || undefined;
	}

	rebuildGameList()
	{
		Engine.ProfileStart("rebuildGameList");

		Engine.ProfileStart("getGameList");
		let selectedGame = this.selectedGame();
		let gameListData = Engine.GetGameList();
		Engine.ProfileStop();

		{
			Engine.ProfileStart("updateGames");
			let selectedColumn = this.gamesBox.selected_column;
			let newGames = {};
			for (let stanza of gameListData)
			{
				let game = this.games[stanza.ip] || undefined;
				let exists = !!game;
				if (!exists)
					game = new Game(this.mapCache);

				game.update(stanza, selectedColumn);
				newGames[stanza.ip] = game;
			}
			this.games = newGames;
			Engine.ProfileStop();
		}

		{
			Engine.ProfileStart("filterGameList");
			this.gameList.length = 0;
			for (let ip in this.games)
			{
				let game = this.games[ip];
				if (this.filters.every(filter => filter.filter(game)))
					this.gameList.push(game);
			}
			Engine.ProfileStop();
		}

		{
			Engine.ProfileStart("sortGameList");
			let sortOrder = this.gamesBox.selected_column_order;
			this.gameList.sort((game1, game2) => {
				if (game1.sortValue < game2.sortValue) return -sortOrder;
				if (game1.sortValue > game2.sortValue) return +sortOrder;
				return 0;
			});
			Engine.ProfileStop();
		}

		let selectedGameIndex = -1;

		{
			Engine.ProfileStart("setupGameList");
			let length = this.gameList.length;
			this.list_buddy.length = length;
			this.list_gameName.length = length;
			this.list_mapName.length = length;
			this.list_mapSize.length = length;
			this.list_mapType.length = length;
			this.list_maxnbp.length = length;
			this.list_gameRating.length = length;
			this.list.length = length;

			this.gameList.forEach((game, i) => {

				let displayData = game.displayData;
				this.list_buddy[i] = displayData.buddy;
				this.list_gameName[i] = displayData.gameName;
				this.list_mapName[i] = displayData.mapName;
				this.list_mapSize[i] = displayData.mapSize;
				this.list_mapType[i] = displayData.mapType;
				this.list_maxnbp[i] = displayData.playerCount;
				this.list_gameRating[i] = game.gameRating;
				this.list[i] = "";

				if (selectedGame && game.stanza.ip == selectedGame.stanza.ip && game.stanza.port == selectedGame.stanza.port)
					selectedGameIndex = i;
			});
			Engine.ProfileStop();
		}

		{
			Engine.ProfileStart("copyToGUI");
			let gamesBox = this.gamesBox;
			gamesBox.list_buddy = this.list_buddy;
			gamesBox.list_gameName = this.list_gameName;
			gamesBox.list_mapName = this.list_mapName;
			gamesBox.list_mapSize = this.list_mapSize;
			gamesBox.list_mapType = this.list_mapType;
			gamesBox.list_maxnbp = this.list_maxnbp;
			gamesBox.list_gameRating = this.list_gameRating;

			// Change these last, otherwise crash
			gamesBox.list = this.list;
			gamesBox.list_data = this.list;
			gamesBox.auto_scroll = false;
			Engine.ProfileStop();

			gamesBox.selected = selectedGameIndex;
		}

		Engine.ProfileStop();
	}

	/**
	 * Select the game where the selected player is currently playing, observing or offline.
	 * Selects in that order to account for players that occur in multiple games.
	 */
	selectGameFromPlayername(playerName)
	{
		if (!playerName)
			return;

		let foundAsObserver = false;

		for (let i = 0; i < this.gameList.length; ++i)
			for (let player of this.gameList[i].players)
			{
				if (playerName != splitRatingFromNick(player.Name).nick)
					continue;

				this.gamesBox.auto_scroll = true;
				if (player.Team == "observer")
				{
					foundAsObserver = true;
					this.gamesBox.selected = i;
				}
				else if (!player.Offline)
				{
					this.gamesBox.selected = i;
					return;
				}

				if (!foundAsObserver)
					this.gamesBox.selected = i;
			}
	}
}
