const resourceIconCellIds = {food : 0, wood : 1, stone : 2, metal : 3};

// Multiple Selection Layout
function selectionLayoutMultiple()
{
	getGUIObjectByName("sdMainText").size = "80 100%-70 100%-14 100%-10";
	getGUIObjectByName("sdSpecific").size = "0 6 100% 30";
	getGUIObjectByName("sdPlayer").size = "0 34 100% 100%-8";		
		
	getGUIObjectByName("sdIcon").size = "10 100%-74 66 100%-18";
	getGUIObjectByName("sdHealth").size = "10 100%-16 66 100%-12";
	getGUIObjectByName("sdStamina").size = "10 100%-10 66 100%-6";

	getGUIObjectByName("sdAttack").hidden = true;
	getGUIObjectByName("sdArmour").hidden = true;

	getGUIObjectByName("sdMainText").sprite = "goldPanel";
	getGUIObjectByName("sdSpecific").sprite = "";
}

// Single Selection Layout
function selectionLayoutSingle()
{
	getGUIObjectByName("sdMainText").size = "6 0 100%-6 50";
	getGUIObjectByName("sdRankIcon").size = "0 0 32 32";
	getGUIObjectByName("sdSpecific").size = "0 0 100% 30";
	getGUIObjectByName("sdPlayer").size = "0 30 100% 50";

	getGUIObjectByName("sdIcon").size = "10 100%-102 90 100%-22";
	getGUIObjectByName("sdHealth").size = "10 100%-20 90 100%-14";
	getGUIObjectByName("sdStamina").size = "10 100%-12 90 100%-6";

	getGUIObjectByName("sdAttack").size = "104 64 100% 100%-30";
	getGUIObjectByName("sdArmour").size = "204 64 100% 100%-30";

	getGUIObjectByName("sdAttack").hidden = false;
	getGUIObjectByName("sdArmour").hidden = false;

	getGUIObjectByName("sdMainText").sprite = "";
	getGUIObjectByName("sdSpecific").sprite = "";
}

// Fills out information that most entities have
function displayGeneralInfo(playerState, entState, template)
{
	var civName = toTitleCase(playerState.civ);
	var color = playerState.color;
	var playerColor = color["r"]*255 + " " + color["g"]*255 + " " + color["b"]*255 + " " + color["a"]*255;
	var iconTooltip = "";
	
	// Rank Icon
	var rankId = getRankCellId(entState.template);
	getGUIObjectByName("sdRankIcon").cell_id = rankId;
	getGUIObjectByName("sdRankIcon").tooltip = getRankTitle(rankId);
	
	// Specific Name
	var name = template.name.specific; // (eliteStatus?  "Elite " + template.name.specific : template.name.specific);
	getGUIObjectByName("sdSpecific").caption = name;
	iconTooltip += "[font=\"serif-bold-16\"]" + name + "[/font]";

	// Generic Name
	if (template.name.generic != template.name.specific)
		getGUIObjectByName("sdSpecific").tooltip = template.name.generic;
	else
		getGUIObjectByName("sdSpecific").tooltip = "";

	// Player Name
	getGUIObjectByName("sdPlayer").caption = playerState.name;
	getGUIObjectByName("sdPlayer").tooltip = getFormalCivName(civName);
	getGUIObjectByName("sdPlayer").textcolor = playerColor;

	// Hitpoints
	if (entState.hitpoints != undefined)
	{
		var healthSize = getGUIObjectByName("sdHealthBar").size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		getGUIObjectByName("sdHealthBar").size = healthSize;
		
		var tooltipHitPoints = "[font=\"serif-bold-13\"]Hitpoints [/font]" + entState.hitpoints + "/" + entState.maxHitpoints;
		getGUIObjectByName("sdHealth").tooltip = tooltipHitPoints;
		getGUIObjectByName("sdHealth").hidden = false;
		iconTooltip += "\n" + tooltipHitPoints;
	}
	else
	{
		getGUIObjectByName("sdHealth").hidden = true;
		getGUIObjectByName("sdHealth").tooltip = "";
	}

	// Attack stats
	getGUIObjectByName("sdAttackStats").caption = damageTypesToTextStacked(entState.attack);
	if (entState.attack)
		iconTooltip += "\n" +  "[font=\"serif-bold-13\"]Attack: [/font]" + damageTypesToText(entState.attack);		

	// Armour stats
	getGUIObjectByName("sdArmourStats").caption = damageTypesToTextStacked(entState.armour);
	if (entState.armour)
		iconTooltip += "\n" + "[font=\"serif-bold-13\"]Armour: [/font]" + damageTypesToText(entState.armour);	

	// Resource stats
	if (entState.resourceSupply)
	{
		var resources = entState.resourceSupply.amount + "/" + entState.resourceSupply.max + " ";
		var resourceType = entState.resourceSupply.type["generic"];
		
		getGUIObjectByName("sdResourceStats").caption =  resources;
		getGUIObjectByName("sdResourceIcon").cell_id = resourceIconCellIds[resourceType];
		getGUIObjectByName("sdResources").hidden = false;
			
		iconTooltip += "\n[font=\"serif-bold-13\"]Resources: [/font]" + resources + "[font=\"serif-12\"]" + resourceType + "[/font]";
			
		// Don't show attack and armour stats on unit with resources - not enough space
		getGUIObjectByName("sdAttack").hidden = true;
		getGUIObjectByName("sdArmour").hidden = true;
	}
	else
	{
		getGUIObjectByName("sdResources").hidden = true;
	}

	// Icon
	getGUIObjectByName("sdIconImage").sprite = getPortraitSheetName(getTemplateCategory(entState.template));
	getGUIObjectByName("sdIconImage").cell_id = template.icon_cell;
	getGUIObjectByName("sdIconImage").tooltip = iconTooltip;
	//getGUIObjectByName("sdIconOutline"); // Need to change color of icon outline with the playerColor

	// Is this a Gaia unit?
	if (civName == GAIA)
		getGUIObjectByName("sdPlayer").tooltip = ""; // Don't need civ tooltip for Gaia Player - redundant
}

// Updates middle entity Selection Details Panel
function updateSelectionDetails(simState)
{
	var detailsPanel = getGUIObjectByName("selectionDetails");
	var commandsPanel = getGUIObjectByName("unitCommands");

	g_Selection.updateSelection();
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
		return;

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
