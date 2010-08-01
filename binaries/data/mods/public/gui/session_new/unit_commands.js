// Panel types
const SELECTION = "Selection";
const QUEUE = "Queue";
const TRAINING = "Training";
const CONSTRUCTION = "Construction";
const COMMAND = "Command";

// Constants used by the Queue or Garrison panel
const UNIT_PANEL_BASE = -47; // The offset above the main panel (will often be negative)
const UNIT_PANEL_HEIGHT = 37; // The height needed for a row of buttons

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Training": 0, "Construction": 0, "Command": 0};

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Selection", "Queue", "Training", "Construction", "Research", "Stance", "Formation", "Command"];

// Lay out button rows
function layoutButtonRow(rowNumber, guiName, buttonSideLength, buttonSpacer, startIndex, endIndex)
{
	var colNumber = 0;

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var size = button.size;
		
		size.left = buttonSpacer*colNumber;
		size.right = buttonSpacer*colNumber + buttonSideLength;
		size.top = buttonSpacer*rowNumber;
		size.bottom = buttonSpacer*rowNumber + buttonSideLength;
		
		button.size = size;
		colNumber++;
	}
}

// Sets up "unit panels" - the panels with rows of icons (Helper function for updateUnitDisplay)
function setupUnitPanel(guiName, usedPanels, unitEntState, items, callback)
{
	usedPanels[guiName] = 1;
	var selection = g_Selection.toList();

	// Set length of loop
	var numberOfItems = items.length;
	if (numberOfItems > 24)
	{
		if (guiName == "Selection")
		{
			if (numberOfItems > 32)
				numberOfItems = 32;
		}
		else
			numberOfItems = 24;
	}

	var i;
	for (i = 0; i < numberOfItems; i++)
	{
		var item = items[i];

		// Get templates
		if (guiName != "Command")
		{
			var entType = ((guiName == "Queue")? item.template : item);	
			var template = Engine.GuiInterfaceCall("GetTemplateData", entType);
			if (!template)
				continue; // ignore attempts to use invalid templates (an error should have been reported already)
		}

		// Tooltip
		var tooltip = "";

		switch (guiName)
		{
		case SELECTION:
			tooltip = getEntityName(template);

			var entState = Engine.GuiInterfaceCall("GetEntityState", selection[i]);
			if (!entState)
				return;

			// Rank Title
			var rankText = getRankTitle(getRankCellId(entState.template));
			rankText = (rankText? " [font=\"serif-bold-16\"](" + rankText + ")[/font]" : "" );
			tooltip += rankText;

			// Hitpoints
			if (entState.maxHitpoints != undefined)
			{
				var unitHealthBar = getGUIObjectByName("unitSelectionHealthForeground[" + i + "]");
				var healthSize = unitHealthBar.size;
				healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
				unitHealthBar.size = healthSize;
				tooltip += " [font=\"serif-9\"](" + entState.hitpoints + "/" + entState.maxHitpoints + ")[/font]";
			}
			break;

		case QUEUE:
			tooltip = getEntityNameWithGeneric(template);

			var progress = Math.round(item.progress*100) + "%";
			tooltip += " - " + progress;
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (item.count > 1 ? item.count : "");
			getGUIObjectByName("unit"+guiName+"Progress["+i+"]").caption = (item.progress ? progress : "");
			break;

		case TRAINING:
			tooltip = getEntityNameWithGeneric(template) + "\n" + getEntityCost(template);

			var [batchSize, batchIncrement] = getTrainingQueueBatchStatus(unitEntState.id, entType);
			if (batchSize)
			{
				tooltip += "\n[font=\"serif-13\"]Training [font=\"serif-bold-13\"]" + batchSize + "[font=\"serif-13\"] units; " + 
				"Shift-click to train [font=\"serif-bold-13\"]"+ (batchSize+batchIncrement) + "[font=\"serif-13\"] units[/font]";
			}
			break;
			
		case CONSTRUCTION:
			tooltip = getEntityNameWithGeneric(template) + "\n" + getEntityCost(template);
			break;
			
		case COMMAND:
			tooltip = item;
			break;

		default:
			break;
		}

		// Button
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		button.hidden = false;
		button.tooltip = tooltip;

		// Button Function
		var parameter = ((guiName == "Selection")? i : item);
		button.onpress = (function(e) { return function() { callback(e) } })(parameter); // (need nested functions to get the closure right)

		// Get icon sheet
		if (guiName == "Command")
		{
			icon.cell_id = i;
		}
		else
		{
			icon.sprite = template.icon_sheet;
			icon.cell_id = ((typeof template.icon_cell == "undefined")? 0 : template.icon_cell);
		}
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var numButtons = i;
	var rowLength = ((guiName == "Command")? 15 : 8);
	var numRows = Math.ceil(numButtons / rowLength);
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;

	// Resize Queue panel if needed
	if (guiName == "Queue") // or garrison
	{
		var panel = getGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		size.top = (UNIT_PANEL_BASE - ((numRows-1)*UNIT_PANEL_HEIGHT));
		panel.size = size;
	}

	// Layout buttons
	for (var i = 0; i < numRows; i++)
		layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*i, rowLength*(i+1) );

	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	g_unitPanelButtons[guiName] = numButtons;
}

// Updates right Unit Commands Panel - runs in the main session loop via updateSelectionDetails()
function updateUnitCommands(entState, commandsPanel, selection)
{
	// Panels that are active
	var usedPanels = {};

	// If the selection is friendly units, add the command panels
	var player = Engine.GetPlayerID();
	if (entState.player == player || g_DevSettings.controlAll)
	{
		if (entState.attack) // TODO - this should be based on some AI properties
		{
			//usedPanels["Stance"] = 1;
			//usedPanels["Formation"] = 1;
			// (These are disabled since they're not implemented yet)
		}
		else // TODO - this should be based on various other things
		{
			//usedPanels["Research"] = 1;
		}

		if (entState.buildEntities && entState.buildEntities.length)
			setupUnitPanel("Construction", usedPanels, entState, entState.buildEntities, startBuildingPlacement);

		if (entState.training && entState.training.entities.length)
			setupUnitPanel("Training", usedPanels, entState, entState.training.entities,
				function (trainEntType) { addToTrainingQueue(entState.id, trainEntType); } );

		if (entState.training && entState.training.queue.length)
			setupUnitPanel("Queue", usedPanels, entState, entState.training.queue,
				function (item) { removeFromTrainingQueue(entState.id, item.id); } );

		if (selection.length > 1)
			setupUnitPanel("Selection", usedPanels, entState, g_Selection.getTemplateNames(),
				function (entType) { changePrimarySelectionGroup(entType); } );
	
		// HACK: This should be referenced in the entities template, rather that completely faked here
		var commands = [];
		for (var i = 0; i < 15; i++)
			commands.push("test"+i);
		commands[4] = "delete";

		// Needs to have list provided by the entity
		if (commands.length)
			setupUnitPanel("Command", usedPanels, entState, commands,
				function (item) { performCommand(entState.id, item); } );

		commandsPanel.hidden = false;
	}
	else
	{
		getGUIObjectByName("sdStamina").hidden = true;
		commandsPanel.hidden = true;
	}

	// Hides / unhides Unit Panels (panels should be grouped by type, not by order, but we will leave that for another time)
	var offset = 0;
	for each (var panelName in g_unitPanels)
	{
		var panel = getGUIObjectByName("unit" + panelName + "Panel");
		if (usedPanels[panelName])
			panel.hidden = false;
		else
			panel.hidden = true;
	}
}
