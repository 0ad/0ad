// Panel types
const SELECTION = "Selection";
const QUEUE = "Queue";
const GARRISON = "Garrison";
const FORMATION = "Formation";
const TRAINING = "Training";
const RESEARCH = "Research";
const CONSTRUCTION = "Construction";
const COMMAND = "Command";
const STANCE = "Stance";
const GATE = "Gate";
const PACK = "Pack";

// Constants
const UNIT_PANEL_BASE = -52; // QUEUE: The offset above the main panel (will often be negative)
const UNIT_PANEL_HEIGHT = 44; // QUEUE: The height needed for a row of buttons

// Trading constants
const TRADING_RESOURCES = ["food", "wood", "stone", "metal"];

// Barter constants
const BARTER_RESOURCE_AMOUNT_TO_SELL = 100;
const BARTER_BUNCH_MULTIPLIER = 5;
const BARTER_RESOURCES = ["food", "wood", "stone", "metal"];
const BARTER_ACTIONS = ["Sell", "Buy"];

// Gate constants
const GATE_ACTIONS = ["lock", "unlock"];

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Formation": 0, "Garrison": 0, "Training": 0, "Research": 0, "Barter": 0, "Construction": 0, "Command": 0, "Stance": 0, "Gate": 0, "Pack": 0};

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Selection", "Queue", "Formation", "Garrison", "Training", "Barter", "Construction", "Research", "Stance", "Command", "Gate", "Pack"];

// Indexes of resources to sell and buy on barter panel
var g_barterSell = 0;

function setPanelObjectPosition(object, index, rowLength, vMargin = 1, hMargin = 1)
{
	var size = object.size;
	// horizontal position
	var oWidth = size.right - size.left;
	var hIndex = index % rowLength;
	size.left = hIndex * (oWidth + vMargin);
	size.right = size.left + oWidth;
	// vertical position
	var oHeight = size.bottom - size.top;
	var vIndex = Math.floor(index / rowLength);
	size.top = vIndex * (oHeight + hMargin);
	size.bottom = size.top + oHeight;
	object.size = size;
}

/**
 * Format entity count/limit message for the tooltip
 */
function formatLimitString(trainEntLimit, trainEntCount, trainEntLimitChangers)
{
	if (trainEntLimit == undefined)
		return "";
	var text = "\n\n" + sprintf(translate("Current Count: %(count)s, Limit: %(limit)s."), { count: trainEntCount, limit: trainEntLimit });
	if (trainEntCount >= trainEntLimit)
		text = "[color=\"red\"]" + text + "[/color]";
	for (var c in trainEntLimitChangers)
	{
		if (trainEntLimitChangers[c] > 0)
			text += "\n" + sprintf(translate("%(changer)s enlarges the limit with %(change)s."), { changer: translate(c), change: trainEntLimitChangers[c] });
		else if (trainEntLimitChangers[c] < 0)
			text += "\n" + sprintf(translate("%(changer)s lessens the limit with %(change)s."), { changer: translate(c), change: (-trainEntLimitChangers[c]) });
	}
	return text;
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
	var batchTrainingString = "";
	var fullBatchesString = "";
	if (buildingsCountToTrainFullBatch > 0)
	{
		if (buildingsCountToTrainFullBatch > 1)
			fullBatchesString = sprintf(translate("%(buildings)s*%(batchSize)s"), {
				buildings: buildingsCountToTrainFullBatch,
				batchSize: fullBatchSize
			});
		else
			fullBatchesString = fullBatchSize;
	}
	var remainderBatchString = remainderBatch > 0 ? remainderBatch : "";
	var batchDetailsString = "";
	var action = "[font=\"sans-bold-13\"]" + translate("Shift-click") + "[/font][font=\"sans-13\"]"

	// We need to display the batch details part if there is either more than
	// one building with full batch or one building with the full batch and
	// another with a partial batch
	if (buildingsCountToTrainFullBatch > 1 ||
		(buildingsCountToTrainFullBatch == 1 && remainderBatch > 0))
	{
		if (remainderBatch > 0)
			return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s (%(fullBatch)s + %(remainderBatch)s)."), {
				action: action,
				number: totalBatchTrainingCount,
				fullBatch: fullBatchesString,
				remainderBatch: remainderBatch
			}) + "[/font]";

		return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s (%(fullBatch)s)."), {
			action: action,
			number: totalBatchTrainingCount,
			fullBatch: fullBatchesString
		}) + "[/font]";
	}

	return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s."), {
		action: action,
		number: totalBatchTrainingCount
	}) + "[/font]";
}

