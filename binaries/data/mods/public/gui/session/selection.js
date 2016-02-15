// Limits selection size
const g_MaxSelectionSize = 200;

// Alpha value of hovered/mouseover/highlighted selection overlays
// (should probably be greater than always visible alpha value,
//	see CCmpSelectable)
const g_HighlightedAlpha = 0.75;

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
	for (let ent of ents)
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
	--this.groups[templateName];

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
};

EntityGroups.prototype.getCount = function(templateName)
{
	return this.groups[templateName];
};

EntityGroups.prototype.getTotalCount = function()
{
	let totalCount = 0;
	for (let templateName in this.groups)
		totalCount += this.groups[templateName];
	return totalCount;
};

EntityGroups.prototype.getTemplateNames = function()
{
	//Preserve order even when shuffling units around
	//Can be optimized by moving the sorting elsewhere
	return Object.keys(this.groups).sort();
};

EntityGroups.prototype.getEntsByName = function(templateName)
{
	var ents = [];
	for (var ent in this.ents)
		if (this.ents[ent] == templateName)
			ents.push(+ent);

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
		if (this.ents[ent] != templateName)
			ents.push(+ent);

	return ents;
};

/**
 * EntitySelection class for managing the entity selection list and the primary selection
 */
function EntitySelection()
{
	// Private properties:
	this.selected = {}; // { id:id, id:id, ... } for each selected entity ID 'id'

	// { id:id, ... } for mouseover-highlighted entity IDs in these, the key is a string and the value is an int;
	//	we want to use the int form wherever possible since it's more efficient to send to the simulation code)
	this.highlighted = {};

	this.motionDebugOverlay = false;

	// Public properties:
	this.dirty = false; // set whenever the selection has changed
	this.groups = new EntityGroups();
}

/**
 * Deselect everything but entities of the chosen type if the modifier is true otherwise deselect just the chosen entity
 */
EntitySelection.prototype.makePrimarySelection = function(templateName, modifierKey)
{
	var template = GetTemplateData(templateName);
	var key = template.selectionGroupName || templateName;

	var ents = [];
	if (modifierKey)
		ents = this.groups.getEntsByNameInverse(key);
	else
		ents = this.groups.getEntsByName(key);

	this.reset();
	this.addList(ents);
};

/**
 * Get a list of the template names
 */
EntitySelection.prototype.getTemplateNames = function()
{
	var templateNames = [];

	for (let ent in this.selected)
	{
		let entState = GetEntityState(+ent);
		if (entState)
			templateNames.push(entState.template);
	}
	return templateNames;
};

/**
 * Update the selection to take care of changes (like units that have been killed)
 */
EntitySelection.prototype.update = function()
{
	this.checkRenamedEntities();

	let changed = false;
	let removeOwnerChanges = !g_IsObserver && !g_DevSettings.controlAll && this.toList().length > 1;

	for (let ent in this.selected)
	{
		let entState = GetEntityState(+ent);

		// Remove deleted units
		if (!entState)
		{
			delete this.selected[ent];
			this.groups.removeEnt(+ent);
			changed = true;
			continue;
		}

		// Remove non-visible units (e.g. moved back into fog-of-war)
		// At the next update, mirages will be renamed to the real
		// entity they replace, so just ignore them now
		// Futhermore, when multiple selection, remove units which have changed ownership
		if (entState.visibility == "hidden" && !entState.mirage ||
			removeOwnerChanges && entState.player != g_ViewedPlayer)
		{
			// Disable any highlighting of the disappeared unit
			_setHighlight([+ent], 0, false);
			_setStatusBars([+ent], false);
			_setMotionOverlay([+ent], false);

			delete this.selected[ent];
			this.groups.removeEnt(+ent);
			changed = true;
			continue;
		}
	}
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
		for (let renamedEntity of renamedEntities)
			renamedLookup[renamedEntity.entity] = renamedEntity.newentity;

		// Reconstruct the selection if at least one entity has been renamed.
		for (let renamedEntity of renamedEntities)
		{
			if (this.selected[renamedEntity.entity])
			{
				this.rebuildSelection(renamedLookup);
				break;
			}
		}
	}
};

