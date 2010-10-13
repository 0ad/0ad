const MAX_SELECTION_SIZE = 40; // Limits selection size and ensures that there will not be too many selection items in the GUI

function _setHighlight(ents, alpha)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetSelectionHighlight", { "entities":ents, "alpha":alpha });
}

function _setStatusBars(ents, enabled)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetStatusBars", { "entities":ents, "enabled":enabled });
}

function _setMotionOverlay(ents, enabled)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetMotionDebugOverlay", { "entities":ents, "enabled":enabled });
}

function _playSound(ent)
{
	Engine.GuiInterfaceCall("PlaySound", { "name":"select", "entity":ent });
}

//-------------------------------- -------------------------------- -------------------------------- 
// EntityGroups class for managing grouped entities
//-------------------------------- -------------------------------- -------------------------------- 
function EntityGroups()
{
	this.EntityGroup = function(name, templateName, ent)
	{
		this.name = name;
		this.templateName = templateName;
		this.ents = {ent : ent}; // the second element is stored as number
		this.count = 1;
	};

	this.EntityGroup.prototype.add = function(ent)
	{
		this.ents[ent] = ent;
		this.count++;
	};
	
	this.EntityGroup.prototype.remove= function(ent)
	{
		delete this.ents[ent];
		this.count--;
	};

	this.groups = {};
	this.ents = {};
}

EntityGroups.prototype.reset = function()
{
	this.groups = {};
	this.ents = {};
};

EntityGroups.prototype.add = function(ents)
{
	for each (var ent in ents)
	{
		if (!this.ents[ent])
		{
			var entState = GetEntityState(ent);
			var templateName = entState.template;
			var template = GetTemplateData(templateName);
			var name = template.name.specific || template.name.generic || "???";

			if (this.groups[name])
				this.groups[name].add(ent);
			else
				this.groups[name] = new this.EntityGroup(name, templateName, ent);
			
			this.ents[ent] = name;
		}
	}
};

EntityGroups.prototype.removeEnt = function(ent)
{
	var name = this.ents[ent];
	
	// Remove the entity
	delete this.ents[ent];
	this.groups[name].remove(ent);
	
	// Remove the entire group
	if (this.groups[name].count == 0)
		delete this.groups[name];
};

EntityGroups.prototype.getCount = function(name)
{
	return this.groups[name].count;
};

EntityGroups.prototype.getTemplateNames = function()
{
	var templateNames = [];
	for each (var group in this.groups)
		templateNames.push(group.templateName);
	return templateNames;
};

EntityGroups.prototype.getEntsByName = function(name)
{
	var ents = [];
	for each (var ent in this.groups[name].ents)
		ents.push(ent);
	return ents;
};

// Gets all ents in every group except ones of the specified group
EntityGroups.prototype.getEntsByNameInverse = function(name)
{
	var ents = [];
	for each (var group in this.groups)
		if (group.name != name)
			for each (var ent in group.ents)
				ents.push(ent);
	return ents;
};

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
	this.groups = new EntityGroups();
}

// Deselect everything but entities of the chosen type if the modifier is true otherwise deselect just the chosen entity
EntitySelection.prototype.makePrimarySelection = function(primaryTemplateName, modifierKey)
{
	var selection = this.toList();
	var ent;
	
	// Find an ent of a unit of the same type
	for (var i = 0; i < selection.length; i++)
	{
		var entState = GetEntityState(selection[i]);
		if (!entState)
			continue;
		if (entState.template == primaryTemplateName)
			ent = selection[i];
	}
	
	var primaryEntState = GetEntityState(ent);
	if (!primaryEntState)
		return;

	var primaryTemplate = GetTemplateData(primaryTemplateName);
	var primaryName = primaryTemplate.name.specific || primaryTemplate.name.generic || "???";

	var ents = [];
	if (modifierKey)
		ents = this.groups.getEntsByNameInverse(primaryName);
	else
		ents = this.groups.getEntsByName(primaryName);

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
		var entState = GetEntityState(ent);
		if (entState)
			templateNames.push(entState.template);
	}
	return templateNames;
}

// Update the selection to take care of changes (like units that have been killed)
EntitySelection.prototype.update = function()
{
	for each (var ent in this.selected)
	{
		var entState = GetEntityState(ent);

		// Remove deleted units
		if (!entState)
		{
			delete this.selected[ent];	
			this.groups.removeEnt(ent);
			this.dirty = true;
			continue;
		}

		// Remove non-visible units (e.g. moved back into fog-of-war)
		if (entState.visibility == "hidden")
		{
			// Disable any highlighting of the disappeared unit
			_setHighlight([ent], 0);
			_setStatusBars([ent], false);
			_setMotionOverlay([ent], false);

			delete this.selected[ent];
			this.dirty = true;
			continue;
		}
	}
};

EntitySelection.prototype.toggle = function(ent)
{
	if (this.selected[ent])
	{
		_setHighlight([ent], 0);
		_setStatusBars([ent], false);
		_setMotionOverlay([ent], false);
		delete this.selected[ent];
	}
	else
	{
		_setHighlight([ent], 1);
		_setStatusBars([ent], true);
		_setMotionOverlay([ent], this.motionDebugOverlay);
		this.selected[ent] = ent;
	}
	this.dirty = true;
};

EntitySelection.prototype.addList = function(ents)
{
	var selectionSize = this.toList().length;
	var i = 1;
	var added = [];

	for each (var ent in ents)
	{
		if (!this.selected[ent] && (selectionSize + i) <= MAX_SELECTION_SIZE)
		{
			added.push(ent);
			this.selected[ent] = ent;
			i++;
		}
	}

	_setHighlight(added, 1);
	_setStatusBars(added, true);
	_setMotionOverlay(added, this.motionDebugOverlay);
	if (added.length)
		_playSound(added[0]);

	this.groups.add(this.toList()); // Create Selection Groups
	this.dirty = true;
};

EntitySelection.prototype.reset = function()
{
	_setHighlight(this.toList(), 0);
	_setStatusBars(this.toList(), false);
	_setMotionOverlay(this.toList(), false);
	this.selected = {};
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
	var highlighted = {};
	for each (var ent in ents)
		highlighted[ent] = ent;

	var removed = [];
	var added = [];

	// Remove highlighting for the old units that are no longer highlighted
	// (excluding ones that are actively selected too)
	for each (var ent in this.highlighted)
		if (!highlighted[ent] && !this.selected[ent])
			removed.push(+ent);
	
	// Add new highlighting for units that aren't already highlighted
	for each (var ent in ents)
		if (!this.highlighted[ent] && !this.selected[ent])
			added.push(+ent);

	_setHighlight(removed, 0);
	_setStatusBars(removed, false);

	_setHighlight(added, 0.5);
	_setStatusBars(added, true);
	
	// Store the new highlight list
	this.highlighted = highlighted;
};

EntitySelection.prototype.SetMotionDebugOverlay = function(enabled)
{
	this.motionDebugOverlay = enabled;
	_setMotionOverlay(this.toList(), enabled);
};

var g_Selection = new EntitySelection();
