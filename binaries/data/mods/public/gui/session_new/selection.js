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
	// private properties
	this.primary = 0;
	this.groups = []; // Includes only highest ranked versions of each type
}

EntityGroups.prototype.Group = function(templateName, typeCount, firstOfType)
{
		this.templateName = templateName;
		this.typeCount = typeCount;
		this.firstOfType = firstOfType;
};

EntityGroups.prototype.reset = function()
{
	this.primary = 0;
	this.groups = [];
};

EntityGroups.prototype.getPrimary = function()
{
	return this.primary;
};

EntityGroups.prototype.setPrimary = function(index)
{
	if (this.groups.length > index)
		this.primary = index;
	else
		console.write("Warning: \"index\" is larger than g_Selection.toList().length: Cannot set Primary Selection.");
};

EntityGroups.prototype.getGroup = function(templateName)
{
	// Get the group that corresponds to the template name
	var group = this.groups[this.getGroupNumber(templateName)];
	if (group)
		return group;

	// The templateName didn't match any in the groups...
	// Check if it's the same unit, but a different rank and return that group
	
	if (g_GroupSelectionByRank)
	{
		var thatGenericTemplateName = templateName.substring(0, templateName.length-2);
		var templateNames = this.getTemplateNames();

		for (var i = 0; i < templateNames.length; i++)
		{
			var thisGenericTemplateName = templateNames[i].substring(0, templateNames[i].length-2);
			if (thisGenericTemplateName == thatGenericTemplateName)
				return this.groups[i];
		}
	}

	// There was no match...
	//console.write("Warning: Could not find either \"" + templateName + "\" or the more generic: \"" + thatGenericTemplateName + "\"");
	return undefined;
};

EntityGroups.prototype.addGroup = function(templateName, typeCount, firstOfType)
{
	this.groups.push(new this.Group(templateName, typeCount, firstOfType));
};

EntityGroups.prototype.removeGroup = function(templateName)
{
	var index = this.getGroupNumber(templateName);
	this.groups.splice(index, 1);
};

EntityGroups.prototype.getGroupNumber = function(templateName)
{
	for (var i = 0; i < this.groups.length; i++)
		if (this.groups[i].templateName == templateName)
			return i;
	
	return -1;
};

EntityGroups.prototype.getTemplateNames = function()
{
	var templateNames = [];
	
	for (var i = 0; i < this.groups.length; i++)
		templateNames.push(this.groups[i].templateName);
		
	return templateNames;
};

// Checks if the new rank code is greater than the old rank code (private helper function for EntityGroups.createGroups)
EntityGroups.prototype.greaterThanPreviousRank = function(oldRank, newRank)
{
	if (oldRank == newRank)
		return false;
	else if (oldRank == 'b' || newRank == 'e')
		return true;
	else
		return false;
};

EntityGroups.prototype.createGroups = function(ents)
{
	// Erase old groups first
	this.reset();

	// Make selection groups
	for (var i = 0; i < ents.length; i++)
	{
		var templateName = Engine.GuiInterfaceCall("GetEntityState", ents[i]).template;
		var group = this.getGroup(templateName);

		// We already have one of these types
		if (group)
		{
			// See if the new one has a higher rank
			var isRankableUnit = ((templateName.charAt(templateName.length-2) == '_')? true : false);

			if (g_GroupSelectionByRank && isRankableUnit)
			{
				var oldRank = group.templateName.charAt(group.templateName.length-1);
				var newRank = templateName.charAt(templateName.length-1);
				
				if (this.greaterThanPreviousRank(oldRank, newRank))
				{
					var oldTypeCount = group.typeCount;
					this.removeGroup(group.templateName);
					this.addGroup(templateName, oldTypeCount+1, i);
				}
				else
				{
					group.typeCount += 1;
				}
			}
			else // It was not a rankable unit or its rank was not higher than the one we had
			{
				group.typeCount += 1;
			}
		}
		else // Don't have any of this type, so add it in
		{
			this.addGroup(templateName, 1, i);
		}
	}

	resetCycleIndex();
}

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

EntitySelection.prototype.getPrimaryTemplateName = function()
{
	var entId = g_Selection.toList()[this.primary];
	var entState = Engine.GuiInterfaceCall("GetEntityState", entId);
	
	if (entState)
		return entState.template
		
	return undefined;
};

EntitySelection.prototype.getPrimary = function()
{
	return this.primary;
};

EntitySelection.prototype.setPrimary = function(index)
{
	if (g_Selection.toList().length > index)
		this.primary = index;
	else
		console.write("Warning: \"index\" is larger than g_Selection.toList().length: Cannot set Primary Selection.");
};

EntitySelection.prototype.resetPrimary = function() 
{
	this.primary = 0; // the primary selection must be reset whenever the selection changes
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
		this.groups.createGroups(this.toList());
		this.resetPrimary(); // TODO: should probably set this to a unit of the same type as the unit that was removed...
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
