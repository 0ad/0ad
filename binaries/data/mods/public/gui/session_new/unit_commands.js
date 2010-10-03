// Panel types
const SELECTION = "Selection";
const QUEUE = "Queue";
const GARRISON = "Garrison";
const FORMATION = "Formation";
const TRAINING = "Training";
const CONSTRUCTION = "Construction";
const COMMAND = "Command";

// Constants
const COMMANDS_PANEL_WIDTH = 228;
const UNIT_PANEL_BASE = -52; // QUEUE: The offset above the main panel (will often be negative)
const UNIT_PANEL_HEIGHT = 44; // QUEUE: The height needed for a row of buttons

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Formation": 0, "Garrison": 0, "Training": 0, "Construction": 0, "Command": 0};

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Selection", "Queue", "Formation", "Garrison", "Training", "Construction", "Research", "Stance", "Command"];

// Lay out a row of centered buttons (does not work inside a loop like the other function)
function layoutButtonRowCentered(rowNumber, guiName, startIndex, endIndex, width)
{
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;
	var colNumber = 0;

	// Collect buttons
	var buttons = [];
	var icons = [];

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");

		if (button)
		{
			buttons.push(button);
			icons.push(icon);
		}
	}

	// Location of middle button
	var middleIndex = Math.ceil(buttons.length/2);

	// Determine whether even or odd number of buttons
	var center = (buttons.length/2 == Math.ceil(buttons.length/2))? Math.ceil(width/2) : Math.ceil(width/2+buttonSpacer/2);

	// Left Side
	for (var i = middleIndex-1; i >= 0; i--)
	{
		if (buttons[i])
		{
			var icon = icons[i];
			var size = buttons[i].size;
			size.left = center - buttonSpacer*colNumber - buttonSideLength;
			size.right = center - buttonSpacer*colNumber;
			size.top = buttonSpacer*rowNumber;
			size.bottom = buttonSpacer*rowNumber + buttonSideLength;
			buttons[i].size = size;
			colNumber++;
		}
	}

	// Right Side
	center += 1; // add spacing to center buttons
	colNumber = 0; // reset to 0

	for (var i = middleIndex; i < buttons.length; i++)
	{
		if (buttons[i])
		{
			var icon = icons[i];
			var size = buttons[i].size;
			size.left = center + buttonSpacer*colNumber;
			size.right = center + buttonSpacer*colNumber + buttonSideLength;
			size.top = buttonSpacer*rowNumber;
			size.bottom = buttonSpacer*rowNumber + buttonSideLength;
			buttons[i].size = size;
			colNumber++;
		}
	}
}

// Lay out button rows
function layoutButtonRow(rowNumber, guiName, buttonSideLength, buttonSpacer, startIndex, endIndex)
{
	var colNumber = 0;

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");

		if (button)
		{
			var size = button.size;
			
			size.left = buttonSpacer*colNumber;
			size.right = buttonSpacer*colNumber + buttonSideLength;
			size.top = buttonSpacer*rowNumber;
			size.bottom = buttonSpacer*rowNumber + buttonSideLength;
			
			button.size = size;
			colNumber++;
		}
	}
}

// Sets up "unit panels" - the panels with rows of icons (Helper function for updateUnitDisplay)

