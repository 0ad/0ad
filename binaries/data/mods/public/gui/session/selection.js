const MAX_SELECTION_SIZE = 64; // Limits selection size

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
			var key = template.selectionGroupName || templateName;
			
			if (this.groups[key])
				this.groups[key] += 1;
			else
				this.groups[key] = 1;

			this.ents[ent] = key;
		}
	}
};

EntityGroups.prototype.removeEnt = function(ent)
{
	var templateName = this.ents[ent];
	
	// Remove the entity
	delete this.ents[ent];
	this.groups[templateName]--;
	
	// Remove the entire group
	if (this.groups[templateName] == 0)
		delete this.groups[templateName];
};

EntityGroups.prototype.getCount = function(templateName)
{
	return this.groups[templateName];
};

EntityGroups.prototype.getTotalCount = function()
{
	var totalCount = 0;
	for each (var group in this.groups)
	{
		totalCount += group;
	}
	return totalCount;
};

EntityGroups.prototype.getTemplateNames = function()
{
	var templateNames = [];
	for (var templateName in this.groups)
		templateNames.push(templateName);
	//Preserve order even when shuffling units around
	//Can be optimized by moving the sorting elsewhere
	templateNames.sort();
	return templateNames;
};

EntityGroups.prototype.getEntsByName = function(templateName)
{
	var entTemplateNames = [];
	for each (var entTemplateName in this.ents)
		entTemplateNames.push(entTemplateName);
	
	var i = 0;
	var ents = [];
	for (var ent in this.ents)
	{
		if (entTemplateNames[i] == templateName)
			ents.push(parseInt(ent));
		i++;
	}

	return ents;
};

// Gets all ents in every group except ones of the specified group
EntityGroups.prototype.getEntsByNameInverse = function(templateName)
{
	var entTemplateNames = [];
	for each (var entTemplateName in this.ents)
		entTemplateNames.push(entTemplateName);
	
	var i = 0;
	var ents = [];
	for (var ent in this.ents)
	{
		if (entTemplateNames[i] != templateName)
			ents.push(parseInt(ent));
		i++;
	}

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
EntitySelection.prototype.makePrimarySelection = function(templateName, modifierKey)
{
	var selection = this.toList();
	var ent;

	// Find an ent of a unit of the same type
	for (var i = 0; i < selection.length; i++)
	{
		var entState = GetEntityState(selection[i]);
		if (!entState)
			continue;
		if (entState.template == templateName)
			ent = selection[i];
	}

	var template = GetTemplateData(templateName);
	var key = template.selectionGroupName || templateName;

	var ents = [];
	if (modifierKey)
		ents = this.groups.getEntsByNameInverse(key);
	else
		ents = this.groups.getEntsByName(key);

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
	this.checkRenamedEntities();
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

/**
 * Update selection if some selected entities was renamed
 * (in case of unit promotion or finishing building structure)
 */
EntitySelection.prototype.checkRenamedEntities = function()
{
	var renamedEntities = Engine.GuiInterfaceCall("GetRenamedEntities", true);
	if (renamedEntities.length > 0)
	{
		var removeFromSelectionList = [];
		var addToSelectionList = [];
		for each (var renamedEntity in renamedEntities)
		{
			if (this.selected[renamedEntity.entity])
			{
				removeFromSelectionList.push(renamedEntity.entity);
				addToSelectionList.push(renamedEntity.newentity);
			}
		}
		this.removeList(removeFromSelectionList);
		this.addList(addToSelectionList);
	}
}

EntitySelection.prototype.addList = function(ents)
{
	var selectionSize = this.toList().length;
	var i = 1;
	var added = [];
	var playerID = Engine.GetPlayerID();
	var allowEnemySelections = g_DevSettings.controlAll || (ents.length == 1 && selectionSize == 0);

	for each (var ent in ents)
	{
		// Only add entities we own to our selection
		var entState = GetEntityState(ent);
		if (!this.selected[ent] && (selectionSize + i) <= MAX_SELECTION_SIZE && (allowEnemySelections || (entState && entState.player == playerID)))
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

EntitySelection.prototype.removeList = function(ents)
{
	var removed = [];

	for each (var ent in ents)
	{
		if (this.selected[ent])
		{
			this.groups.removeEnt(ent);
			removed.push(ent);
			delete this.selected[ent];
		}
	}

	_setHighlight(removed, 0);
	_setStatusBars(removed, false);
	_setMotionOverlay(removed, false);

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

//-------------------------------- -------------------------------- --------------------------------
// EntityGroupsContainer class for managing grouped entities
//-------------------------------- -------------------------------- --------------------------------
function EntityGroupsContainer()
{
	this.groups = {};
	for (var i = 0; i < 10; ++i)
	{
		this.groups[i] = new EntityGroups();
	}
}

EntityGroupsContainer.prototype.addEntities = function(groupName, ents)
{
	for each (var ent in ents)
	{
		for each (var group in this.groups)
		{
			if (ent in group.ents)
			{
				group.removeEnt(ent);
			}
		}
	}
	this.groups[groupName].add(ents);
}

EntityGroupsContainer.prototype.update = function()
{
	for each (var group in this.groups)
	{
		for (var ent in group.ents)
		{
			var entState = GetEntityState(+ent);

			// Remove deleted units
			if (!entState)
			{
				group.removeEnt(ent);
			}
		}
	}
}

var g_Groups = new EntityGroupsContainer();
