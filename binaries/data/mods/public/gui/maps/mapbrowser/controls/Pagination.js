MapBrowserPageControls.prototype.Pagination = class
{
	constructor(mapBrowserPage, gridBrowser)
	{
		this.status = Engine.GetGUIObjectByName("mapBrowserPageStatus");

		this.previous = Engine.GetGUIObjectByName("mapBrowserPreviousButton");
		this.next = Engine.GetGUIObjectByName("mapBrowserNextButton");

		this.zoomIn = Engine.GetGUIObjectByName("mapsZoomIn");
		this.zoomOut = Engine.GetGUIObjectByName("mapsZoomOut");

		this.gridBrowser = gridBrowser;
		this.gridBrowser.registerPageChangeHandler(() => this.render());
		this.gridBrowser.registerGridResizeHandler(() => this.render());

		this.setup();
		this.render();
	}

	setup()
	{
		this.previous.onPress = () => this.gridBrowser.previousPage();
		this.next.onPress = () => this.gridBrowser.nextPage();
		this.previous.caption = "←";
		this.next.caption = "→";
		this.previous.tooltip = translate("Go to the previous page.");
		this.next.tooltip = translate("Go to the next page.");

		this.zoomIn.onPress = () => this.gridBrowser.increaseColumnCount(-1);
		this.zoomOut.onPress = () => this.gridBrowser.increaseColumnCount(1);
		this.zoomIn.tooltip = translate("Increase map preview size.");
		this.zoomOut.tooltip = translate("Decrease map preview size.");
	}

	render()
	{
		this.status.caption =
			sprintf(translate("Maps: %(mapCount)s"), {
				"mapCount": this.gridBrowser.itemCount
			}) +
			"   " +
			sprintf(translate("Page: %(currentPage)s/%(maxPage)s"), {
				"currentPage": this.gridBrowser.currentPage + 1,
				"maxPage": Math.max(1, this.gridBrowser.pageCount)
			});

		this.previous.enabled = this.gridBrowser.pageCount > 1;
		this.next.enabled = this.gridBrowser.pageCount > 1;

		this.zoomIn.enabled = this.gridBrowser.columnCount > 0;
		this.zoomOut.enabled = this.gridBrowser.columnCount < this.gridBrowser.maxColumns;

	}
};
