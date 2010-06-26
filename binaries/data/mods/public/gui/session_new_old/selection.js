var g_ActiveSelectionColour    = { r:1, g:1, b:1, a:1 };
var g_HighlightSelectionColour = { r:1, g:1, b:1, a:0.5 };
var g_InactiveSelectionColour  = { r:1, g:1, b:1, a:0 };

function _setHighlight(ents, colour)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetSelectionHighlight", { "entities":ents, "colour":colour });
}

function _setMotionOverlay(ents, enabled)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetMotionDebugOverlay", { "entities":ents, "enabled":enabled });
}

function EntitySelection()
{
	// Private properties:
	this.selected = {}; // { id:id, id:id, ... } for each selected entity ID 'id'
	this.highlighted = {}; // { id:id, ... } for mouseover-highlighted entity IDs
		// (in these, the key is a string and the value is an int; we want to use the
		// int form wherever possible since it's more efficient to send to the simulation code)
	this.motionDebugOverlay = false;

	// Public properties:
	this.dirty = false; // set whenever the selection has changed
}

EntitySelection.prototype.toggle = function(ent)
{
	if (this.selected[ent])
	{
		_setHighlight([ent], g_InactiveSelectionColour);
		_setMotionOverlay([ent], false);
		delete this.selected[ent];
	}
	else
	{
		_setHighlight([ent], g_ActiveSelectionColour);
		_setMotionOverlay([ent], this.motionDebugOverlay);
		this.selected[ent] = ent;
	}
	this.dirty = true;
};

EntitySelection.prototype.addList = function(ents)
{
	var added = [];
	for each (var ent in ents)
	{
		if (!this.selected[ent])
		{
			added.push(ent);
			this.selected[ent] = ent;
		}
	}
	_setHighlight(added, g_ActiveSelectionColour);
	_setMotionOverlay(added, this.motionDebugOverlay);
	this.dirty = true;
};

EntitySelection.prototype.reset = function()
{
	_setHighlight(this.toList(), g_InactiveSelectionColour);
	_setMotionOverlay(this.toList(), false);
	this.selected = {};
	this.dirty = true;
};

EntitySelection.prototype.toList = function()
{
	var ents = [];
	for each (var ent in this.selected)
		ents.push(ent);
	return ents;
};

EntitySelection.prototype.setHighlightList = function(ents)
{
	var removed = [];
	var added = [];

	// Remove highlighting for the old units (excluding ones that are actively selected too)
	for each (var ent in this.highlighted)
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
		this.highlighted[ent] = ent;
};

EntitySelection.prototype.SetMotionDebugOverlay = function(enabled)
{
	this.motionDebugOverlay = enabled;
	_setMotionOverlay(this.toList(), enabled);
};

var g_Selection = new EntitySelection();