function getStanceDisplayName(name)
{
	var displayName;
	switch(name)
	{
		case "violent":
			displayName = translateWithContext("stance", "Violent");
			break;
		case "aggressive":
			displayName = translateWithContext("stance", "Aggressive");
			break;
		case "passive":
			displayName = translateWithContext("stance", "Passive");
			break;
		case "defensive":
			displayName = translateWithContext("stance", "Defensive");
			break;
		case "standground":
			displayName = translateWithContext("stance", "Standground");
			break;
		default:
			warn(sprintf("Internationalization: Unexpected stance found with code ‘%(stance)s’. This stance must be internationalized.", { stance: name }));
			displayName = name;
			break;
	}
	return displayName;
}

/**
 * Helper function for updateUnitCommands; sets up "unit panels" (i.e. panels with rows of icons) for the currently selected
 * unit.
 *
 * @param guiName Short identifier string of this panel; see constants defined at the top of this file.
 * @param usedPanels Output object; usedPanels[guiName] will be set to 1 to indicate that this panel was used during this
 * 				run of updateUnitCommands and should not be hidden. TODO: why is this done this way instead of having
 * 				updateUnitCommands keep track of this?
 * @param unitEntState Entity state of the (first) selected unit.
 * @param items Panel-specific data to construct the icons with.
 * @param callback Callback function to argument to execute when an item's icon gets clicked. Takes a single 'item' argument.
 */
function setupUnitPanel(guiName, usedPanels, unitEntState, playerState, items, callback)
{
	if (!g_SelectionPanels[guiName])
	{
		error("unknown guiName used '" + guiName + "'");
		return;
	}
	usedPanels[guiName] = 1;

	var numberOfItems = items.length;
	var selection = g_Selection.toList();
	var garrisonGroups = new EntityGroups();

	// Determine how many buttons there should be
	if (g_SelectionPanels[guiName].maxNumberOfItems)
		numberOfItems = Math.min(g_SelectionPanels[guiName].maxNumberOfItems, numberOfItems);
	if (g_SelectionPanels[guiName].rowLength)
		var rowLength = g_SelectionPanels[guiName].rowLength;
	else
		var rowLength = 8;

	// TODO get this out of here
	// Common code for garrison and 'unload all' button counts.
	if (guiName == GARRISON || guiName == COMMAND)
	{
		for (var i = 0; i < selection.length; ++i)
		{
			var state = GetEntityState(selection[i]);
			if (state.garrisonHolder)
				garrisonGroups.add(state.garrisonHolder.entities)
		}
	}
	// Resize Queue panel if needed
	if (guiName == QUEUE) // or garrison
	{
		var numRows = Math.ceil(numberOfItems / rowLength);
		var panel = Engine.GetGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		size.top = (UNIT_PANEL_BASE - ((numRows-1)*UNIT_PANEL_HEIGHT));
		panel.size = size;
	}

	// Make buttons
	for (var i = 0; i < numberOfItems; i++)
	{
		var item = items[i];

		// If a tech has been researched it leaves an empty slot
		if (!item && g_SelectionPanels[guiName].hideItem)
		{
			g_SelectionPanels[guiName].hideItem(i, rowLength);
			continue;
		}

		// STANDARD DATA
		// add standard data 
		var data = {
			"i": i,
			"item": item,
			"selection": selection,
			"playerState": playerState,
			"unitEntState": unitEntState,
			"callback": callback,
			"rowLength": rowLength,
			"numberOfItems": numberOfItems,
		};

		if (garrisonGroups)
			data.garrisonGroups = garrisonGroups;

		// add standard gui objects to the data
		// depending on the actual XML, some of this may be undefined
		data.button = Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]");
		data.affordableMask = Engine.GetGUIObjectByName("unit"+guiName+"Unaffordable["+i+"]");
		data.icon = Engine.GetGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		data.guiSelection = Engine.GetGUIObjectByName("unit"+guiName+"Selection["+i+"]");
		data.countDisplay = Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]");


		// DEFAULTS
		data.button.hidden = false;
		data.button.enabled = true;
		data.button.tooltip = "";
		data.button.caption = "";

		// Items can have a callback element that overrides the normal 
		// caller-supplied callback function. Button Function 
		// (need nested functions to get the closure right)
		data.button.onpress = (function(e){ return function() { e.callback ? e.callback(e) : callback(e) } })(item);

		if (data.affordableMask)
			data.affordableMask.hidden = true;	// actually used for the red "lack of resource" overlay, and darkening if unavailable. Sort of a hack.

		// GENERAL DATA
		// add general data, and a chance to abort on faulty data
		if (g_SelectionPanels[guiName].addData)
		{
			var success = g_SelectionPanels[guiName].addData(data);
			if (!success)
				continue; // ignore faulty data
		}

		// SET CONTENT
		// run all content setters
		for (var f in g_SelectionPanels[guiName])
		{
			if (f.match(/^set/))
				g_SelectionPanels[guiName][f](data);
		}

		// Special case: position
		if (!g_SelectionPanels[guiName].setPosition)
			setPanelObjectPosition(data.button, i, rowLength);

		// TODO: we should require all entities to have icons, so this case never occurs
		if (!data.icon.sprite)
			data.icon.sprite = "bkFillBlack";
		
	}

	// Hide any buttons we're no longer using
	for (var i = numberOfItems; i < g_unitPanelButtons[guiName]; ++i)
		if (g_SelectionPanels[guiName].hideItem)
			g_SelectionPanels[guiName].hideItem(i, rowLength);
		else
			Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	// remember the number of items
	g_unitPanelButtons[guiName] = numberOfItems;
}

