const resourceIconCellIds = {food : 0, wood : 1, stone : 2, metal : 3};

// Multiple Selection Layout
function selectionLayoutMultiple()
{
		getGUIObjectByName("selectionDetailsMainText").size = "80 100%-70 100%-14 100%-10";
		getGUIObjectByName("selectionDetailsSpecific").size = "0 6 100% 30";
		getGUIObjectByName("selectionDetailsPlayer").size = "0 34 100% 100%-8";

		getGUIObjectByName("selectionDetailsIcon").size = "10 100%-74 66 100%-18";
		getGUIObjectByName("selectionDetailsHealth").size = "10 100%-16 66 100%-12";
		getGUIObjectByName("selectionDetailsStamina").size = "10 100%-10 66 100%-6";

		getGUIObjectByName("selectionDetailsAttack").hidden = true;
		getGUIObjectByName("selectionDetailsArmour").hidden = true;

		getGUIObjectByName("selectionDetailsMainText").sprite = "goldPanel";
		getGUIObjectByName("selectionDetailsSpecific").sprite = "";
}

// Single Selection Layout
function selectionLayoutSingle()
{
		getGUIObjectByName("selectionDetailsMainText").size = "6 0 100%-6 50";
		getGUIObjectByName("selectionDetailsSpecific").size = "0 0 100% 30";
		getGUIObjectByName("selectionDetailsPlayer").size = "0 30 100% 50";

		getGUIObjectByName("selectionDetailsIcon").size = "10 100%-104 90 100%-22";
		getGUIObjectByName("selectionDetailsHealth").size = "10 100%-20 90 100%-14";
		getGUIObjectByName("selectionDetailsStamina").size = "10 100%-12 90 100%-6";

		getGUIObjectByName("selectionDetailsAttack").size = "104 64 100% 100%-30";
		getGUIObjectByName("selectionDetailsArmour").size = "204 64 100% 100%-30";

		getGUIObjectByName("selectionDetailsAttack").hidden = false;
		getGUIObjectByName("selectionDetailsArmour").hidden = false;

		getGUIObjectByName("selectionDetailsMainText").sprite = "";
		getGUIObjectByName("selectionDetailsSpecific").sprite = "wheatWindowTitle";
}

// Fills out information that most entities have
function displayGeneralInfo(playerState, entState, template)
{
	var civName = toTitleCase(playerState.civ);
	var color = playerState.color;
	var playerColor = color["r"]*255 + " " + color["g"]*255 + " " + color["b"]*255 + " " + color["a"]*255;
	var iconTooltip = "";
	
	// Is unit Elite?
	var eliteStatus = isUnitElite(entState.template);
	
	// Specific Name
	var name = (eliteStatus?  "Elite " + template.name.specific : template.name.specific);
	getGUIObjectByName("selectionDetailsSpecific").caption = name;
	iconTooltip += "[font=\"serif-bold-16\"]" + name + "[/font]";

	// Generic Name
	if (template.name.generic != template.name.specific)
		getGUIObjectByName("selectionDetailsSpecific").tooltip = template.name.generic;
	else
		getGUIObjectByName("selectionDetailsSpecific").tooltip = "";

	// Player Name
	getGUIObjectByName("selectionDetailsPlayer").caption = playerState.name;
	getGUIObjectByName("selectionDetailsPlayer").tooltip = getFormalCivName(civName);
	getGUIObjectByName("selectionDetailsPlayer").textcolor = playerColor;

	// Hitpoints
	if (entState.hitpoints != undefined)
	{
		var healthSize = getGUIObjectByName("selectionDetailsHealthBar").size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		getGUIObjectByName("selectionDetailsHealthBar").size = healthSize;
		
		var tooltipHitPoints = "[font=\"serif-bold-13\"]Hitpoints [/font]" + entState.hitpoints + "/" + entState.maxHitpoints;
		getGUIObjectByName("selectionDetailsHealth").tooltip = tooltipHitPoints;
		getGUIObjectByName("selectionDetailsHealth").hidden = false;
		iconTooltip += "\n" + tooltipHitPoints;
	}
	else
	{
		getGUIObjectByName("selectionDetailsHealth").hidden = true;
		getGUIObjectByName("selectionDetailsHealth").tooltip = "";
	}

	// Attack stats
	getGUIObjectByName("selectionDetailsAttackStats").caption = damageTypesToTextStacked(entState.attack);
	if (entState.attack)
		iconTooltip += "\n" +  "[font=\"serif-bold-13\"]Attack: [/font]" + damageTypesToText(entState.attack);		

	// Armour stats
	getGUIObjectByName("selectionDetailsArmourStats").caption = damageTypesToTextStacked(entState.armour);
	if (entState.armour)
		iconTooltip += "\n" + "[font=\"serif-bold-13\"]Armour: [/font]" + damageTypesToText(entState.armour);	

	// Resources
	getGUIObjectByName("selectionDetailsResources").hidden = true;

	// Is this a Gaia unit?
	var firstWord = getTemplateFirstWord(entState.template);
	if (firstWord == "gaia")
	{
		if (civName == GAIA)
			getGUIObjectByName("selectionDetailsPlayer").tooltip = ""; // Don't need civ tooltip for Gaia Player - redundant

		// Resource stats
		if (entState.resourceSupply)
		{
			var resources = entState.resourceSupply.amount + "/" + entState.resourceSupply.max + " ";
			var resourceType = entState.resourceSupply.type["generic"];
		
			getGUIObjectByName("selectionDetailsResourceStats").caption =  resources;
			getGUIObjectByName("selectionDetailsResourceIcon").cell_id = resourceIconCellIds[resourceType];
			getGUIObjectByName("selectionDetailsResources").hidden = false;
			
			iconTooltip += "\n[font=\"serif-bold-13\"]Resources: [/font]" + resources + "[font=\"serif-12\"]" + resourceType + "[/font]";
			
			// Don't show attack and armour stats on unit with resources - not enough space
			getGUIObjectByName("selectionDetailsAttack").hidden = true;
			getGUIObjectByName("selectionDetailsArmour").hidden = true;
		}
	}

	// Icon
	getGUIObjectByName("selectionDetailsIconImage").sprite = getPortraitSheetName(playerState, entState);
	getGUIObjectByName("selectionDetailsIconImage").cell_id = template.icon_cell;
	getGUIObjectByName("selectionDetailsIconImage").tooltip = iconTooltip;
	
	//getGUIObjectByName("selectionDetailsIconOutline"); // Need to change color of icon outline with the playerColor
}

// Updates middle entity Selection Details Panel
function updateSelectionDetails(simState)
{
	var detailsPanel = getGUIObjectByName("selectionDetails");
	var commandsPanel = getGUIObjectByName("unitCommands");

	var selection = g_Selection.toList();
	if (selection.length == 0)
	{
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	/* If the unit has no data (e.g. it was killed), don't try displaying any
	 data for it. (TODO: it should probably be removed from the selection too;
	 also need to handle multi-unit selections) */
	var entState = Engine.GuiInterfaceCall("GetEntityState", selection[g_Selection.getPrimary()]);
	if (!entState)
	{
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	var playerState = simState.players[entState.player];
	if (!playerState)
		return;

	var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);
	
	// Different selection details are shown based on whether multiple units or a single unit is selected
	if (selection.length > 1)
		selectionLayoutMultiple();
	else
		selectionLayoutSingle();

	// Fill out general info and display it
	displayGeneralInfo(playerState, entState, template); // must come after layout functions

	// Show Panels
	detailsPanel.hidden = false;
	
	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(playerState, entState, commandsPanel, selection);
}
