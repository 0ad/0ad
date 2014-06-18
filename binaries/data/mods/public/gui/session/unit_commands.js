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
const TRADING_RESOURCES = [
	markForTranslation("food"),
	markForTranslation("wood"),
	markForTranslation("stone"),
	markForTranslation("metal")
];

// Barter constants
const BARTER_RESOURCE_AMOUNT_TO_SELL = 100;
const BARTER_BUNCH_MULTIPLIER = 5;
const BARTER_RESOURCES = ["food", "wood", "stone", "metal"];
const BARTER_ACTIONS = ["Sell", "Buy"];

// Gate constants
const GATE_ACTIONS = ["lock", "unlock"];

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Formation": 0, "Garrison": 0, "Training": 0, "Research": 0, "Barter": 0, "Construction": 0, "Command": 0, "Stance": 0, "Gate": 0, "Pack": 0};

// Resources to sell on barter panel
var g_barterSell = "food";

/**
 * Set the position of a panel object according to the index,
 * from left to right, from top to bottom.
 * Will wrap around to subsequent rows if the index
 * is larger than rowLength.
 */
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
 * @param unitEntState Entity state of the (first) selected unit.
 */
function setupUnitPanel(guiName, unitEntState, playerState)
{
	if (!g_SelectionPanels[guiName])
	{
		error("unknown guiName used '" + guiName + "'");
		return;
	}
	var selection = g_Selection.toList();

	var items = g_SelectionPanels[guiName].getItems(unitEntState, selection);

	if (!items || !items.length)
		return;

	var numberOfItems = items.length;
	var garrisonGroups = new EntityGroups();

	// Determine how many buttons there should be
	var maxNumberOfItems = g_SelectionPanels[guiName].getMaxNumberOfItems()
	if (maxNumberOfItems < numberOfItems)
		numberOfItems = maxNumberOfItems;

	if (g_SelectionPanels[guiName].rowLength)
		var rowLength = g_SelectionPanels[guiName].rowLength;
	else
		var rowLength = 8;

	if (g_SelectionPanels[guiName].resizePanel)
		g_SelectionPanels[guiName].resizePanel(numberOfItems, rowLength);

	// Make buttons
	for (var i = 0; i < numberOfItems; i++)
	{
		var item = items[i];

		// If a tech has been researched it leaves an empty slot
		if (!item)
		{
			if (g_SelectionPanels[guiName].hideItem)
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
			"rowLength": rowLength,
			"numberOfItems": numberOfItems,
		};

		// add standard gui objects to the data
		// depending on the actual XML, some of this may be undefined
		data.button = Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]");
		data.affordableMask = Engine.GetGUIObjectByName("unit"+guiName+"Unaffordable["+i+"]");
		data.icon = Engine.GetGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		data.guiSelection = Engine.GetGUIObjectByName("unit"+guiName+"Selection["+i+"]");
		data.countDisplay = Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]");


		// DEFAULTS
		if (data.button)
		{
			data.button.hidden = false;
			data.button.enabled = true;
			data.button.tooltip = "";
			data.button.caption = "";
		}

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
		if (data.icon && !data.icon.sprite)
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
	g_SelectionPanels[guiName].used = true;
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
	for each (var panel in g_SelectionPanels)
		panel.used = false;

	// If the selection is friendly units, add the command panels
	var player = Engine.GetPlayerID();

	// Get player state to check some constraints
	// e.g. presence of a hero or build limits
	var simState = GetSimState();
	var playerState = simState.players[player];

	if (entState.player == player || g_DevSettings.controlAll)
	{
		for (var guiName of g_PanelsOrder)
		{
			if (
				g_SelectionPanels[guiName].conflictsWith && 
				g_SelectionPanels[guiName].conflictsWith.some(function (p) { return g_SelectionPanels[p].used; })
			)
				continue;

			setupUnitPanel(guiName, entState, playerState);
		}

		supplementalDetailsPanel.hidden = false;
		commandsPanel.hidden = false;
	}
	else if (playerState.isMutualAlly[entState.player]) // owned by allied player
	{
		// TODO if there's a second panel needed for a different player
		// we should consider adding the players list to g_SelectionPanels
		setupUnitPanel(GARRISON, entState, playerState);
		if (g_SelectionPanels["Garrison"].used)
			supplementalDetailsPanel.hidden = false;
		else
			supplementalDetailsPanel.hidden = true;

		commandsPanel.hidden = true;
	}
	else // owned by another player
	{
		supplementalDetailsPanel.hidden = true;
		commandsPanel.hidden = true;
	}

	// Hides / unhides Unit Panels (panels should be grouped by type, not by order, but we will leave that for another time)
	for (var panelName in g_SelectionPanels)
		Engine.GetGUIObjectByName("unit" + panelName + "Panel").hidden = !g_SelectionPanels[panelName].used;
}

// Force hide commands panels
function hideUnitCommands()
{
	for (var panelName in g_SelectionPanels)
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
}

function getNumberOfRightPanelButtons()
{
	var sum = 0;
	if (g_SelectionPanels["Construction"].used)
		sum += g_unitPanelButtons["Construction"];
	if (g_SelectionPanels["Training"].used)
		sum += g_unitPanelButtons["Training"];
	if (g_SelectionPanels["Pack"].used)
		sum += g_unitPanelButtons["Pack"];
	if (g_SelectionPanels["Gate"].used)
		sum += g_unitPanelButtons["Gate"];
	return sum;
}
