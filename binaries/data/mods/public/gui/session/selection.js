// Limits selection size
const MAX_SELECTION_SIZE = 200; 

// Alpha value of hovered/mouseover/highlighted selection overlays
// (should probably be greater than always visible alpha value,
//	see CCmpSelectable)
const HIGHLIGHTED_ALPHA = 0.75; 

function _setHighlight(ents, alpha, selected)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetSelectionHighlight", { "entities":ents, "alpha":alpha, "selected":selected });
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

/**
 * EntityGroups class for managing grouped entities
 */
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

			// When this function is called during group rebuild, deleted
			// entities will not yet have been removed, so entities might
			// still be present in the group despite not existing.
			if (!entState)
				continue;

			var templateName = entState.template;
			var key = GetTemplateData(templateName).selectionGroupName || templateName;

			// TODO ugly hack, just group them by player too.
			// Prefix garrisoned unit's selection name with the player they belong to
			var index = templateName.indexOf("&");
			if (index != -1 && key.indexOf("&") == -1)
				key = templateName.slice(0, index+1) + key;

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

EntityGroups.prototype.rebuildGroup = function(renamed)
{
	var oldGroup = this.ents;
	this.reset();

	var toAdd = [];
	for (var ent in oldGroup)
		toAdd.push(renamed[ent] ? renamed[ent] : +ent);

	this.add(toAdd);
}

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
	var ents = [];
	for (var ent in this.ents)
	{
		if (this.ents[ent] == templateName)
			ents.push(+ent);
	}

	return ents;
};

/**
 * get a list of entities grouped by templateName
 */
EntityGroups.prototype.getEntsGrouped = function()
{
	var templateNames = this.getTemplateNames();
	var list = [];
	for (var t of templateNames)
	{
		list.push({
			"ents": this.getEntsByName(t),
			"template": t,
		});	
	}
	return list;
};

/**
 * Gets all ents in every group except ones of the specified group
 */
EntityGroups.prototype.getEntsByNameInverse = function(templateName)
{
	var ents = [];
	for (var ent in this.ents)
	{
		if (this.ents[ent] != templateName)
			ents.push(+ent);
	}

	return ents;
};

/**
 * EntitySelection class for managing the entity selection list and the primary selection
 */
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

/**
 * Deselect everything but entities of the chosen type if the modifier is true otherwise deselect just the chosen entity
 */
