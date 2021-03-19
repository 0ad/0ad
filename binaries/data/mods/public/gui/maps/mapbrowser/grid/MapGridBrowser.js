class MapGridBrowser extends GridBrowser
{
	constructor(mapBrowserPage, setupWindow)
	{
		super(Engine.GetGUIObjectByName("mapBrowserContainer"));

		this.setupWindow = setupWindow;
		this.mapBrowserPage = mapBrowserPage;
		this.mapCache = mapBrowserPage.mapCache;
		this.mapFilters = mapBrowserPage.mapFilters;

		this.mapList = [];

		this.items = this.container.children.map((imageObject, itemIndex) =>
			new MapGridBrowserItem(mapBrowserPage, this, imageObject, itemIndex));

		this.mapBrowserPage.registerOpenPageHandler(this.onOpenPage.bind(this));
		this.mapBrowserPage.registerClosePageHandler(this.onClosePage.bind(this));
		this.mapBrowserPage.mapBrowserPageDialog.onMouseWheelUp = () => this.previousPage(false);
		this.mapBrowserPage.mapBrowserPageDialog.onMouseWheelDown = () => this.nextPage(false);
	}

	onOpenPage()
	{
		this.updateMapList();
		this.goToPageOfSelected();
		this.container.onWindowResized = this.onWindowResized.bind(this);

		Engine.SetGlobalHotkey(this.HotkeyConfigNext, "Press", this.nextPage.bind(this));
		Engine.SetGlobalHotkey(this.HotkeyConfigPrevious, "Press", this.previousPage.bind(this));
	}

	onClosePage()
	{
		delete this.container.onWindowResized;
		Engine.UnsetGlobalHotkey(this.HotkeyConfigNext, "Press");
		Engine.UnsetGlobalHotkey(this.HotkeyConfigPrevious, "Press");
	}

	getSelectedFile()
	{
		return this.mapList[this.selected].file || undefined;
	}

	select(mapFile)
	{
		this.setSelectedIndex(this.mapList.findIndex(map => map.file == mapFile));
		this.goToPageOfSelected();
	}

	updateMapList()
	{
		let selectedMap =
			this.mapList[this.selected] &&
			this.mapList[this.selected].file || undefined;

		let mapList = this.mapFilters.getFilteredMaps(
			this.mapBrowserPage.controls.MapFiltering.getSelectedMapType(),
			this.mapBrowserPage.controls.MapFiltering.getSelectedMapFilter());

		let filterText = this.mapBrowserPage.controls.MapFiltering.getSearchText();
		if (filterText)
		{
			mapList = MatchSort.get(filterText, mapList, "name");
			if (!mapList.length)
			{
				let filter = "all";
				for (let type of g_MapTypes.Name)
					for (let map of this.mapFilters.getFilteredMaps(type, filter))
						mapList.push(Object.assign({ "type": type, "filter": filter }, map));
				mapList = MatchSort.get(filterText, mapList, "name");
			}
		}
		if (this.mapBrowserPage.controls.MapFiltering.getSelectedMapType() == "random")
		{
			mapList = [{
				"file": "random",
				"name": "Random",
				"description": "Pick a map at random.",
			}, ...mapList];
		}
		this.mapList = mapList;
		this.itemCount = this.mapList.length;
		this.resizeGrid();

		this.setSelectedIndex(this.mapList.findIndex(map => map.file == selectedMap));
	}
}

MapGridBrowser.prototype.ItemRatio = 4 / 3;

MapGridBrowser.prototype.DefaultItemWidth = 200;

MapGridBrowser.prototype.MinItemWidth = 100;
