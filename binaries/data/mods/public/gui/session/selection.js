// Limits selection size
var g_MaxSelectionSize = 200;

// Alpha value of hovered/mouseover/highlighted selection overlays
// (should probably be greater than always visible alpha value,
//	see CCmpSelectable)
var g_HighlightedAlpha = 0.75;

function _setHighlight(ents, alpha, selected)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetSelectionHighlight", { "entities": ents, "alpha": alpha, "selected": selected });
}

function _setStatusBars(ents, enabled)
{
	if (!ents.length)
		return;
	Engine.GuiInterfaceCall("SetStatusBars", {
		"entities": ents,
		"enabled": enabled,
		"showRank": Engine.ConfigDB_GetValue("user", "gui.session.rankabovestatusbar") == "true",
		"showExperience": Engine.ConfigDB_GetValue("user", "gui.session.experiencestatusbar") == "true"
	});
}

function _setMotionOverlay(ents, enabled)
{
	if (ents.length)
		Engine.GuiInterfaceCall("SetMotionDebugOverlay", { "entities": ents, "enabled": enabled });
}

function _playSound(ent)
{
	Engine.GuiInterfaceCall("PlaySound", { "name": "select", "entity": ent });
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
		if (this.ents[ent])
			continue;
		var entState = GetEntityState(ent);

		// When this function is called during group rebuild, deleted
		// entities will not yet have been removed, so entities might
		// still be present in the group despite not existing.
		if (!entState)
			continue;

		var templateName = entState.template;
		var key = GetTemplateData(templateName).selectionGroupName || templateName;

		// Group the ents by player and template
		if (entState.player !== undefined)
			key = "p" + entState.player + "&" + key;

		if (this.groups[key])
			this.groups[key] += 1;
		else
			this.groups[key] = 1;

		this.ents[ent] = key;
	}
};

