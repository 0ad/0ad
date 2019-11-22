GameListFilters.MapSize = class
{
	constructor(onFilterChange)
	{
		this.selected = 0;
		this.onFilterChange = onFilterChange;

		this.mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
		this.mapSizeFilter.list = [translateWithContext("map size", "Any"), ...g_MapSizes.Name];
		this.mapSizeFilter.list_data = ["", ...g_MapSizes.Tiles];
		this.mapSizeFilter.selected = 0;
		this.mapSizeFilter.onSelectionChange = this.onSelectionChange.bind(this);
	}

	onSelectionChange()
	{
		this.selected = this.mapSizeFilter.list_data[this.mapSizeFilter.selected];
		this.onFilterChange();
	}

	filter(game)
	{
		return !this.selected || game.stanza.mapSize == this.selected;
	}
};
