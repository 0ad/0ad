/**
 * @file Contains all helper functions that are needed only for selection_panels.js
 * and some that are needed for hotkeys, but not for anything inside input.js.
 */

const UPGRADING_NOT_STARTED = -2;
const UPGRADING_CHOSEN_OTHER = -1;

function canMoveSelectionIntoFormation(formationTemplate)
{
	if (formationTemplate == NULL_FORMATION)
		return true;
	if (!(formationTemplate in g_canMoveIntoFormation))
		g_canMoveIntoFormation[formationTemplate] = Engine.GuiInterfaceCall("CanMoveEntsIntoFormation", {
			"ents": g_Selection.toList(),
			"formationTemplate": formationTemplate
		});

	return g_canMoveIntoFormation[formationTemplate];
}

function hasSameRestrictionCategory(templateName1, templateName2)
{
	let template1 = GetTemplateData(templateName1);
	let template2 = GetTemplateData(templateName2);

	if (template1.trainingRestrictions && template2.trainingRestrictions)
		return template1.trainingRestrictions.category == template2.trainingRestrictions.category;

	if (template1.buildRestrictions && template2.buildRestrictions)
		return template1.buildRestrictions.category == template2.buildRestrictions.category;

	return false;
}

/**
 * Returns a "color:255 0 0 Alpha" string based on how many resources are needed.
 */
function resourcesToAlphaMask(neededResources)
{
	let totalCost = 0;
	for (let resource in neededResources)
		totalCost += +neededResources[resource];

	return "color:255 0 0 " + Math.min(125, Math.round(+totalCost / 10) + 50);
}

function getStanceDisplayName(name)
{
	switch (name)
	{
	case "violent":
		return translateWithContext("stance", "Violent");
	case "aggressive":
		return translateWithContext("stance", "Aggressive");
	case "defensive":
		return translateWithContext("stance", "Defensive");
	case "passive":
		return translateWithContext("stance", "Passive");
	case "standground":
		return translateWithContext("stance", "Standground");
	default:
		warn("Internationalization: Unexpected stance found: " + name);
		return name;
	}
}

function getStanceTooltip(name)
{
	switch (name)
	{
	case "violent":
		return translateWithContext("stance", "Attack nearby opponents, focus on attackers and chase while visible");
	case "aggressive":
		return translateWithContext("stance", "Attack nearby opponents");
	case "defensive":
		return translateWithContext("stance", "Attack nearby opponents, chase a short distance and return to the original location");
	case "passive":
		return translateWithContext("stance", "Flee if attacked");
	case "standground":
		return translateWithContext("stance", "Attack opponents in range, but don't move");
	default:
		return "";
	}
}

/**
 * Format entity count/limit message for the tooltip
 */
function formatLimitString(trainEntLimit, trainEntCount, trainEntLimitChangers)
{
	if (trainEntLimit == undefined)
		return "";

	var text = sprintf(translate("Current Count: %(count)s, Limit: %(limit)s."), {
		"count": trainEntCount,
		"limit": trainEntLimit
	});

	if (trainEntCount >= trainEntLimit)
		text = coloredText(text, "red");

	for (var c in trainEntLimitChangers)
	{
		if (!trainEntLimitChangers[c])
			continue;

		let string = trainEntLimitChangers[c] > 0 ?
			translate("%(changer)s enlarges the limit with %(change)s.") :
			translate("%(changer)s lessens the limit with %(change)s.");

		text += "\n" + sprintf(string, {
			"changer": translate(c),
			"change": trainEntLimitChangers[c]
		});
	}
	return text;
}

/**
 * Format template match count/limit message for the tooltip.
 *
 * @param {number} matchEntLimit - The limit of the entity.
 * @param {number} matchEntCount - The count of the entity.
 * @param {string} type - The type of the action (i.e. "build" or "training").
 *
 * @return {string} - The string to show the user with information regarding the limit of this template.
 */
