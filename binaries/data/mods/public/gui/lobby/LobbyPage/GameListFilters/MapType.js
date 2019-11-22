GameListFilters.MapType = class
{
	constructor(onFilterChange)
	{
		this.selected = "";
		this.onFilterChange = onFilterChange;

		this.mapTypeFilter = Engine.GetGUIObjectByName("mapTypeFilter");
		this.mapTypeFilter.list = [translateWithContext("map", "Any")].concat(g_MapTypes.Title);
		this.mapTypeFilter.list_data = [""].concat(g_MapTypes.Name);
		this.mapTypeFilter.selected = g_MapTypes.Default;
		this.mapTypeFilter.onSelectionChange = this.onSelectionChange.bind(this);
	}

	onSelectionChange()
	{
		this.selected = this.mapTypeFilter.list_data[this.mapTypeFilter.selected];
		this.onFilterChange();
	}

	filter(game)
	{
		return !this.selected || game.stanza.mapType == this.selected;
	}
};
