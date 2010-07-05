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
// EntityGroups class for ordering / managing entities by their templates
//-------------------------------- -------------------------------- -------------------------------- 
function EntityGroups()
{
	this.primary = 0;
	this.groupNumbers = {};
	this.firstOfType = {}; // array for holding index to first appearance of a specific unit type in g_Selection 
	this.typeCount = {}; // format { name:count, name:count, ... } - maps units to the currently selected quantity of that type
	this.templates = [];
}

EntityGroups.prototype.reset = function()
{
	getGUIObjectByName("unitSelectionHighlight[" + g_Selection.groups.primary + "]").hidden = true;
	this.primary = 0;
	this.groupNumbers = {};
	this.firstOfType = {};
	this.typeCount = {};
	this.templates = [];
};

//-------------------------------- -------------------------------- -------------------------------- 
// EntitySelection class for managing the entity selection list and the primary selection
//-------------------------------- -------------------------------- -------------------------------- 
function EntitySelection()
{
	// Private properties:
	//--------------------------------
	this.primary = 0; // The active selection in the unit details panel
	this.selected = {}; // { id:id, id:id, ... } for each selected entity ID 'id'

	// { id:id, ... } for mouseover-highlighted entity IDs in these, the key is a string and the value is an int; 
	//	we want to use the int form wherever possible since it's more efficient to send to the simulation code)
	this.highlighted = {}; 

	this.motionDebugOverlay = false;

	// Public properties:
	//--------------------------------
	this.groups = new EntityGroups();  // the selection entity groups must be reset whenever the selection changes
	this.dirty = false; // set whenever the selection has changed
}

EntitySelection.prototype.getPrimary = function()
{
	return this.primary;
};

EntitySelection.prototype.setPrimary = function(index)
{
	if (g_Selection.toList().length > index)
		this.primary = index;
	else
		console.write("\"index\" is larger than g_Selection.toList().length: Cannot set Primary Selection.");
};

EntitySelection.prototype.resetPrimary = function() 
{
	this.primary = 0; // the primary selection must be reset whenever the selection changes
};

// Make the selection groups for the selection display buttons
EntitySelection.prototype.createSelectionGroups = function(ents)
{
	// Erase old selection first
	this.groups.reset();

	// Make selection groups
	var j = 0;
	for (var i = 0; i < ents.length; i++)
	{
		var template = Engine.GuiInterfaceCall("GetEntityState", ents[i]).template;		

		if (!this.groups.typeCount[template])
		{
			this.groups.typeCount[template] = 1;
			this.groups.firstOfType[template] = i;
			this.groups.templates.push(template);
			this.groups.groupNumbers[template] = j;
			j++;
		}
		else if (this.groups.typeCount[template])
		{
			this.groups.typeCount[template] += 1;
		}
	}
	
	getGUIObjectByName("unitSelectionHighlight[0]").hidden = false;
};

// Update the selection to take care of changes (like units that have been killed)
EntitySelection.prototype.updateSelection = function()
{
	var numberRemoved = 0;
	var i = 0;
	
	for each (var unit in this.selected)
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", unit);
		
		if (!entState)
		{
			delete this.selected[unit];
			numberRemoved++;
		}

		i++;
	}

	if (numberRemoved > 0)
	{
		this.dirty = true;
		this.createSelectionGroups(g_Selection.toList());
	}
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
	this.resetPrimary();
	this.groups.reset();
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