function resourcesToAlphaMask(neededResources)
{
	var totalCost = 0;
	for each (var resource in neededResources)
		totalCost += resource;
	var alpha = 50 + Math.round(totalCost/10);
	alpha = alpha > 125 ? 125 : alpha;
	return "colour: 255 0 0 " + (alpha);
}

// Sets up "unit barter panel" - special case for setupUnitPanel
function setupUnitBarterPanel(unitEntState, playerState)
{
	// Amount of player's resource to exchange
	var amountToSell = BARTER_RESOURCE_AMOUNT_TO_SELL;
	if (Engine.HotkeyIsPressed("session.massbarter"))
		amountToSell *= BARTER_BUNCH_MULTIPLIER;
	// One pass for each resource
	for (var i = 0; i < BARTER_RESOURCES.length; i++)
	{
		var resource = BARTER_RESOURCES[i];
		// One pass for 'sell' row and another for 'buy'
		for (var j = 0; j < 2; j++)
		{
			var action = BARTER_ACTIONS[j];

			if (j == 0)
			{
				// Display the selection overlay
				var selection = Engine.GetGUIObjectByName("unitBarter" + action + "Selection["+i+"]");
				selection.hidden = !(i == g_barterSell);
			}

			// We gray out the not selected icons in 'sell' row
			var grayscale = (j == 0 && i != g_barterSell) ? "grayscale:" : "";
			var icon = Engine.GetGUIObjectByName("unitBarter" + action + "Icon["+i+"]");

			var button = Engine.GetGUIObjectByName("unitBarter" + action + "Button["+i+"]");
			button.size = (i * 46) + " 0 " + ((i + 1) * 46) + " 46";
			var amountToBuy;
			// We don't display a button in 'buy' row if the same resource is selected in 'sell' row
			if (j == 1 && i == g_barterSell)
			{
				button.hidden = true;
			}
			else
			{
				button.hidden = false;
				button.tooltip = action + " " + resource;
				icon.sprite = "stretched:"+grayscale+"session/icons/resources/" + resource + ".png";
				var sellPrice = unitEntState.barterMarket.prices["sell"][BARTER_RESOURCES[g_barterSell]];
				var buyPrice = unitEntState.barterMarket.prices["buy"][resource];
				amountToBuy = "+" + Math.round(sellPrice / buyPrice * amountToSell);
			}

			var amount;
			if (j == 0)
			{
				button.onpress = (function(i){ return function() { g_barterSell = i; } })(i);
				if (i == g_barterSell)
				{
					amount = "-" + amountToSell;

					var neededRes = {};
					neededRes[resource] = amountToSell;
					var neededResources = Engine.GuiInterfaceCall("GetNeededResources", neededRes);
					var hidden = neededResources ? false : true;
					for (var ii = 0; ii < BARTER_RESOURCES.length; ii++)
					{
						var affordableMask = Engine.GetGUIObjectByName("unitBarterBuyUnaffordable["+ii+"]");
						affordableMask.hidden = hidden;
					}
				}
				else
					amount = "";
 			}
			else
			{
				var exchangeResourcesParameters = { "sell": BARTER_RESOURCES[g_barterSell], "buy": BARTER_RESOURCES[i], "amount": amountToSell };
				button.onpress = (function(exchangeResourcesParameters){ return function() { exchangeResources(exchangeResourcesParameters); } })(exchangeResourcesParameters);
				amount = amountToBuy;
			}
			Engine.GetGUIObjectByName("unitBarter" + action + "Amount["+i+"]").caption = amount;
		}
	}
}

