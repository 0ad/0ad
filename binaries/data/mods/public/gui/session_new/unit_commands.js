// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = { "Construction": 0, "Training": 0, "Queue": 0 };

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Stance", "Formation", "Construction", "Research", "Training", "Queue", "Selection"];

// Sets up "unit panels" - the panels with rows of icons (Helper function for updateUnitDisplay)
function setupUnitPanel(guiName, usedPanels, unitEntState, items, callback)
{
	usedPanels[guiName] = 1;
	var i = 0;

	for each (var item in items)
	{
		if (i > 15) // End loop early if more than 16 buttons
			break;

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
		var tooltip = (isUnitElite(entType)?  "Elite " + name : name ); // "Elite " is not formatted in bold, so may need custom versions of this later
		
		if (guiName == "Selection")
		{
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = 
				(g_Selection.groups.groupTypeCount[item] > 1 ? g_Selection.groups.groupTypeCount[item] : "");

			tooltip += (g_Selection.groups.groupTypeCount[item] > 1 ? " (" + g_Selection.groups.groupTypeCount[item] + ")" : "")
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

		icon.sprite = "snPortraitSheetHele"; // TODO
		if (typeof template.icon_cell == "undefined")
			icon.cell_id = 0;
		else
			icon.cell_id = template.icon_cell;
			
		++i;
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = ((guiName == "Selection")? 37 : 45);
	var numButtons = i;
	var j = 0; // index for second row of buttons
	
	for (i = 0; i < numButtons; ++i)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var size = button.size;

		if (i > 7) // Make second row
		{
			if (guiName == "Queue")
				getGUIObjectByName("unit"+guiName+"Panel").size = "0 -104 100% 100%-166"
				
			size.left = buttonSpacer*j;
			size.right = buttonSpacer*j + buttonSideLength;
			size.top = buttonSpacer;
			size.bottom = buttonSpacer + buttonSideLength;
			j++;
		}
		else // Make first row
		{
			if ((guiName == "Queue"))
				getGUIObjectByName("unit"+guiName+"Panel").size = "0 -60 100% 100%-166"
		
			size.left = buttonSpacer*i;
			size.right = buttonSpacer*i + size.bottom;
		}

		button.size = size;
	}
	
	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	g_unitPanelButtons[guiName] = numButtons;
}

//  Updates right Unit Commands Panel - runs in the main session loop via updateSelectionDetails()
function updateUnitCommands(commandsPanel,  selection, entState)
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
			setupUnitPanel("Selection", usedPanels, entState, g_Selection.groups.groupTemplates,
				function (entType) { changePrimarySelectionGroup(entType); } );

		// Stamina
		//	if (entState.stamina != undefined)
				getGUIObjectByName("selectionDetailsStamina").hidden = false;
		//	else
		//		getGUIObjectByName("selectionDetailsStamina").hidden = true;
				
		commandsPanel.hidden = false;
	}
	else
	{
		getGUIObjectByName("selectionDetailsStamina").hidden = true;
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