function formatMatchLimitString(matchEntLimit, matchEntCount, type)
{
	if (matchEntLimit == undefined)
		return "";

	let passedLimit = matchEntCount >= matchEntLimit;
	let count = matchEntLimit - matchEntCount;
	let text;
	if (type == "build")
	{
		if (matchEntLimit == 1)
			text = translate("Can be constructed only once.");
		else if (passedLimit)
			text = sprintf(translatePlural(
				"Could only be constructed %(limit)s time.",
				"Could only be constructed %(limit)s times.",
				matchEntLimit),
			{ "limit": matchEntLimit });
		else
			text = sprintf(translatePlural("Can be constructed %(count)s more time.", "Can be constructed %(count)s more times.", count), {
				"count": count
			});
	}
	else if (type == "training")
	{
		if (passedLimit)
			text = sprintf(translatePlural("Could only be trained once.", "Could only be trained %(limit)s times.", matchEntLimit), {
				"limit": matchEntLimit
			});
		else if (matchEntLimit == 1)
			text = translate("Can be trained only once.");
		else
			text = sprintf(translatePlural("Can be trained %(count)s more time.", "Can be trained %(count)s more times.", count), {
				"count": count
			});
	}
	else
	{
		if (passedLimit)
			text = sprintf(translatePlural("Could only be created once.", "Could only be created %(limit)s times.", matchEntLimit), {
				"limit": matchEntLimit
			});
		else if (matchEntLimit == 1)
			text = translate("Can be created only once.");
		else
			text = sprintf(translatePlural("Can be created %(count)s more time.", "Can be created %(count)s more times.", count), {
				"count": count
			});
	}

	return passedLimit ? coloredText(text, "red") : text;
}

/**
 * Format batch training string for the tooltip
 * Examples:
 * buildingsCountToTrainFullBatch = 1, fullBatchSize = 5, remainderBatch = 0:
 * "Shift-click to train 5"
 * buildingsCountToTrainFullBatch = 2, fullBatchSize = 5, remainderBatch = 0:
 * "Shift-click to train 10 (2*5)"
 * buildingsCountToTrainFullBatch = 1, fullBatchSize = 15, remainderBatch = 12:
 * "Shift-click to train 27 (15 + 12)"
 */
function formatBatchTrainingString(buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch)
{
	var totalBatchTrainingCount = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

	// Don't show the batch training tooltip if either units of this type can't be trained at all
	// or only one unit can be trained
	if (totalBatchTrainingCount < 2)
		return "";

	let fullBatchesString = "";
	if (buildingsCountToTrainFullBatch > 1)
		fullBatchesString = sprintf(translate("%(buildings)s*%(batchSize)s"), {
			"buildings": buildingsCountToTrainFullBatch,
			"batchSize": fullBatchSize
		});
	else if (buildingsCountToTrainFullBatch == 1)
		fullBatchesString = fullBatchSize;

	// We need to display the batch details part if there is either more than
	// one structure with full batch or one structure with the full batch and
	// another with a partial batch
	let batchString;
	if (buildingsCountToTrainFullBatch > 1 ||
	    buildingsCountToTrainFullBatch == 1 && remainderBatch > 0)
		if (remainderBatch > 0)
			batchString = translate("%(action)s to train %(number)s (%(fullBatch)s + %(remainderBatch)s).");
		else
			batchString = translate("%(action)s to train %(number)s (%(fullBatch)s).");
	else
		batchString = translate("%(action)s to train %(number)s.");

	return "[font=\"sans-13\"]" +
		setStringTags(
			sprintf(batchString, {
				"action": "[font=\"sans-bold-13\"]" + translate("Shift-click") + "[/font]",
				"number": totalBatchTrainingCount,
				"fullBatch": fullBatchesString,
				"remainderBatch": remainderBatch
			}),
			g_HotkeyTags) +
		"[/font]";
}

/**
 * Camera jumping: when the user presses a hotkey the current camera location is marked.
 * When pressing another camera jump hotkey the camera jumps back to that position.
 * When the camera is already roughly at that location, jump back to where it was previously.
 */