function setupUnitPanel(guiName, usedPanels, unitEntState, items, callback)
{
	usedPanels[guiName] = 1;
	var numberOfItems = items.length;
	var selection = g_Selection.toList();

	if (guiName == "Selection")
	{
		if (numberOfItems > 15)
				numberOfItems =  15;
	}
	if (guiName == "Formation" || guiName == "Garrison")
	{
		if (numberOfItems > 15)
				numberOfItems =  15;
	}
	else if (guiName == "Queue")
	{
		if (numberOfItems > 16)
			numberOfItems = 16;
	}
	else if ( guiName == "Command")
	{
		if (numberOfItems > 4)
			numberOfItems = 4;
	}
	else if (numberOfItems > 18)
	{
		numberOfItems =  18;
	}

	var i;
	for (i = 0; i < numberOfItems; i++)
	{
		var item = items[i];
		var entType = ((guiName == "Queue")? item.template : item);
		var template;
		if (guiName != "Formation" && guiName != "Command")
		{
			template = GetTemplateData(entType);
			if (!template)
				continue; // ignore attempts to use invalid templates (an error should have been reported already)
		}

		switch (guiName)
		{
		case SELECTION:
			var tooltip = getEntityName(template);	
			var count = g_Selection.groups.getCount(item);
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 1 ? count : "");
			break;

		case QUEUE:
			var tooltip = getEntityName(template);
			var progress = Math.round(item.progress*100) + "%";
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (item.count > 1 ? item.count : "");
			
			if (i == 0)
			{
				getGUIObjectByName("queueProgress").caption = (item.progress ? progress : "");
				var size = getGUIObjectByName("unit"+guiName+"ProgressSlider["+i+"]").size;
				size.top = Math.round(item.progress*40);
				getGUIObjectByName("unit"+guiName+"ProgressSlider["+i+"]").size = size;
			}
			break;

		case GARRISON:
			var tooltip = getEntityName(template);
			var count = g_Selection.groups.getCount(item);
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 1 ? count : "");
			break;

		case FORMATION:
			var tooltip = toTitleCase(item);
			break;

		case TRAINING:
			var tooltip = getEntityNameWithGenericType(template);
			if (template.tooltip)
				tooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

			tooltip += "\n" + getEntityCost(template);

			var [batchSize, batchIncrement] = getTrainingQueueBatchStatus(unitEntState.id, entType);
			if (batchSize)
			{
				tooltip += "\n[font=\"serif-13\"]Training [font=\"serif-bold-13\"]" + batchSize + "[font=\"serif-13\"] units; " + 
				"Shift-click to train [font=\"serif-bold-13\"]"+ (batchSize+batchIncrement) + "[font=\"serif-13\"] units[/font]";
			}
			break;

		case CONSTRUCTION:
			var tooltip = getEntityNameWithGenericType(template);
			if (template.tooltip)
				tooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

			tooltip += "\n" + getEntityCost(template);
			break;

		case COMMAND:
			tooltip = toTitleCase(item);
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
		button.onpress = (function(e) { return function() { callback(e) } })(item); // (need nested functions to get the closure right)

		// Get icon image
		if (guiName == "Formation")
		{
			icon.cell_id = getFormationCellId(item);
			icon.enabled = false;
			if (!icon.enabled)
				icon.sprite = "formation_disabled";
		}
		else if (guiName == "Command")
		{
			//icon.cell_id = i;
			icon.cell_id = getCommandCellId(item);
		}
		else
		{
			icon.sprite = template.icon_sheet;
			icon.cell_id = ((typeof template.icon_cell == "undefined")? 0 : template.icon_cell);
		}
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var numButtons = i;

	var rowLength = 8;
	if (guiName == "Selection")
		rowLength = 5;
	else if (guiName == "Formation" || guiName == "Garrison" || guiName == "Command")
		rowLength = 4;

	var numRows = Math.ceil(numButtons / rowLength);
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;

	// Layout buttons
	if (guiName == "Command")
	{
		layoutButtonRowCentered(0, guiName, 0, numButtons, COMMANDS_PANEL_WIDTH);
	}
	else
	{
		for (var i = 0; i < numRows; i++)
			layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*i, rowLength*(i+1) );
	}
	
	// Resize Queue panel if needed
	if (guiName == "Queue") // or garrison
	{
		var panel = getGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		size.top = (UNIT_PANEL_BASE - ((numRows-1)*UNIT_PANEL_HEIGHT));
		panel.size = size;
	}

	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	g_unitPanelButtons[guiName] = numButtons;
}

// Updates right Unit Commands Panel - runs in the main session loop via updateSelectionDetails()
function updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection)
{
	var isInvisble = true;
	
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

		if (selection.length > 1)
			setupUnitPanel("Selection", usedPanels, entState, g_Selection.groups.getTemplateNames(),
				function (entType) { changePrimarySelectionGroup(entType); } );

		var commands = getEntityCommandsList(entState);
		if (commands.length)
			setupUnitPanel("Command", usedPanels, entState, commands,
				function (item) { performCommand(entState.id, item); } );

/*
		if (selection.length > 1)
			setupUnitPanel("Garrison", usedPanels, entState, g_Selection.groups.getTemplateNames(),
				function (entType) { changePrimarySelectionGroup(entType); } );
*/

		var formations = getEntityFormationsList(entState);
		if (formations.length)
			setupUnitPanel("Formation", usedPanels, entState, formations,
				function (item) { performFormation(entState.id, item); } );

		if (entState.buildEntities && entState.buildEntities.length)
		{
			setupUnitPanel("Construction", usedPanels, entState, entState.buildEntities, startBuildingPlacement);
			isInvisble = false;
		}

		if (entState.training && entState.training.entities.length)
		{
			setupUnitPanel("Training", usedPanels, entState, entState.training.entities,
				function (trainEntType) { addToTrainingQueue(entState.id, trainEntType); } );
			isInvisble = false;
		}

		if (entState.training && entState.training.queue.length)
			setupUnitPanel("Queue", usedPanels, entState, entState.training.queue,
				function (item) { removeFromTrainingQueue(entState.id, item.id); } );

		supplementalDetailsPanel.hidden = false;
		commandsPanel.hidden = isInvisble;
	}
	else
	{
		getGUIObjectByName("stamina").hidden = true;
		supplementalDetailsPanel.hidden = true;
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
