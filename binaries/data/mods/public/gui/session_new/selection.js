var g_ActiveSelectionColour    = { r:1, g:1, b:1, a:1 };
var g_HighlightSelectionColour = { r:1, g:1, b:1, a:0.5 };
var g_InactiveSelectionColour  = { r:1, g:1, b:1, a:0 };

function _setHighlight(ents, colour)
{
	Engine.GuiInterfaceCall("SetSelectionHighlight", { "entities":ents, "colour":colour });
}

function EntitySelection()
{
	this.selected = {}; // { id: 1, id: 1, ... } for each selected entity ID 'id'
	this.highlighted = {}; // { id: 1, ... } for mouseover-highlighted entity IDs
}

EntitySelection.prototype.toggle = function(ent)
{
	if (this.selected[ent])
	{
		_setHighlight([ent], g_InactiveSelectionColour);
		delete this.selected[ent];
	}
	else
	{
		_setHighlight([ent], g_ActiveSelectionColour);
		this.selected[ent] = 1;
	}
};

EntitySelection.prototype.addList = function(ents)
{
	var added = [];
	for each (var ent in ents)
	{
		if (!this.selected[ent])
		{
			added.push(ent);
			this.selected[ent] = 1;
		}
	}
	_setHighlight(added, g_ActiveSelectionColour);
};

EntitySelection.prototype.reset = function()
{
	_setHighlight(this.toList(), g_InactiveSelectionColour);
	this.selected = {};
};

EntitySelection.prototype.toList = function()
{
	var ents = [];
	for (var ent in this.selected)
		ents.push(+ent); // convert from string to number and push
	return ents;
};

EntitySelection.prototype.setHighlightList = function(ents)
{
	var removed = [];
	var added = [];

	// Remove highlighting for the old units (excluding ones that are actively selected too)
	for (var ent in this.highlighted)
		if (!this.selected[ent])
			removed.push(ent);
	
	// Add new highlighting
	for each (var ent in ents)
		if (!this.selected[ent])
			added.push(ent);

	_setHighlight(removed, g_InactiveSelectionColour);
	_setHighlight(added, g_HighlightSelectionColour);

	// TODO: this could be a bit more efficient by only changing the ones that
	// have entered/left the highlight list
	
	// Store the new list
	this.highlighted = {};
	for each (var ent in ents)
		this.highlighted[ent] = 1;
};


var g_Selection = new EntitySelection();


