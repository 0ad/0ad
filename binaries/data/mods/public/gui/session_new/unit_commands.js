// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = { "Construction": 0, "Training": 0, "Queue": 0 };

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Stance", "Formation", "Construction", "Research", "Training", "Queue", "Selection"];

// Lay out button rows
function layoutButtonRow(rowNumber, guiName, buttonSideLength, buttonSpacer, startIndex, endIndex)
{
	var colNumber = 0;

	for (i = startIndex; i < endIndex; i++)
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
function setupUnitPanel(guiName, usedPanels, playerState, unitEntState, items, callback)
{
	usedPanels[guiName] = 1;
	var i = 0;

	for each (var item in items)
	{
		if (i > 23) // End loop early if there are more than 24 buttons
			break;
		else if (guiName == "Selection" && i > 15) // End loop early if more then 16 selection buttons
			break

		// Get templates
		var entType;
		if (guiName == "Queue")
			entType = item.template;
		else
			entType = item;
			
		var template = Engine.GuiInterfaceCall("GetTemplateData", entType);
		if (!template)
			continue; // ignore attempts to use invalid templates (an error should have been reported already)

		// Name
		var name;
		if (guiName == "Selection")
			name = template.name.specific || template.name.generic || "???";
		else
			name = getFullName(template);

		// Tooltip
		var tooltip = name;
		
		if (guiName == "Selection")
		{
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = 
				(g_Selection.groups.typeCount[item] > 1 ? g_Selection.groups.typeCount[item] : "");

			tooltip += (g_Selection.groups.typeCount[item] > 1 ? " (" + g_Selection.groups.typeCount[item] + ")" : "")
		}
		else if (guiName == "Queue")
		{
			var progress = Math.round(item.progress*100) + "%";
			tooltip += " - " + progress;
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (item.count > 1 ? item.count : "");
			getGUIObjectByName("unit"+guiName+"Progress["+i+"]").caption = (item.progress ? progress : "");
		}
		else if (guiName == "Construction" || guiName == "Training")
		{
			if (template.cost)
			{
				var costs = [];
				if (template.cost.food) costs.push("[font=\"serif-bold-13\"]Food:[/font] " + template.cost.food);
				if (template.cost.wood) costs.push("[font=\"serif-bold-13\"]Wood:[/font] " + template.cost.wood);
				if (template.cost.metal) costs.push("[font=\"serif-bold-13\"]Metal:[/font] " + template.cost.metal);
				if (template.cost.stone) costs.push("[font=\"serif-bold-13\"]Stone:[/font] " + template.cost.stone);
				if (costs.length)
					tooltip += "\n" + costs.join(", ");
			}
		
			if (guiName == "Training")
			{
				var [batchSize, batchIncrement] = getTrainingQueueBatchStatus(unitEntState.id, entType);
				tooltip += "\n[font=\"serif-13\"]";
				if (batchSize) tooltip += "Training [font=\"serif-bold-13\"]" + batchSize + "[font=\"serif-13\"] units; ";
					tooltip += "Shift-click to train [font=\"serif-bold-13\"]"+ (batchSize+batchIncrement) + "[font=\"serif-13\"] units[/font]";
			}	
		}

		// Button
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		button.hidden = false;
		button.tooltip = tooltip;

		if (callback != null)
			button.onpress = (function(e) { return function() { callback(e) } })(item); // (need nested functions to get the closure right)

		// Get icon sheet
		icon.sprite = getPortraitSheetName(getTemplateCategory(item));

		if (typeof template.icon_cell == "undefined")
			icon.cell_id = 0;
		else
			icon.cell_id = template.icon_cell;
			
		++i;
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var numButtons = i;
	var buttonSpacer = ((guiName == "Selection")? 37 : 45);
	
	if (numButtons < 9) // Row 0
	{
		if ((guiName == "Queue")) // or garrison
			getGUIObjectByName("unit"+guiName+"Panel").size = "0 -60 100% 100%-166";
	
		layoutButtonRow(0, guiName, buttonSideLength, buttonSpacer, 0, 8);
	}
	else if (numButtons < 17) // Row 1
	{
		if (guiName == "Queue") // or garrison
			getGUIObjectByName("unit"+guiName+"Panel").size = "0 -105 100% 100%-166";
	
		layoutButtonRow(0, guiName, buttonSideLength, buttonSpacer, 0, 8);
		layoutButtonRow(1, guiName, buttonSideLength, buttonSpacer, 8, 16);
	}
	else // Row 2
	{
		if (guiName == "Queue") // or garrison
			getGUIObjectByName("unit"+guiName+"Panel").size = "0 -150 100% 100%-166";
			
		layoutButtonRow(0, guiName, buttonSideLength, buttonSpacer, 0, 8);
		layoutButtonRow(1, guiName, buttonSideLength, buttonSpacer, 8, 16);
		if (guiName != "Selection")
			layoutButtonRow(2, guiName, buttonSideLength, buttonSpacer, 16, 24);
	}

	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	g_unitPanelButtons[guiName] = numButtons;
}

// Updates right Unit Commands Panel - runs in the main session loop via updateSelectionDetails()
function updateUnitCommands(playerState, entState, commandsPanel, selection)
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
			setupUnitPanel("Construction", usedPanels, playerState, entState, entState.buildEntities, startBuildingPlacement);

		if (entState.training && entState.training.entities.length)
			setupUnitPanel("Training", usedPanels, playerState, entState, entState.training.entities,
				function (trainEntType) { addToTrainingQueue(entState.id, trainEntType); } );

		if (entState.training && entState.training.queue.length)
			setupUnitPanel("Queue", usedPanels, playerState, entState, entState.training.queue,
				function (item) { removeFromTrainingQueue(entState.id, item.id); } );

		if (selection.length > 1)
			setupUnitPanel("Selection", usedPanels, playerState, entState, g_Selection.groups.templates,
				function (entType) { changePrimarySelectionGroup(entType); } );

		// Stamina
		if (entState.stamina != undefined)
			getGUIObjectByName("sdStamina").hidden = false;
		else
			getGUIObjectByName("sdStamina").hidden = true;
				
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
		{
//			var size = panel.size;
//			var h = size.bottom - size.top;
//			size.bottom = offset;
//			size.top = offset - h;
//			panel.size = size;
			panel.hidden = false;
//			offset -= (h + 6); // changed 12 point spacing to 6 point:   offset -= (h + 12);
		}
		else
		{
			panel.hidden = true;
		}
	}
}
