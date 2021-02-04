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
		this.setSelectedIndex(this.mapList.findIndex(map => map.file == g_GameAttributes.map));
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

	submitMapSelection()
	{
		if (!g_IsController)
			return;

		let map = this.mapList[this.selected] || undefined;
		if (!map)
			return;

		g_GameAttributes.mapType = map.type ? map.type :
			this.mapBrowserPage.controls.MapFiltering.getSelectedMapType();
		g_GameAttributes.mapFilter = map.filter ? map.filter :
			this.mapBrowserPage.controls.MapFiltering.getSelectedMapFilter();
		g_GameAttributes.map = map.file;
		this.setupWindow.controls.gameSettingsControl.updateGameAttributes();
		this.setupWindow.controls.gameSettingsControl.setNetworkGameAttributes();
	}
}

MapGridBrowser.prototype.ItemRatio = 4 / 3;

MapGridBrowser.prototype.DefaultItemWidth = 200;

MapGridBrowser.prototype.MinItemWidth = 100;
