var brushShapes = {
	'circle': {
		width: function (size) { return size },
		height: function (size) { return size },
		data: function (size) {
			var data = [];
			// All calculations are done in units of half-tiles, since that
			// is the required precision
			var mid_x = size-1;
			var mid_y = size-1;
			var scale = 1 / (Math.sqrt(2) - 1);
			for (var y = 0; y < size; ++y)
			{
				for (var x = 0; x < size; ++x)
				{
					var dist_sq = // scaled to 0 in centre, 1 on edge
					    ((2*x - mid_x)*(2*x - mid_x) +
					     (2*y - mid_y)*(2*y - mid_y)) / (size*size);
					if (dist_sq <= 1)
						data.push((Math.sqrt(2 - dist_sq) - 1) * scale);
					else
						data.push(0);
				}
			}
			return data;
		}
	},

	'square': {
		width: function (size) { return size },
		height: function (size) { return size },
		data: function (size) {
			var data = [];
			for (var i = 0; i < size*size; ++i)
				data.push(1);
			return data;
		}
	}
};

var brush = {
	shape: brushShapes['circle'],
	size: 4,
	strength: 1.0,
	active: false,
	send: function () {
		Atlas.Message.Brush(
			this.shape.width(this.size),
			this.shape.height(this.size),
			this.shape.data(this.size)
		);
		// TODO: rather than this hack to make things interact correctly with C++ tools,
		// implement the tools in JS and do something better
		Atlas.SetBrushStrength(this.strength);
	}
};

var lastTerrainSelection = null; // button that was last selected, so we can undo its colouring
function onTerrainSelect()
{
	Atlas.SetSelectedTexture(this.terrainName);
	if (lastTerrainSelection)
		lastTerrainSelection.backgroundColour = wxNullColour;
	this.backgroundColour = new wxColour(255, 255, 0);
	lastTerrainSelection = this;
}

function TerrainPreviewPage(panel, name)
{
	this.panel = panel;
	this.name = name;

	// Size of texture preview images
	this.w = 120
	this.h = 40;

	this.previewReloadTimer = null;
}

