GameListFilters.PlayerCount = class
{
	constructor(onFilterChange)
	{
		this.selected = "";
		this.onFilterChange = onFilterChange;

		let playersArray = Array(g_MaxPlayers).fill(0).map((v, i) => i + 1); // 1, 2, ... MaxPlayers
		this.playersNumberFilter = Engine.GetGUIObjectByName("playersNumberFilter");
		this.playersNumberFilter.list = [translateWithContext("player number", "Any")].concat(playersArray);
		this.playersNumberFilter.list_data = [""].concat(playersArray);
		this.playersNumberFilter.selected = 0;
		this.playersNumberFilter.onSelectionChange = this.onSelectionChange.bind(this);
	}

	onSelectionChange()
	{
		this.selected = this.playersNumberFilter.list_data[this.playersNumberFilter.selected];
		this.onFilterChange();
	}

	filter(game)
	{
		return !this.selected || game.stanza.maxnbp == this.selected;
	}
};
