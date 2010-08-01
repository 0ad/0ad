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

//-------------------------------- -------------------------------- -------------------------------- 
// EntitySelection class for managing the entity selection list and the primary selection
//-------------------------------- -------------------------------- -------------------------------- 
function EntitySelection()
{
	// Private properties:
	//--------------------------------
	this.selected = {}; // { id:id, id:id, ... } for each selected entity ID 'id'

	// { id:id, ... } for mouseover-highlighted entity IDs in these, the key is a string and the value is an int; 
	//	we want to use the int form wherever possible since it's more efficient to send to the simulation code)
	this.highlighted = {}; 

	this.motionDebugOverlay = false;

	// Public properties:
	//--------------------------------
	this.dirty = false; // set whenever the selection has changed
}

// Deselect everything but entities of the chosen type if the modifier is true
// otherwise deselect just the chosen entity
EntitySelection.prototype.makePrimarySelection = function(primaryIndex, modifierKey)
{
	var ents = [];
	var selection = this.toList();

	if (modifierKey)
	{
		var primaryEntState = Engine.GuiInterfaceCall("GetEntityState", selection[primaryIndex]);
		if (!primaryEntState)
			return;

		for (var i = 0; i < selection.length; i++)
		{
			var entState = Engine.GuiInterfaceCall("GetEntityState", selection[i]);
			if (!entState)
				return;
			if (entState.template == primaryEntState.template)
				ents.push(selection[i]);
		}
	}
	else
	{
		ents.push(selection[primaryIndex]);
	}
	this.reset();
	this.addList(ents);
}

// Get a list of the template names
EntitySelection.prototype.getTemplateNames = function()
{
	var templateNames = [];
	var ents = this.toList();
	
	for each (var ent in ents)
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", ent);
		if (entState)
			templateNames.push(entState.template);
	}
	return templateNames;
}

// Update the selection to take care of changes (like units that have been killed)
EntitySelection.prototype.update = function()
{
	var numberRemoved = 0;
	for each (var ent in this.selected)
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", ent);
		if (!entState)
		{
			delete this.selected[ent];
			numberRemoved++;
		}
	}
	if (numberRemoved > 0)
		this.dirty = true;
};

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