TerrainPreviewPage.prototype = {
	reloadPreviews: function() {
		this.panel.freeze();
		this.scrolled.destroyChildren();
		this.itemSizer.clear();

		lastTerrainSelection = null; // clear any reference to deleted window

		// TODO: Do something clever like load the preview images asynchronously,
		// to avoid the annoying freeze when switching tabs
		var previews = Atlas.Message.GetTerrainGroupPreviews(this.name, this.w, this.h).previews;
		var i = 0;
		var names = [];
		var allLoaded = true;
		for each (var p in previews)
		{
			if (!p.loaded)
				allLoaded = false;

			// Create a wrapped-text label (replacing '_' with ' ' so there are more wrapping opportunities)
			var labelText = p.name.replace(/_/g, ' ');
			var label = new wxStaticText(this.scrolled, -1, labelText, wxDefaultPosition, wxDefaultSize, wxStaticText.ALIGN_CENTER);
			label.wrap(this.w);
			
			var imgSizer = new wxBoxSizer(wxOrientation.VERTICAL);
			var button = new wxBitmapButton(this.scrolled, -1, p.imagedata);
			button.terrainName = p.name;
			button.onClicked = onTerrainSelect;
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

	display: function() {
		if (this.loaded)
			return;
		
		this.panel.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
		var scrolled = new wxScrolledWindow(this.panel, -1, wxDefaultPosition, wxDefaultSize, wxWindow.VSCROLL);
		scrolled.setScrollRate(0, 10);
		scrolled.backgroundColour = new wxColour(255, 255, 255);
		this.panel.sizer.add(scrolled, 1, wxStretch.EXPAND);
		
		var itemSizer = new wxGridSizer(6, 4, 0);
		scrolled.sizer = itemSizer;
		
		// Adjust the number of columns to fit in the available area
		var w = this.w;
		scrolled.onSize = function (evt) {
			var numCols = Math.max(1, Math.floor(evt.size.width / (w+16)));
			if (itemSizer.cols != numCols)
				itemSizer.cols = numCols;
		};
		
		this.scrolled = scrolled;
		this.itemSizer = itemSizer;
		this.reloadPreviews();

		// TODO: fix keyboard navigation of the terrain previews

		this.loaded = true;
	}
};

function init(window, bottomWindow)
{
	window.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
		
	var tools = [
		{ label: 'Modify', name: 'AlterElevation' },
		{ label: 'Smooth', name: 'SmoothElevation' },
		{ label: 'Flatten', name: 'FlattenElevation' },
		{ label: 'Paint', name: 'PaintTerrain' },
	];
	var selectedTool = null; // null if none selected, else an element of 'tools'

	var toolSizer = new wxStaticBoxSizer(new wxStaticBox(window, -1, 'Elevation tools'), wxOrientation.HORIZONTAL);
	window.sizer.add(toolSizer, 0, wxStretch.EXPAND);
	for each (var tool in tools)
	{
		var button = new wxButton(window, -1, tool.label);
		toolSizer.add(button, 1);
		tool.button = button;
		
		// Explicitly set the background to the default colour, so that the button
		// is always owner-drawn (by the wxButton code), rather than initially using the
		// native (standard colour) button appearance then changing inconsistently later.
		button.backgroundColour = wxSystemSettings.getColour(wxSystemSettings.COLOUR_BTNFACE);
		
		(function(tool) { // (local scope)
			button.onClicked = function () {
				if (selectedTool == tool)
				{
					// Clicking on one tool twice should disable it
					selectedTool = null;
					this.backgroundColour = wxSystemSettings.getColour(wxSystemSettings.COLOUR_BTNFACE);
					Atlas.SetCurrentTool('');
				}
				else
				{
					// Disable the old tool
					if (selectedTool)
						selectedTool.button.backgroundColour = wxSystemSettings.getColour(wxSystemSettings.COLOUR_BTNFACE);
					// Enable the new one
					selectedTool = tool;
					this.backgroundColour = new wxColour(0xEE, 0xCC, 0x55);
					Atlas.SetCurrentTool(tool.name);
					brush.send();
				}
			};
		})(tool);
		// TODO: Need to make this interact properly with Tools.cpp/RegisterToolButton so all the buttons are in sync
	}
	
	var brushSizer = new wxStaticBoxSizer(new wxStaticBox(window, -1, 'Brush'), wxOrientation.VERTICAL);
	window.sizer.add(brushSizer);
	
	var shapes = [
		[ 'Circle', brushShapes['circle'] ],
		[ 'Square', brushShapes['square'] ]
	];
	var shapeNames = [];
	for each (var s in shapes) shapeNames.push(s[0]);
	var shapeBox = new wxRadioBox(window, -1, 'Shape', wxDefaultPosition, wxDefaultSize, shapeNames, 0, wxRadioBox.SPECIFY_ROWS);
	brushSizer.add(shapeBox);
	shapeBox.onRadioBox = function(evt)
	{
		brush.shape = shapes[evt.integer][1];
		brush.send();
	};
	
	var brushSettingsSizer = new wxFlexGridSizer(2);
	
	brushSizer.add(brushSettingsSizer);
	brushSettingsSizer.add(new wxStaticText(window, -1, 'Size'), 0, wxAlignment.RIGHT);
	var sizeSpinner = new wxSpinCtrl(window, -1, 4, wxDefaultPosition, wxDefaultSize, wxSpinCtrl.ARROW_KEYS, 1, 100);
	brushSettingsSizer.add(sizeSpinner);
	sizeSpinner.onSpinCtrl = function(evt)
	{
		brush.size = evt.position;
		brush.send();
	};
	
	brushSettingsSizer.add(new wxStaticText(window, -1, 'Strength'), wxAlignment.RIGHT);
	var strengthSpinner = new wxSpinCtrl(window, -1, 10, wxDefaultPosition, wxDefaultSize, wxSpinCtrl.ARROW_KEYS, 1, 100);
	brushSettingsSizer.add(strengthSpinner);
	strengthSpinner.onSpinCtrl = function(evt)
	{
		brush.strength = evt.position / 10;
		brush.send();
	};
	


	var visualiseSizer = new wxStaticBoxSizer(new wxStaticBox(window, -1, 'Visualise'), wxOrientation.VERTICAL);
	window.sizer.add(visualiseSizer);
	var visualiseSettingsSizer = new wxFlexGridSizer(2);
	visualiseSizer.add(visualiseSettingsSizer);

	visualiseSettingsSizer.add(new wxStaticText(window, -1, 'Passability'), 0, wxAlignment.RIGHT);
	var passabilityClasses = Atlas.Message.GetTerrainPassabilityClasses().classnames;
	var passabilitySelector = new wxChoice(window, -1, wxDefaultPosition, wxDefaultSize,
		["(none)"].concat(passabilityClasses)
	);
	passabilitySelector.selection = 0;
	visualiseSettingsSizer.add(passabilitySelector);
	passabilitySelector.onChoice = function (evt) {
		if (evt.selection == 0)
			Atlas.Message.SetViewParamS(Atlas.RenderView.GAME, "passability", "");
		else
			Atlas.Message.SetViewParamS(Atlas.RenderView.GAME, "passability", evt.string);
	};
	
	visualiseSettingsSizer.add(new wxStaticText(window, -1, 'Priorities'), 0, wxAlignment.RIGHT);
	var passabilityClasses = Atlas.Message.GetTerrainPassabilityClasses().classnames;
	var priorityCheckbox = new wxCheckBox(window, -1, "");
	visualiseSettingsSizer.add(priorityCheckbox);
	priorityCheckbox.onCheckBox = function (evt) {
		Atlas.Message.SetViewParamB(Atlas.RenderView.GAME, "priorities", evt.checked);
	};


	var terrainGroups = Atlas.Message.GetTerrainGroups();
	var nb = new wxNotebook(bottomWindow, -1);
	bottomWindow.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	bottomWindow.sizer.add(nb, 1, wxStretch.EXPAND);

	var pages = [];
	nb.onPageChanged = function (evt) {
		pages[evt.selection].display()
		evt.skip = true;
	}
	for each (var groupname in terrainGroups.groupnames)
	{
		var panel = new wxPanel(nb, -1);
		var page = new TerrainPreviewPage(panel, groupname);
		pages.push(page);
		nb.addPage(panel, groupname); // TODO: use Titlecase letters
	}
}
