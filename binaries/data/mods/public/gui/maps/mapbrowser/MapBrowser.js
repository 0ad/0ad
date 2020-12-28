class MapBrowser
{
	constructor(mapCache, mapFilters, setupWindow = undefined)
	{
		this.openPageHandlers = new Set();
		this.closePageHandlers = new Set();

		this.mapCache = mapCache;
		this.mapFilters = mapFilters;

		this.mapBrowserPage = Engine.GetGUIObjectByName("mapBrowserPage");
		this.mapBrowserPageDialog = Engine.GetGUIObjectByName("mapBrowserPageDialog");

		this.gridBrowser = new MapGridBrowser(this, setupWindow);
		this.controls = new MapBrowserPageControls(this, this.gridBrowser);

		this.open = false;
	}

	// TODO: this is mostly gamesetup specific stuff.
	registerOpenPageHandler(handler)
	{
		this.openPageHandlers.add(handler);
	}

	registerClosePageHandler(handler)
	{
		this.closePageHandlers.add(handler);
	}

	openPage()
	{
		if (this.open)
			return;
		for (let handler of this.openPageHandlers)
			handler();
		this.open = true;
	}

	closePage()
	{
		if (!this.open)
			return;
		for (let handler of this.closePageHandlers)
			handler();
		this.open = false;
	}
}