/**
 * Add entities to selection. Play selection sound unless quiet is true
 */
EntitySelection.prototype.addList = function(ents, quiet)
{
	let selection = this.toList();

	// If someone else's player is the sole selected unit, don't allow adding to the selection
	let firstEntState = selection.length == 1 && GetEntityState(selection[0]);
	if (firstEntState && !controlsPlayer(firstEntState.player))
		return;

	let i = 1;
	let added = [];

	for (let ent of ents)
	{
		if (selection.length + i > g_MaxSelectionSize)
			break;

		if (this.selected[ent])
			continue;

		var entState = GetEntityState(ent);
		if (!entState)
			continue;

		let isUnowned = g_ViewedPlayer != -1 && entState.player != g_ViewedPlayer ||
		                g_ViewedPlayer == -1 && entState.player == 0;

		// Don't add unowned entities to the list, unless a single entity was selected
		if (isUnowned && (ents.length > 1 || selection.length))
			continue;

		added.push(ent);
		this.selected[ent] = ent;
		++i;
	}

	_setHighlight(added, 1, true);
	_setStatusBars(added, true);
	_setMotionOverlay(added, this.motionDebugOverlay);
	if (added.length)
	{
		// Play the sound if the entity is controllable by us or Gaia-owned.
		var owner = GetEntityState(added[0]).player;
		if (!quiet && (controlsPlayer(owner) || g_IsObserver || owner == 0))
			_playSound(added[0]);
	}

	this.groups.add(this.toList()); // Create Selection Groups
	this.onChange();
};

EntitySelection.prototype.removeList = function(ents)
{
	var removed = [];

	for (let ent of ents)
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
	for (let ent in oldSelection)
		toAdd.push(renamed[ent] || +ent);

	this.addList(toAdd, true); // don't play selection sounds
};

EntitySelection.prototype.getFirstSelected = function()
{
	for (let ent in this.selected)
		return +ent;
	return undefined;
};

EntitySelection.prototype.toList = function()
{
	let ents = [];
	for (let ent in this.selected)
		ents.push(+ent);
	return ents;
};

EntitySelection.prototype.setHighlightList = function(ents)
{
	var highlighted = {};
	for (let ent of ents)
		highlighted[ent] = ent;

	var removed = [];
	var added = [];

	// Remove highlighting for the old units that are no longer highlighted
	// (excluding ones that are actively selected too)
	for (let ent in this.highlighted)
		if (!highlighted[ent] && !this.selected[ent])
			removed.push(+ent);

	// Add new highlighting for units that aren't already highlighted
	for (let ent of ents)
		if (!this.highlighted[ent] && !this.selected[ent])
			added.push(+ent);

	_setHighlight(removed, 0, false);
	_setStatusBars(removed, false);

	_setHighlight(added, g_HighlightedAlpha, true);
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
	if (this.isSelection)
		onSelectionChange();
};

/**
 * Cache some quantities which depends only on selection
 */

var g_Selection = new EntitySelection();
g_Selection.isSelection = true;

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
		this.groups[i] = new EntityGroups();
}

EntityGroupsContainer.prototype.addEntities = function(groupName, ents)
{
	for (let ent of ents)
		for (let group of this.groups)
			if (ent in group.ents)
				group.removeEnt(ent);

	this.groups[groupName].add(ents);
};

EntityGroupsContainer.prototype.update = function()
{
	this.checkRenamedEntities();
	for (let group of this.groups)
		for (var ent in group.ents)
		{
			var entState = GetEntityState(+ent);
			// Remove deleted units
			if (!entState)
				group.removeEnt(ent);
		}
};

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
		for (let renamedEntity of renamedEntities)
			renamedLookup[renamedEntity.entity] = renamedEntity.newentity;

		for (let group of this.groups)
			for (let renamedEntity of renamedEntities)
			{
				// Reconstruct the group if at least one entity has been renamed.
				if (renamedEntity.entity in group.ents)
				{
					group.rebuildGroup(renamedLookup);
					break;
				}
			}
	}
};

var g_Groups = new EntityGroupsContainer();
