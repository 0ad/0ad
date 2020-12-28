class GridBrowserItem
{
	constructor(gridBrowser, imageObject, itemIndex)
	{
		this.gridBrowser = gridBrowser;
		this.itemIndex = itemIndex;
		this.imageObject = imageObject;

		imageObject.onMouseLeftPress = this.select.bind(this);
		imageObject.onMouseWheelDown = () => gridBrowser.nextPage(false);
		imageObject.onMouseWheelUp = () => gridBrowser.previousPage(false);

		gridBrowser.registerGridResizeHandler(this.onGridResize.bind(this));
		gridBrowser.registerPageChangeHandler(this.updateVisibility.bind(this));
	}

	updateVisibility()
	{
		this.imageObject.hidden =
			this.itemIndex >= Math.min(
				this.gridBrowser.itemsPerPage,
				this.gridBrowser.itemCount - this.gridBrowser.currentPage * this.gridBrowser.itemsPerRow);
	}

	onGridResize()
	{
		let gridBrowser = this.gridBrowser;
		let x = this.itemIndex % gridBrowser.columnCount;
		let y = Math.floor(this.itemIndex / gridBrowser.columnCount);
		let size = this.imageObject.size;
		size.left = gridBrowser.itemWidth * x;
		size.right = gridBrowser.itemWidth * (x + 1);
		size.top = gridBrowser.itemHeight * y;
		size.bottom = gridBrowser.itemHeight * (y + 1);
		this.imageObject.size = size;
		this.updateVisibility();
	}

	select()
	{
		this.gridBrowser.setSelectedIndex(
			this.itemIndex + this.gridBrowser.currentPage * this.gridBrowser.itemsPerRow);
	}
}