var g_JumpCameraPositions = [];
var g_JumpCameraLast;

function jumpCamera(index)
{
	let position = g_JumpCameraPositions[index];
	if (!position)
		return;

	let threshold = Engine.ConfigDB_GetValue("user", "gui.session.camerajump.threshold");
	let cameraPivot = Engine.GetCameraPivot();
	if (g_JumpCameraLast &&
	    Math.abs(cameraPivot.x - position.x) < threshold &&
	    Math.abs(cameraPivot.z - position.z) < threshold)
	{
		Engine.CameraMoveTo(g_JumpCameraLast.x, g_JumpCameraLast.z);
	}
	else
	{
		g_JumpCameraLast = cameraPivot;
		Engine.CameraMoveTo(position.x, position.z);
	}
}

function setJumpCamera(index)
{
	g_JumpCameraPositions[index] = Engine.GetCameraPivot();
}

/**
 * Called by GUI when user clicks a research button.
 */
function addResearchToQueue(entity, researchType)
{
	Engine.PostNetworkCommand({
		"type": "research",
		"entity": entity,
		"template": researchType,
		"pushFront": Engine.HotkeyIsPressed("session.pushorderfront")
	});
}

/**
 * Called by GUI when user clicks a production queue item.
 */
function removeFromProductionQueue(entity, id)
{
	Engine.PostNetworkCommand({
		"type": "stop-production",
		"entity": entity,
		"id": id
	});
}

/**
 * Called by unit selection buttons.
 */
function makePrimarySelectionGroup(templateName)
{
	g_Selection.makePrimarySelection(templateName);
}

function removeFromSelectionGroup(templateName)
{
	g_Selection.removeGroupFromSelection(templateName);
}

function performCommand(entStates, commandName)
{
	if (!entStates.length)
		return;

	if (getCommandInfo(commandName, entStates))
		g_EntityCommands[commandName].execute(entStates);
}

function performFormation(entities, formationTemplate)
{
	if (!entities)
		return;

	Engine.PostNetworkCommand({
		"type": "formation",
		"entities": entities,
		"formation": formationTemplate
	});
}

function performStance(entities, stanceName)
{
	if (!entities)
		return;

	Engine.PostNetworkCommand({
		"type": "stance",
		"entities": entities,
		"name": stanceName
	});
}

function lockGate(lock)
{
	Engine.PostNetworkCommand({
		"type": "lock-gate",
		"entities": g_Selection.toList(),
		"lock": lock
	});
}

function packUnit(pack)
{
	Engine.PostNetworkCommand({
		"type": "pack",
		"entities": g_Selection.toList(),
		"pack": pack,
		"queued": false
	});
}

function cancelPackUnit(pack)
{
	Engine.PostNetworkCommand({
		"type": "cancel-pack",
		"entities": g_Selection.toList(),
		"pack": pack,
		"queued": false
	});
}

function upgradeEntity(Template, selection)
{
	Engine.PostNetworkCommand({
		"type": "upgrade",
		"entities": selection,
		"template": Template,
		"queued": false
	});
}

function cancelUpgradeEntity()
{
	Engine.PostNetworkCommand({
		"type": "cancel-upgrade",
		"entities": g_Selection.toList(),
		"queued": false
	});
}

/**
 * Set the camera to follow the given entity if it's a unit.
 * Otherwise stop following.
 */
function setCameraFollow(entity)
{
	let entState = entity && GetEntityState(entity);
	if (entState && hasClass(entState, "Unit"))
		Engine.CameraFollow(entity);
	else
		Engine.CameraFollow(0);
}

function stopUnits(entities)
{
	Engine.PostNetworkCommand({
		"type": "stop",
		"entities": entities,
		"queued": false
	});
}

function unloadTemplate(template, owner)
{
	Engine.PostNetworkCommand({
		"type": "unload-template",
		"all": Engine.HotkeyIsPressed("session.unloadtype"),
		"template": template,
		"owner": owner,
		// Filter out all entities that aren't garrisonable.
		"garrisonHolders": g_Selection.filter(ent => {
			let state = GetEntityState(ent);
			return state && !!state.garrisonHolder;
		})
	});
}