EntitySelection.prototype.makePrimarySelection = function(templateName, modifierKey)
{
	var selection = this.toList();
	
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

/**
 * Get a list of the template names
 */
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

/**
 * Update the selection to take care of changes (like units that have been killed)
 */
EntitySelection.prototype.update = function()
{
	this.checkRenamedEntities();

	var miraged = {};
	var changed = false;
	for each (var ent in this.selected)
	{
		var entState = GetEntityState(ent);

		// Remove deleted units
		if (!entState)
		{
			delete this.selected[ent];
			this.groups.removeEnt(ent);
			changed = true;
			continue;
		}

		// Manually replace newly miraged entities by their mirages
		if (entState.fogging && entState.fogging.mirage)
		{
			miraged[ent] = entState.fogging.mirage;
			continue;
		}

		// Remove non-visible units (e.g. moved back into fog-of-war)
		if (entState.visibility == "hidden")
		{
			// Disable any highlighting of the disappeared unit
			_setHighlight([ent], 0, false);
			_setStatusBars([ent], false);
			_setMotionOverlay([ent], false);

			delete this.selected[ent];
			this.groups.removeEnt(ent);
			changed = true;
			continue;
		}
	}

	this.rebuildSelection(miraged);

	if (changed)
		this.onChange();
};

/**
 * Update selection if some selected entities were renamed
 * (in case of unit promotion or finishing building structure)
 */
EntitySelection.prototype.checkRenamedEntities = function()
{
	var renamedEntities = Engine.GuiInterfaceCall("GetRenamedEntities");
	if (renamedEntities.length > 0)
	{
		var renamedLookup = {};
		for each (var renamedEntity in renamedEntities)
			renamedLookup[renamedEntity.entity] = renamedEntity.newentity;

		// Reconstruct the selection if at least one entity has been renamed.
		for each (var renamedEntity in renamedEntities)
		{
			if (this.selected[renamedEntity.entity])
			{
				this.rebuildSelection(renamedLookup);
				break;
			}
		}
	}
}

/**
 * Add entities to selection. Play selection sound unless quiet is true
 */
EntitySelection.prototype.addList = function(ents, quiet)
{
	var selection = this.toList();
	var playerID = Engine.GetPlayerID();

	// If someone else's player is the sole selected unit, don't allow adding to the selection
	if (!g_DevSettings.controlAll && selection.length == 1)
	{
		var firstEntState = GetEntityState(selection[0]);
		if (firstEntState && firstEntState.player != playerID)
			return;
	}

	// Allow selecting things not belong to this player (enemy, ally, gaia)
	var allowUnownedSelect = g_DevSettings.controlAll || (ents.length == 1 && selection.length == 0);

	var i = 1;
	var added = [];

	for each (var ent in ents)
	{
		// Only add entities we own to our selection
		var entState = GetEntityState(ent);
		if (!this.selected[ent] && (selection.length + i) <= MAX_SELECTION_SIZE && (allowUnownedSelect || (entState && entState.player == playerID)))
		{
			added.push(ent);
			this.selected[ent] = ent;
			i++;
		}
	}

	_setHighlight(added, 1, true);
	_setStatusBars(added, true);
	_setMotionOverlay(added, this.motionDebugOverlay);
	if (added.length)
	{
		// Play the sound if the entity is controllable by us or Gaia-owned.
		var owner = GetEntityState(added[0]).player;
		if (!quiet && (owner == playerID || owner == 0 || g_DevSettings.controlAll))
			_playSound(added[0]);
	}

	this.groups.add(this.toList()); // Create Selection Groups
	this.onChange();
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

	_setHighlight(removed, 0, false);
	_setStatusBars(removed, false);
	_setMotionOverlay(removed, false);

	this.onChange();
};

EntitySelection.prototype.reset = function()
{
	_setHighlight(this.toList(), 0, false);
	_setStatusBars(this.toList(), false);
	_setMotionOverlay(this.toList(), false);
	this.selected = {};
	this.groups.reset();
	this.onChange();
};

EntitySelection.prototype.rebuildSelection = function(renamed)
{
	var oldSelection = this.selected;
	this.reset();

	var toAdd = [];
	for each (var ent in oldSelection)
		toAdd.push(renamed[ent] ? renamed[ent] : ent);

	this.addList(toAdd, true); // don't play selection sounds
}

EntitySelection.prototype.toList = function(){
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

	_setHighlight(removed, 0, false);
	_setStatusBars(removed, false);

	_setHighlight(added, HIGHLIGHTED_ALPHA, true);
	_setStatusBars(added, true);
	
	// Store the new highlight list
	this.highlighted = highlighted;
};

EntitySelection.prototype.SetMotionDebugOverlay = function(enabled)
{
	this.motionDebugOverlay = enabled;
	_setMotionOverlay(this.toList(), enabled);
};

EntitySelection.prototype.onChange = function()
{
	this.dirty = true;
	if (this == g_Selection)
		onSelectionChange();
}

/**
 * Cache some quantities which depends only on selection
 */

var g_Selection = new EntitySelection();

var g_canMoveIntoFormation = {};
var g_allBuildableEntities = undefined;
var g_allTrainableEntities = undefined;

// Reset cached quantities
function onSelectionChange()
{
	g_canMoveIntoFormation = {};
	g_allBuildableEntities = undefined;
	g_allTrainableEntities = undefined;
}


/**
 * EntityGroupsContainer class for managing grouped entities
 */
function EntityGroupsContainer()
{
	this.groups = [];
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
	this.checkRenamedEntities();
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

/**
 * Update control group if some entities in the group were renamed
 * (in case of unit promotion or finishing building structure)
 */
EntityGroupsContainer.prototype.checkRenamedEntities = function()
{
	var renamedEntities = Engine.GuiInterfaceCall("GetRenamedEntities");
	if (renamedEntities.length > 0)
	{
		var renamedLookup = {};
		for each (var renamedEntity in renamedEntities)
			renamedLookup[renamedEntity.entity] = renamedEntity.newentity;

		for each (var group in this.groups)
		{
			for each (var renamedEntity in renamedEntities)
			{
				// Reconstruct the group if at least one entity has been renamed.
				if (renamedEntity.entity in group.ents)
				{
					group.rebuildGroup(renamedLookup);
					break;
				}
			}
		}
	}
}

var g_Groups = new EntityGroupsContainer();
