//////////////////////////////////////////////////////////////////////////////
// Terrain preview page
//
// Used by new dialog and terrain panel

global.TerrainPreviewPage = function(panel, name, width, height)
{
	this.panel = panel;
	this.name = name;

	// Size of texture preview images
	this.w = width ? width : 120
	this.h = height ? height : 40;

	this.lastTerrainSelection = null; // button that was last selected, so we can undo its colouring
	this.previewReloadTimer = null;
}

global.TerrainPreviewPage.prototype = {
	reloadPreviews: function()
	{
		this.panel.freeze();
		this.scrolled.destroyChildren();
		this.itemSizer.clear();

		this.lastTerrainSelection = null; // clear any reference to deleted window

		var previews = Atlas.Message.GetTerrainGroupPreviews(this.name, this.w, this.h).previews;
		var i = 0;
		var names = [];
		var allLoaded = true;
		for each (var p in previews)
		{
			if (!p.loaded)
			{
				allLoaded = false;
			}

			// Create a wrapped-text label (replacing '_' with ' ' so there are more wrapping opportunities)
			var labelText = p.name.replace(/_/g, ' ');
			var label = new wxStaticText(this.scrolled, -1, labelText, wxDefaultPosition, wxDefaultSize, wxStaticText.ALIGN_CENTER);
			label.wrap(this.w);
			
			var imgSizer = new wxBoxSizer(wxOrientation.VERTICAL);
			var button = new wxBitmapButton(this.scrolled, -1, p.imagedata);
			var self = this;
			button.terrainName = p.name;
			button.onClicked = function()
				{
					Atlas.SetSelectedTexture(this.terrainName);
					if (self.lastTerrainSelection)
					{
						self.lastTerrainSelection.backgroundColour = wxNullColour;
					}
					this.backgroundColour = new wxColour(255, 255, 0);
					self.lastTerrainSelection = this;
				};
			imgSizer.add(button, 0, wxAlignment.CENTRE);
			imgSizer.add(label, 1, wxAlignment.CENTRE);
			this.itemSizer.add(imgSizer, 0, wxAlignment.CENTRE | wxStretch.EXPAND);
		}
		this.itemSizer.layout();
		this.panel.layout();
		this.panel.thaw();

		// If not all textures were loaded yet, run a timer to reload the previews
		// every so often until they've all finished
		if (allLoaded && this.previewReloadTimer)
		{
			this.previewReloadTimer.stop();
			this.previewReloadTimer = null;
		}
		else if (!allLoaded && !this.previewReloadTimer)
		{
			this.previewReloadTimer = new wxTimer();
			var self = this;
			this.previewReloadTimer.onNotify = function() { self.reloadPreviews(); };
			this.previewReloadTimer.start(2000);
		}
	},

	display: function() 
	{
		if (this.loaded)
		{
			return;
		}
		
		this.panel.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
		var scrolled = new wxScrolledWindow(this.panel, -1, wxDefaultPosition, wxDefaultSize, wxWindow.VSCROLL);
		scrolled.setScrollRate(0, 10);
		scrolled.backgroundColour = new wxColour(255, 255, 255);
		this.panel.sizer.add(scrolled, 1, wxStretch.EXPAND);
		
		var itemSizer = new wxGridSizer(6, 4, 0);
		scrolled.sizer = itemSizer;
		
		// Adjust the number of columns to fit in the available area
		var w = this.w;
		scrolled.onSize = function (evt)
			{
				var numCols = Math.max(1, Math.floor(evt.size.width / (w+16)));
				if (itemSizer.cols != numCols)
				{
					itemSizer.cols = numCols;
				}
			};
		
		this.scrolled = scrolled;
		this.itemSizer = itemSizer;
		this.reloadPreviews();

		// TODO: fix keyboard navigation of the terrain previews

		this.loaded = true;
	}
};

global.TerrainPreviewNotebook = function(window)
{
	var terrainGroups = Atlas.Message.GetTerrainGroups();
	var nb = new wxNotebook(window, -1);
	window.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	window.sizer.add(nb, 1, wxStretch.EXPAND);

	var pages = [];
	nb.onPageChanged = function (evt)
		{
			pages[evt.selection].display()
			evt.skip = true;
		}
	for each (var groupName in terrainGroups.groupNames)
	{
		var panel = new wxPanel(nb, -1);
		var page = new global.TerrainPreviewPage(panel, groupName);
		pages.push(page);
		nb.addPage(panel, groupName); // TODO: use Titlecase letters
	}
}
