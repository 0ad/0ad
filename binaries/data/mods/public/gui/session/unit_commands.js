// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Formation": 0, "Garrison": 0, "Training": 0, "Research": 0, "Barter": 0, "Construction": 0, "Command": 0, "Stance": 0, "Gate": 0, "Pack": 0};

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
 * Helper function for updateUnitCommands; sets up "unit panels"
 * (i.e. panels with rows of icons) for the currently selected unit.
 *
 * @param guiName Short identifier string of this panel. See g_SelectionPanels.
 * @param unitEntState Entity state of the (first) selected unit.
 * @param payerState Player state
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
 * Updates the selection panels where buttons are supposed to 
 * depend on the context.
 * Runs in the main session loop via updateSelectionDetails().
 * Delegates to setupUnitPanel to set up individual subpanels,
 * appropriately activated depending on the selected unit's state.
 *
 * @param entState Entity state of the (first) selected unit.
 * @param supplementalDetailsPanel Reference to the 
 *        "supplementalSelectionDetails" GUI Object
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
		setupUnitPanel("Garrison", entState, playerState);
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