EntityGroups.prototype.removeEnt = function(ent)
{
	var key = this.ents[ent];

	// Remove the entity
	delete this.ents[ent];
	--this.groups[key];

	// Remove the entire group
	if (this.groups[key] == 0)
		delete this.groups[key];
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

EntityGroups.prototype.getCount = function(key)
{
	return this.groups[key];
};

EntityGroups.prototype.getTotalCount = function()
{
	let totalCount = 0;
	for (let key in this.groups)
		totalCount += this.groups[key];
	return totalCount;
};

EntityGroups.prototype.getKeys = function()
{
	// Preserve order even when shuffling units around
	// Can be optimized by moving the sorting elsewhere
	return Object.keys(this.groups).sort();
};

EntityGroups.prototype.getEntsByKey = function(key)
{
	var ents = [];
	for (var ent in this.ents)
		if (this.ents[ent] == key)
			ents.push(+ent);

	return ents;
};

/**
 * get a list of entities grouped by a key
 */
EntityGroups.prototype.getEntsGrouped = function()
{
	return this.getKeys().map(key => ({
		"ents": this.getEntsByKey(key),
		"key": key
	}));
};

/**
 * Gets all ents in every group except ones of the specified group
 */
EntityGroups.prototype.getEntsByKeyInverse = function(key)
{
	var ents = [];
	for (var ent in this.ents)
		if (this.ents[ent] != key)
			ents.push(+ent);

	return ents;
};

/**
 * EntitySelection class for managing the entity selection list and the primary selection
 */
function EntitySelection()
{
	// Private properties:
	this.selected = new Set();

	// For mouseover-highlighted entity IDs in these.
	this.highlighted = new Set();

	this.motionDebugOverlay = false;

	// Public properties:
	this.dirty = false; // set whenever the selection has changed
	this.groups = new EntityGroups();

	this.UpdateFormationSelectionBehaviour();
	registerConfigChangeHandler(changes => {
		if (changes.has("gui.session.selectformationasone"))
			this.UpdateFormationSelectionBehaviour();
	});
}

EntitySelection.prototype.UpdateFormationSelectionBehaviour = function()
{
	this.SelectFormationAsOne = Engine.ConfigDB_GetValue("user", "gui.session.selectformationasone") == "true";
}

/**
 * Deselect everything but entities of the chosen type.
 */
EntitySelection.prototype.makePrimarySelection = function(key)
{
	const ents = this.groups.getEntsByKey(key);
	this.reset();
	this.addList(ents, false, false, false);
};

/**
 * Deselect entities of the chosen type.
 */
EntitySelection.prototype.removeGroupFromSelection = function(key)
{
	this.removeList(this.groups.getEntsByKey(key));
};

/**
 * Get a list of the template names
 */
EntitySelection.prototype.getTemplateNames = function()
{
	const templateNames = [];
	for (const ent of this.selected)
	{
		const entState = GetEntityState(ent);
		if (entState)
			templateNames.push(entState.template);
	}
	return templateNames;
};

/**
 * Update the selection to take care of changes (like units that have been killed).
 */
EntitySelection.prototype.update = function()
{
	this.checkRenamedEntities();

	const controlsAll = g_SimState.players[g_ViewedPlayer] && g_SimState.players[g_ViewedPlayer].controlsAll;
	const removeOwnerChanges = !g_IsObserver && !controlsAll && this.selected.size > 1;

	let changed = false;

	for (const ent of this.selected)
	{
		const entState = GetEntityState(ent);

		if (!entState)
		{
			this.selected.delete(ent);
			this.groups.removeEnt(ent);
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
			_setHighlight([ent], 0, false);
			_setStatusBars([ent], false);
			_setMotionOverlay([ent], false);

			this.selected.delete(ent);
			this.groups.removeEnt(ent);
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
			if (this.selected.has(renamedEntity.entity))
			{
				this.rebuildSelection(renamedLookup);
				return;
			}
	}
};

/**
 * Add entities to selection. Play selection sound unless quiet is true
 */
EntitySelection.prototype.addList = function(ents, quiet, force = false, addFormationMembers = true)
{
	// If someone else's player is the sole selected unit, don't allow adding to the selection.
	const firstEntState = this.selected.size == 1 && GetEntityState(this.getFirstSelected());
	if (firstEntState && firstEntState.player != g_ViewedPlayer && !force)
		return;

	const added = [];

	for (const ent of addFormationMembers ? this.addFormationMembers(ents) : ents)
	{
		if (this.selected.size >= g_MaxSelectionSize)
			break;

		if (this.selected.has(ent))
			continue;

		const entState = GetEntityState(ent);
		if (!entState)
			continue;

		let isUnowned = g_ViewedPlayer != -1 && entState.player != g_ViewedPlayer ||
		                g_ViewedPlayer == -1 && entState.player == 0;

		// Don't add unowned entities to the list, unless a single entity was selected
		if (isUnowned && (ents.length > 1 || this.selected.size) && !force)
			continue;

		added.push(ent);
		this.selected.add(ent);
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

/**
 * @param {number[]} ents - The entities to remove.
 * @param {boolean} addFormationMembers - If true we need to add formation members.
 */
EntitySelection.prototype.removeList = function(ents, addFormationMembers = true)
{
	const removed = [];

	for (const ent of addFormationMembers ? this.addFormationMembers(ents) : ents)
		if (this.selected.has(ent))
		{
			this.groups.removeEnt(ent);
			removed.push(ent);
			this.selected.delete(ent);
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
	this.selected.clear();
	this.groups.reset();
	this.onChange();
};

EntitySelection.prototype.rebuildSelection = function(renamed)
{
	const toAdd = [];
	for (const ent of this.selected)
		toAdd.push(renamed[ent] || ent);
	this.reset();

	this.addList(toAdd, true); // don't play selection sounds
};

EntitySelection.prototype.getFirstSelected = function()
{
	for (const ent of this.selected)
		return ent;
	return undefined;
};

/**
 * TODO: This array should not be recreated every call
 */
EntitySelection.prototype.toList = function()
{
	return Array.from(this.selected);
};

/**
 * @return {number} - The number of entities selected.
 */
EntitySelection.prototype.size = function()
{
	return this.selected.size;
};

EntitySelection.prototype.find = function(condition)
{
	for (const ent of this.selected)
		if (condition(ent))
			return ent;
	return null;
};

/**
 * @param {function} condition - A function.
 * @return {number[]} - The entities passing the condition.
 */
EntitySelection.prototype.filter = function(condition)
{
	const result = [];
	for (const ent of this.selected)
		if (condition(ent))
			result.push(ent);
	return result;
};

EntitySelection.prototype.setHighlightList = function(entities)
{
	const highlighted = new Set();
	const ents = this.addFormationMembers(entities);
	for (const ent of ents)
		highlighted.add(ent);

	const removed = [];
	const added = [];

	// Remove highlighting for the old units that are no longer highlighted
	// (excluding ones that are actively selected too).
	for (const ent of this.highlighted)
		if (!highlighted.has(ent) && !this.selected.has(ent))
			removed.push(ent);

	// Add new highlighting for units that aren't already highlighted.
	for (const ent of ents)
		if (!this.highlighted.has(ent) && !this.selected.has(ent))
			added.push(ent);

	_setHighlight(removed, 0, false);
	_setStatusBars(removed, false);

	_setHighlight(added, g_HighlightedAlpha, true);
	_setStatusBars(added, true);

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

EntitySelection.prototype.selectAndMoveTo = function(entityID)
{
	let entState = GetEntityState(entityID);
	if (!entState || !entState.position)
		return;

	this.reset();
	this.addList([entityID]);

	Engine.CameraMoveTo(entState.position.x, entState.position.z);
}

/**
 * Adds the formation members of a selected entities to the selection.
 * @param {number[]} entities - The entity IDs of selected entities.
 * @return {number[]} - Some more entity IDs if part of a formation was selected.
 */
EntitySelection.prototype.addFormationMembers = function(entities)
{
	if (!entities.length || !this.SelectFormationAsOne || Engine.HotkeyIsPressed("selection.singleselection"))
		return entities;

	const result = new Set(entities);
	for (const entity of entities)
	{
		const entState = GetEntityState(+entity);
		if (entState?.unitAI?.formation)
			for (const member of GetEntityState(+entState.unitAI.formation).formation.members)
				result.add(member);
	}

	return result;
};

/**
 * Cache some quantities which depends only on selection
 */

var g_Selection = new EntitySelection();
g_Selection.isSelection = true;

var g_canMoveIntoFormation = {};
var g_allBuildableEntities;
var g_allTrainableEntities;

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

/**
 * Add entities to a group.
 * @param {string} groupName - The number of the group to add the entities to.
 * @param {number[]} ents - The entities to add to the group.
 */
EntityGroupsContainer.prototype.addEntities = function(groupName, ents)
{
	if (Engine.ConfigDB_GetValue("user", "gui.session.disjointcontrolgroups") == "true")
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
				// Reconstruct the group if at least one entity has been renamed.
				if (renamedEntity.entity in group.ents)
				{
					group.rebuildGroup(renamedLookup);
					break;
				}
	}
};

var g_Groups = new EntityGroupsContainer();
