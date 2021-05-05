MapBrowserPageControls.prototype.MapFiltering = class
{
	constructor(mapBrowserPage, gridBrowser)
	{
		this.mapBrowserPage = mapBrowserPage;
		this.gridBrowser = gridBrowser;
		this.mapFilters = mapBrowserPage.mapFilters;

		this.searchBox = new LabelledInput("mapBrowserSearchBox")
			.setupEvents(() => this.onChange());
		this.mapType = new LabelledDropdown("mapBrowserMapType")
			.setupEvents(() => this.onMapTypeChange());
		this.mapFilter = new LabelledDropdown("mapBrowserMapFilter")
			.setupEvents(() => this.onChange());

		mapBrowserPage.registerOpenPageHandler(() => this.onOpenPage());
		mapBrowserPage.registerClosePageHandler(() => this.onClosePage());

		this.searchBox.blur();
	}

	onOpenPage()
	{
		// setTimeout avoids having the hotkey key inserted into the input text.
		setTimeout(() => {
			this.searchBox.control.caption = "";
			this.searchBox.focus();
		}, 0);
	}

	onClosePage()
	{
		this.searchBox.blur();
	}

	onMapTypeChange()
	{
		this.renderMapFilter();
		this.onChange();
	}

	onChange()
	{
		this.gridBrowser.updateMapList();
		this.gridBrowser.goToPageOfSelected();
	}

	select(filter, type)
	{
		this.mapType.render(g_MapTypes.Title, g_MapTypes.Name);
		this.mapType.select(type);
		this.renderMapFilter();
		this.mapFilter.select(filter);
		this.gridBrowser.updateMapList();
		this.gridBrowser.goToPageOfSelected();
	}

	renderMapFilter()
	{
		let filters = this.mapFilters.getAvailableMapFilters(this.getSelectedMapType());
		this.mapFilter.render(filters.map(f => f.Title), filters.map(f => f.Name));
	}

	// TODO: would be nicer to store this state somewhere else.
	getSearchText()
	{
		return this.searchBox.getText() || "";
	}

	getSelectedMapType()
	{
		return this.mapType.getSelected() || "";
	}

	getSelectedMapFilter()
	{
		return this.mapFilter.getSelected() || "";
	}
};
