class MapGridBrowserItem extends GridBrowserItem
{
	constructor(mapBrowserPage, mapGridBrowser, imageObject, itemIndex)
	{
		super(mapGridBrowser, imageObject, itemIndex);

		this.mapBrowserPage = mapBrowserPage;
		this.mapCache = mapBrowserPage.mapCache;

		this.mapPreview = Engine.GetGUIObjectByName("mapPreview[" + itemIndex + "]");

		mapGridBrowser.registerSelectionChangeHandler(this.onSelectionChange.bind(this));
		mapGridBrowser.registerPageChangeHandler(this.onGridResize.bind(this));

		if (g_IsController)
			this.imageObject.onMouseLeftDoubleClick = this.onMouseLeftDoubleClick.bind(this);
	}

	onSelectionChange()
	{
		this.updateSprite();
	}

	onGridResize()
	{
		super.onGridResize();
		this.updateMapAssignment();
		this.updateSprite();
	}

	updateSprite()
	{
		this.imageObject.sprite =
			this.gridBrowser.selected == this.itemIndex + this.gridBrowser.currentPage * this.gridBrowser.itemsPerRow ?
				this.SelectedSprite :
				"";
	}

	updateMapAssignment()
	{
		let map = this.gridBrowser.mapList[
			this.itemIndex + this.gridBrowser.currentPage * this.gridBrowser.itemsPerRow] || undefined;

		if (!map)
			return;

		this.mapPreview.caption = map.name;

		this.imageObject.tooltip =
			map.description + "\n" +
			this.gridBrowser.container.tooltip;

		this.mapPreview.sprite =
			this.mapCache.getMapPreview(this.mapBrowserPage.controls.MapFiltering.getSelectedMapType(), map.file);
	}

	onMouseLeftDoubleClick()
	{
		// Only submit the map selection if no save is loaded
		if (!g_isSaveLoaded)
			this.mapBrowserPage.submitMapSelection();
	}
}

MapGridBrowserItem.prototype.SelectedSprite = "color: 120 0 0 255";
