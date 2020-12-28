/**
 * Class that arranges a grid of items using paging.
 *
 * Needs an object as container with items and a object to display the page numbering (if not
 * make hidden object and assign it to that).
 */
class GridBrowser
{
	constructor(container)
	{
		this.container = container;

		// These properties may be read from publicly.
		this.pageCount = undefined;
		this.currentPage = undefined;
		this.columnCount = undefined;
		this.minColumns = undefined;
		this.maxColumns = undefined;
		this.rowCount = undefined;
		this.itemCount = undefined;
		this.itemsPerPage = undefined;
		this.selected = undefined;

		this.gridResizeHandlers = new Set();
		this.pageChangeHandlers = new Set();
		this.selectionChangeHandlers = new Set();
	}

	registerGridResizeHandler(handler)
	{
		this.gridResizeHandlers.add(handler);
	}

	registerPageChangeHandler(handler)
	{
		this.pageChangeHandlers.add(handler);
	}

	registerSelectionChangeHandler(handler)
	{
		this.selectionChangeHandlers.add(handler);
	}

	// Inheriting classes must subscribe to this event.
	onWindowResized()
	{
		this.resizeGrid();
		this.goToPageOfSelected();
	}

	setSelectedIndex(index)
	{
		this.selected = index;

		for (let handler of this.selectionChangeHandlers)
			handler();
	}

	goToPage(pageNumber)
	{
		if (!Number.isInteger(pageNumber))
			throw new Error("Given argument is not a number");

		this.currentPage = pageNumber;

		for (let handler of this.pageChangeHandlers)
			handler();
	}

	nextPage(wrapAround = true)
	{
		let numberPages = Math.max(1, this.pageCount);
		if (!wrapAround)
			this.goToPage(Math.min(this.currentPage + 1, numberPages - 1));
		else
			this.goToPage((this.currentPage + 1) % numberPages);
	}

	previousPage(wrapAround = true)
	{
		let numberPages = Math.max(1, this.pageCount);
		if (!wrapAround)
			this.goToPage(Math.max(this.currentPage - 1, 0));
		else
			this.goToPage((this.currentPage + numberPages - 1) % numberPages);
	}

	goToPageOfSelected()
	{
		this.goToPage(
			Math.max(Math.min(
				Math.floor(this.selected / this.itemsPerRow) - Math.floor(this.rowCount / 2),
				this.pageCount-1),
			0)
		);
	}

	increaseColumnCount(diff)
	{
		let isSelectedInPage =
			this.selected !== undefined &&
			Math.floor(this.selected / this.itemsPerRow) >= this.currentPage &&
			Math.floor(this.selected / this.itemsPerRow) < this.currentPage + this.rowCount;

		this.columnCount += diff;
		this.resizeGrid();

		if (isSelectedInPage)
			this.goToPageOfSelected();
		else
			this.goToPage(Math.min(this.currentPage, Math.max(0, this.pageCount - 1)));
	}

	resizeGrid()
	{
		let size = this.container.getComputedSize();
		let width = size.right - size.left;
		let height = size.bottom - size.top;

		let maxColumns = Math.floor(width / this.MinItemWidth);
		if (maxColumns <= 0)
			return;

		if (this.columnCount === undefined)
			this.columnCount = Math.floor(width / this.DefaultItemWidth);

		this.minColumns = Math.ceil(width / (height * this.ItemRatio));
		this.maxColumns = maxColumns;


		this.columnCount = Math.min(this.maxColumns, Math.max(this.minColumns, this.columnCount));

		this.itemWidth = Math.floor(width / this.columnCount);
		this.itemHeight = Math.floor(this.itemWidth / this.ItemRatio);

		this.rowCount = Math.floor((size.bottom - size.top) / this.itemHeight);
		this.itemsPerRow = Math.min(this.columnCount, this.items.length);
		this.itemsPerPage = Math.min(this.columnCount * this.rowCount, this.items.length);
		// NB: pages only change by one row, so items are in several pages.
		this.pageCount = Math.ceil(this.itemCount / this.itemsPerRow) - this.rowCount + 1;

		for (let handler of this.gridResizeHandlers)
			handler();
	}
}