/**
 * Updates the right hand side "Unit Commands" panel. Runs in the main session loop via updateSelectionDetails().
 * Delegates to setupUnitPanel to set up individual subpanels, appropriately activated depending on the selected
 * unit's state.
 *
 * @param entState Entity state of the (first) selected unit.
 * @param supplementalDetailsPanel Reference to the "supplementalSelectionDetails" GUI Object
 * @param commandsPanel Reference to the "commandsPanel" GUI Object
 * @param selection Array of currently selected entity IDs.
 */
function updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection)
{
	// Panels that are active
	var usedPanels = {};

	// If the selection is friendly units, add the command panels
	var player = Engine.GetPlayerID();

	// Get player state to check some constraints
	// e.g. presence of a hero or build limits
	var simState = GetSimState();
	var playerState = simState.players[player];

	if (entState.player == player || g_DevSettings.controlAll)
	{
		if (selection.length > 1)
			setupUnitPanel(SELECTION, usedPanels, entState, playerState, g_Selection.groups.getTemplateNames(),
				function (entType, rightPressed) { changePrimarySelectionGroup(entType, rightPressed); } );

		var commands = getEntityCommandsList(entState);
		if (commands.length)
			setupUnitPanel(COMMAND, usedPanels, entState, playerState, commands,
				function (item) { performCommand(entState.id, item.name); } );

		if (entState.garrisonHolder)
		{
			var groups = new EntityGroups();
			for (var i in selection)
			{
				var state = GetEntityState(selection[i]);
				if (state.garrisonHolder)
					groups.add(state.garrisonHolder.entities)
			}

			setupUnitPanel(GARRISON, usedPanels, entState, playerState, groups.getTemplateNames(),
				function (item) { unloadTemplate(item); } );
		}

		var formations = Engine.GuiInterfaceCall("GetAvailableFormations");
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && !entState.garrisonHolder && formations.length)
		{
			setupUnitPanel(FORMATION, usedPanels, entState, playerState, formations,
				function (item) { performFormation(entState.id, item); } );
		}

		// TODO: probably should load the stance list from a data file,
		// and/or vary depending on what units are selected
		var stances = ["violent", "aggressive", "passive", "defensive", "standground"];
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && stances.length)
		{
			setupUnitPanel(STANCE, usedPanels, entState, playerState, stances,
				function (item) { performStance(entState.id, item); } );
		}

		Engine.GetGUIObjectByName("unitBarterPanel").hidden = !entState.barterMarket;
		if (entState.barterMarket)
		{
			usedPanels["Barter"] = 1;
			setupUnitBarterPanel(entState, playerState);
		}

		var buildableEnts = getAllBuildableEntitiesFromSelection();
		var trainableEnts = getAllTrainableEntitiesFromSelection();

		// Whether the GUI's right panel has been filled.
		var rightUsed = true;

		// The first selected entity's type has priority.
		if (entState.buildEntities)
			setupUnitPanel(CONSTRUCTION, usedPanels, entState, playerState, buildableEnts,
				function (trainEntType) { startBuildingPlacement(trainEntType, playerState); } );
		else if (entState.production && entState.production.entities)
			setupUnitPanel(TRAINING, usedPanels, entState, playerState, trainableEnts,
				function (trainEntType) { addTrainingToQueue(selection, trainEntType, playerState); } );
		else if (!entState.foundation && entState.gate || hasClass(entState, "LongWall"))
		{
			// Allow long wall pieces to be converted to gates
			var longWallTypes = {};
			var walls = [];
			var gates = [];
			for (var i in selection)
			{
				var state = GetEntityState(selection[i]);
				if (hasClass(state, "LongWall") && !state.gate && !longWallTypes[state.template])
				{
					var gateTemplate = getWallGateTemplate(state.id);
					if (gateTemplate)
					{
						var tooltipString = GetTemplateDataWithoutLocalization(state.template).gateConversionTooltip;
						if (!tooltipString)
						{
							warn(state.template + " is supposed to be convertable to a gate, but it's missing the GateConversionTooltip in the Identity template");
							tooltipString = "";
						}
						walls.push({
							"tooltip": translate(tooltipString),
							"template": gateTemplate,
							"callback": function (item) { transformWallToGate(item.template); }
						});
					}

					// We only need one entity per type.
					longWallTypes[state.template] = true;
				}
				else if (state.gate && !gates.length)
				{
					gates.push({
						"gate": state.gate,
						"tooltip": translate("Lock Gate"),
						"locked": true,
						"callback": function (item) { lockGate(item.locked); }
					});
					gates.push({
						"gate": state.gate,
						"tooltip": translate("Unlock Gate"),
						"locked": false,
						"callback": function (item) { lockGate(item.locked); }
					});
				}
				// Show both 'locked' and 'unlocked' as active if the selected gates have both lock states.
				else if (state.gate && state.gate.locked != gates[0].gate.locked)
					for (var j = 0; j < gates.length; ++j)
						delete gates[j].gate.locked;
			}

			// Place wall conversion options after gate lock/unlock icons.
			var items = gates.concat(walls);
			if (items.length)
				setupUnitPanel(GATE, usedPanels, entState, playerState, items);
			else
				rightUsed = false;
		}
		else if (entState.pack)
		{
			var items = [];
			var packButton = false;
			var unpackButton = false;
			var packCancelButton = false;
			var unpackCancelButton = false;
			for (var i in selection)
			{
				// Find un/packable entities
				var state = GetEntityState(selection[i]);
				if (state.pack)
				{
					if (state.pack.progress == 0)
					{
						if (!state.pack.packed)
							packButton = true;
						else if (state.pack.packed)
							unpackButton = true;
					}
					else
					{
						// Already un/packing - show cancel button
						if (!state.pack.packed)
							packCancelButton = true;
						else if (state.pack.packed)
							unpackCancelButton = true;
					}
				}
			}
			if (packButton)
				items.push({ "packing": false, "packed": false, "tooltip": translate("Pack"), "callback": function() { packUnit(true); } });
			if (unpackButton)
				items.push({ "packing": false, "packed": true, "tooltip": translate("Unpack"), "callback": function() { packUnit(false); } });
			if (packCancelButton)
				items.push({ "packing": true, "packed": false, "tooltip": translate("Cancel Packing"), "callback": function() { cancelPackUnit(true); } });
			if (unpackCancelButton)
				items.push({ "packing": true, "packed": true, "tooltip": translate("Cancel Unpacking"), "callback": function() { cancelPackUnit(false); } });

			if (items.length)
				setupUnitPanel(PACK, usedPanels, entState, playerState, items);
			else
				rightUsed = false;
		}
		else
			rightUsed = false;

		if (!rightUsed)
		{
			// The right pane is empty. Fill the pane with a sane type.
			// Prefer buildables for units and trainables for structures.
			if (buildableEnts.length && (hasClass(entState, "Unit") || !trainableEnts.length))
				setupUnitPanel(CONSTRUCTION, usedPanels, entState, playerState, buildableEnts,
					function (trainEntType) { startBuildingPlacement(trainEntType, playerState); });
			else if (trainableEnts.length)
				setupUnitPanel(TRAINING, usedPanels, entState, playerState, trainableEnts,
					function (trainEntType) { addTrainingToQueue(selection, trainEntType, playerState); } );
		}
		// Show technologies if the active panel has at most one row of icons.
		if (entState.production && entState.production.technologies.length)
		{
			var activepane = usedPanels[CONSTRUCTION] ? buildableEnts.length : trainableEnts.length;
			if (selection.length == 1 || activepane <= 8)
				setupUnitPanel(RESEARCH, usedPanels, entState, playerState, entState.production.technologies,
					function (researchType) { addResearchToQueue(entState.id, researchType); } );
		}

		if (entState.production && entState.production.queue.length)
			setupUnitPanel(QUEUE, usedPanels, entState, playerState, entState.production.queue,
				function (item) { removeFromProductionQueue(entState.id, item.id); } );

		supplementalDetailsPanel.hidden = false;
		commandsPanel.hidden = false;
	}
	else if (playerState.isMutualAlly[entState.player]) // owned by allied player
	{
		if (entState.garrisonHolder)
		{
			var groups = new EntityGroups();
			for (var i in selection)
			{
				var state = GetEntityState(selection[i]);
				if (state.garrisonHolder)
					groups.add(state.garrisonHolder.entities)
			}

			setupUnitPanel(GARRISON, usedPanels, entState, playerState, groups.getTemplateNames(),
				function (item) { unloadTemplate(item); } );

			supplementalDetailsPanel.hidden = false;
		}
		else
		{
			supplementalDetailsPanel.hidden = true;
		}

		commandsPanel.hidden = true;
	}
	else // owned by another player
	{
		supplementalDetailsPanel.hidden = true;
		commandsPanel.hidden = true;
	}

	// Hides / unhides Unit Panels (panels should be grouped by type, not by order, but we will leave that for another time)
	var offset = 0;
	for each (var panelName in g_unitPanels)
	{
		var panel = Engine.GetGUIObjectByName("unit" + panelName + "Panel");
		if (usedPanels[panelName])
			panel.hidden = false;
		else
			panel.hidden = true;
	}
}

