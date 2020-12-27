SetupWindowPages.MapBrowserPage = class extends MapBrowser
{
	constructor(setupWindow)
	{
		super(setupWindow.controls.mapCache, setupWindow.controls.mapFilters, setupWindow);
		this.mapBrowserPage.hidden = true;
	}

	openPage()
	{
		super.openPage();

		this.mapBrowserPage.hidden = false;
	}

	closePage()
	{
		super.closePage();

		this.mapBrowserPage.hidden = true;
	}
};
