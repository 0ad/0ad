
function init(window)
{
	var terrainGroups = Atlas.Message.GetTerrainGroups();
	var nb = new wxNotebook(window, -1);
	window.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	window.sizer.add(nb, 1, wxStretch.EXPAND);

	var pages = [];
	nb.onPageChanged = function (evt) {
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