function unloadAll()
{
	const garrisonHolders = g_Selection.filter(e => {
		let state = GetEntityState(e);
		return state && !!state.garrisonHolder;
	});

	if (!garrisonHolders.length)
		return;

	let ownEnts = [];
	let otherEnts = [];

	for (let ent of garrisonHolders)
	{
		if (controlsPlayer(GetEntityState(ent).player))
			ownEnts.push(ent);
		else
			otherEnts.push(ent);
	}

	if (ownEnts.length)
		Engine.PostNetworkCommand({
			"type": "unload-all",
			"garrisonHolders": ownEnts
		});

	if (otherEnts.length)
		Engine.PostNetworkCommand({
			"type": "unload-all-by-owner",
			"garrisonHolders": otherEnts
		});
}

function unloadAllTurrets()
{
	const turretHolders = g_Selection.filter(e => {
		let state = GetEntityState(e);
		return state && !!state.turretHolder;
	});

	if (!turretHolders.length)
		return;

	let ownedHolders = [];
	let ejectables = [];
	for (let ent of turretHolders)
	{
		let turretHolderState = GetEntityState(ent);
		if (controlsPlayer(turretHolderState.player))
			ownedHolders.push(ent);
		else
		{
			for (let turret of turretHolderState.turretHolder.turretPoints.map(tp => tp.entity))
				if (turret && controlsPlayer(GetEntityState(turret).player))
					ejectables.push(turret);
		}
	}

	if (ejectables.length)
		Engine.PostNetworkCommand({
			"type": "leave-turret",
			"entities": ejectables
		});

	if (ownedHolders.length)
		Engine.PostNetworkCommand({
			"type": "unload-turrets",
			"entities": ownedHolders
		});
}

function leaveTurretPoints()
{
	const entities = g_Selection.filter(entity => {
		let entState = GetEntityState(entity);
		return entState && entState.turretable &&
			entState.turretable.holder != INVALID_ENTITY;
	});

	Engine.PostNetworkCommand({
		"type": "leave-turret",
		"entities": entities
	});
}

function backToWork()
{
	Engine.PostNetworkCommand({
		"type": "back-to-work",
		// Filter out all entities that can't go back to work.
		"entities": g_Selection.filter(ent => {
			let state = GetEntityState(ent);
			return state && state.unitAI && state.unitAI.hasWorkOrders;
		})
	});
}

function removeGuard()
{
	Engine.PostNetworkCommand({
		"type": "remove-guard",
		// Filter out all entities that are currently guarding/escorting.
		"entities": g_Selection.filter(ent => {
			let state = GetEntityState(ent);
			return state && state.unitAI && state.unitAI.isGuarding;
		})
	});
}

function raiseAlert()
{
	Engine.PostNetworkCommand({
		"type": "alert-raise",
		"entities": g_Selection.filter(ent => {
			let state = GetEntityState(ent);
			return state && !!state.alertRaiser;
		})
	});
}

function endOfAlert()
{
	Engine.PostNetworkCommand({
		"type": "alert-end",
		"entities": g_Selection.filter(ent => {
			let state = GetEntityState(ent);
			return state && !!state.alertRaiser;
		})
	});
}

function turnAutoQueueOn()
{
	Engine.PostNetworkCommand({
		"type": "autoqueue-on",
		"entities": g_Selection.filter(ent => {
			let state = GetEntityState(ent);
			return !!state?.trainer?.entities?.length &&
				!state.production.autoqueue;
		})
	});
}

function turnAutoQueueOff()
{
	Engine.PostNetworkCommand({
		"type": "autoqueue-off",
		"entities": g_Selection.filter(ent => {
			let state = GetEntityState(ent);
			return !!state?.trainer?.entities?.length &&
				state.production.autoqueue;
		})
	});
}