// Force hide commands panels
function hideUnitCommands()
{
	for each (var panelName in g_unitPanels)
		Engine.GetGUIObjectByName("unit" + panelName + "Panel").hidden = true;
}

// Get all of the available entities which can be trained by the selected entities
function getAllTrainableEntities(selection)
{
	var trainableEnts = [];
	var state;
	// Get all buildable and trainable entities
	for (var i in selection)
	{
		if ((state = GetEntityState(selection[i])) && state.production && state.production.entities.length)
			trainableEnts = trainableEnts.concat(state.production.entities);
	}

	// Remove duplicates
	removeDupes(trainableEnts);
	return trainableEnts;
}

function getAllTrainableEntitiesFromSelection()
{
	if (!g_allTrainableEntities)
		g_allTrainableEntities = getAllTrainableEntities(g_Selection.toList());

	return g_allTrainableEntities;
}

// Get all of the available entities which can be built by the selected entities
function getAllBuildableEntities(selection)
{
	var buildableEnts = [];
	var state;
	// Get all buildable entities
	for (var i in selection)
	{
		if ((state = GetEntityState(selection[i])) && state.buildEntities && state.buildEntities.length)
			buildableEnts = buildableEnts.concat(state.buildEntities);
	}

	// Remove duplicates
	removeDupes(buildableEnts);
	return buildableEnts;
}

function getAllBuildableEntitiesFromSelection()
{
	if (!g_allBuildableEntities)
		g_allBuildableEntities = getAllBuildableEntities(g_Selection.toList());

	return g_allBuildableEntities;
}

// Check if the selection can move into formation, and cache the result
function canMoveSelectionIntoFormation(formationTemplate)
{
	if (!(formationTemplate in g_canMoveIntoFormation))
	{
		g_canMoveIntoFormation[formationTemplate] = Engine.GuiInterfaceCall("CanMoveEntsIntoFormation", {
			"ents": g_Selection.toList(),
			"formationTemplate": formationTemplate
		});
	}
	return g_canMoveIntoFormation[formationTemplate];
}

function getVisibleEntityClassesFormatted(template)
{
	var r = ""
	if (template.visibleIdentityClasses && template.visibleIdentityClasses.length)
	{
		r += "\n[font=\"sans-bold-13\"]" + translate("Classes:") + "[/font] ";
		r += "[font=\"sans-13\"]" + translate(template.visibleIdentityClasses[0]) ;
		for (var c = 1; c < template.visibleIdentityClasses.length; c++)
			r += ", " + translate(template.visibleIdentityClasses[c]);
		r += "[/font]";
	}
	return r;
};
