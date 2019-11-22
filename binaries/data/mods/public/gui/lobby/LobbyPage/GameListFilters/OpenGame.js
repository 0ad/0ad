GameListFilters.OpenGame = class
{
	constructor(onFilterChange)
	{
		this.checked = false;
		this.onFilterChange = onFilterChange;

		this.filterOpenGames = Engine.GetGUIObjectByName("filterOpenGames");
		this.filterOpenGames.checked = false;
		this.filterOpenGames.onPress = this.onPress.bind(this);
	}

	onPress()
	{
		this.checked = this.filterOpenGames.checked;
		this.onFilterChange();
	}

	filter(game)
	{
		let stanza = game.stanza;
		return !this.checked || stanza.state == "init" && stanza.nbp < stanza.maxnbp;
	}
};
