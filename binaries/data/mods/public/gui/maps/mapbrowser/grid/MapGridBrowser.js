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

	getSelected()
	{
		return this.mapList[this.selected] || undefined;
	}

	select(mapFile)
	{
		this.setSelectedIndex(this.mapList.findIndex(map => map.file == mapFile));
		this.goToPageOfSelected();
	}

	updateMapList()
	{
		const selectedMap = this.mapList[this.selected]?.file;
		const mapType = this.mapBrowserPage.controls.MapFiltering.getSelectedMapType();
		const mapFilter = this.mapBrowserPage.controls.MapFiltering.getSelectedMapFilter();
		const filterText = this.mapBrowserPage.controls.MapFiltering.getSearchText();
		const randomMap = {
			"file": "random",
			"name": translateWithContext("map selection", "Random"),
			"description": translate("Pick a map at random."),
			"mapType": "random",
			"filter": "default"
		};

		let mapList = this.mapFilters.getFilteredMaps(mapType, mapFilter);

		if (mapType === "random")
			mapList.unshift(randomMap);

		if (filterText)
		{
			mapList = MatchSort.get(filterText, mapList, "name");
			if (!mapList.length)
			{
				const filter = "all";
				mapList.push(randomMap);
				for (let type of g_MapTypes.Name)
					for (let map of this.mapFilters.getFilteredMaps(type, filter))
						mapList.push(Object.assign({ "type": type, "filter": filter }, map));
				mapList = MatchSort.get(filterText, mapList, "name");
			}
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

MapGridBrowser.prototype.HotkeyConfigNext = "tab.next";

MapGridBrowser.prototype.HotkeyConfigPrevious = "tab.prev";
