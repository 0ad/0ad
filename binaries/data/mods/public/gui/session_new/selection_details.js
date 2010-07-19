const resourceIconCellIds = {food : 0, wood : 1, stone : 2, metal : 3};

// Cycle through the units in the main icon
var cycleIndex = 0;
var displayedCycleIndex = 1;

function resetCycleIndex()
{
	cycleIndex = 0;
	displayedCycleIndex = 1;
}

function cycleThroughSelection(forward)
{
	var selection = g_Selection.toList();
	
	if (selection.length > 1)
	{
		var primaryTemplateName = g_Selection.getPrimaryTemplateName();
		var primaryIndex = g_Selection.getPrimary();
		var startIndex = cycleIndex;
		var endIndex = selection.length-1;

		if (forward)
			cycleIndex = ((cycleIndex < endIndex)? cycleIndex+1 : 0);
		else
			cycleIndex = ((cycleIndex > 0)? cycleIndex-1 : endIndex);

		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[cycleIndex]);
		if (!entState)
			return;

		while (cycleIndex != startIndex)
		{
			var equivalentTemplateNames;
			if (g_GroupSelectionByRank)
				equivalentTemplateNames = templatesEqualWithoutRank(primaryTemplateName, entState.template);
			else
				equivalentTemplateNames = (primaryTemplateName == entState.template);

			if ((cycleIndex != primaryIndex) && equivalentTemplateNames)
			{
				var typeCount = g_Selection.groups.getGroup(entState.template).typeCount;
				
				if (forward)
					displayedCycleIndex = ((displayedCycleIndex < typeCount)? displayedCycleIndex+1 : 1);
				else
					displayedCycleIndex = ((displayedCycleIndex > 1)? displayedCycleIndex-1 : typeCount);
					
				g_Selection.setPrimary(cycleIndex);
				break;
			}

			if (forward)
				cycleIndex = ((cycleIndex < endIndex)? cycleIndex+1 : 0);
			else
				cycleIndex = ((cycleIndex > 0)? cycleIndex-1 : endIndex);

			entState = Engine.GuiInterfaceCall("GetEntityState", selection[cycleIndex]);
			if (!entState)
				return;
		}
	}
}



function cycleThroughSelection1()
{
	var selection = g_Selection.toList();
	
	if (selection.length > 1)
	{
		var primaryTemplateName = g_Selection.getPrimaryTemplateName();
		var primaryIndex = g_Selection.getPrimary();
		var startIndex = cycleIndex;
		var endIndex = selection.length-1;

		cycleIndex = ((cycleIndex < endIndex)? cycleIndex+1 : 0);
		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[cycleIndex]);
		if (!entState)
			return;

		while (cycleIndex != startIndex)
		{
			var equivalentTemplateNames;
			if (g_GroupSelectionByRank)
				equivalentTemplateNames = templatesEqualWithoutRank(primaryTemplateName, entState.template);
			else
				equivalentTemplateNames = (primaryTemplateName == entState.template);

			if ((cycleIndex != primaryIndex) && equivalentTemplateNames)
			{
				var typeCount = g_Selection.groups.getGroup(entState.template).typeCount;
				displayedCycleIndex = ((displayedCycleIndex < typeCount)? displayedCycleIndex+1 : 1);
				g_Selection.setPrimary(cycleIndex);
				break;
			}

			cycleIndex = ((cycleIndex < endIndex)? cycleIndex+1 : 0);
			entState = Engine.GuiInterfaceCall("GetEntityState", selection[cycleIndex]);
			if (!entState)
				return;
		}
	}
}

function reverseCycleThroughSelection()
{
	var selection = g_Selection.toList();
	
	if (selection.length > 1)
	{
		var primaryTemplateName = g_Selection.getPrimaryTemplateName();
		var primaryIndex = g_Selection.getPrimary();
		var startIndex = cycleIndex;
		var endIndex = selection.length-1;

		cycleIndex = ((cycleIndex > 0)? cycleIndex-1 : endIndex);
		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[cycleIndex]);
		if (!entState)
			return;

		while (cycleIndex != startIndex)
		{
			var equivalentTemplateNames;
			if (g_GroupSelectionByRank)
				equivalentTemplateNames = templatesEqualWithoutRank(primaryTemplateName, entState.template);
			else
				equivalentTemplateNames = (primaryTemplateName == entState.template);

			if ((cycleIndex != primaryIndex) && equivalentTemplateNames)
			{
				var typeCount = g_Selection.groups.getGroup(entState.template).typeCount;
				displayedCycleIndex = ((displayedCycleIndex > 1)? displayedCycleIndex-1 : typeCount);
				g_Selection.setPrimary(cycleIndex);
				break;
			}

			cycleIndex = ((cycleIndex > 0)? cycleIndex-1 : endIndex);
			entState = Engine.GuiInterfaceCall("GetEntityState", selection[cycleIndex]);
			if (!entState)
				return;
		}
	}
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

	var rankText = getRankTitle(rankId);
	rankText = (rankText? " (" + rankText + ")" : "" );

	// Specific Name
	var name = template.name.specific + rankText; // (eliteStatus?  "Elite " + template.name.specific : template.name.specific);
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
		iconTooltip += "\n[font=\"serif-bold-13\"]Attack: [/font]" + damageTypesToText(entState.attack);	

	// Armour stats
	getGUIObjectByName("sdArmourStats").caption = damageTypesToTextStacked(entState.armour);
	if (entState.armour)
		iconTooltip += "\n[font=\"serif-bold-13\"]Armour: [/font]" + damageTypesToText(entState.armour);

	// Resource stats
	if (entState.resourceSupply)
	{
		var resources = entState.resourceSupply.amount + "/" + entState.resourceSupply.max + " ";
		var resourceType = entState.resourceSupply.type["generic"];
		
		getGUIObjectByName("sdResourceStats").caption = resources;
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
		getGUIObjectByName("sdAttack").hidden = false;
		getGUIObjectByName("sdArmour").hidden = false;
	}

	// Icon
	getGUIObjectByName("sdIconImage").sprite = template.icon_sheet;
	getGUIObjectByName("sdIconImage").cell_id = template.icon_cell;
	getGUIObjectByName("sdIcon").tooltip = iconTooltip;
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

	// Choose the highest ranked version of the primary selection
	// Different selection details are shown based on whether multiple units or a single unit is selected
	if (selection.length > 1)
	{
		var selectionGroup = g_Selection.groups.getGroup(entState.template);
		var typeCount = g_Selection.groups.getGroup(entState.template).typeCount;
		getGUIObjectByName("sdSelectionCount").caption = ((typeCount > 1)? displayedCycleIndex + "/" + typeCount : "");
		
		var visible = !(typeCount > 1);
		getGUIObjectByName("sdCycleArea").hidden = visible;
		getGUIObjectByName("sdStatsArea").hidden = true;
	}
	else
	{
		getGUIObjectByName("sdSelectionCount").caption = "";
		getGUIObjectByName("sdCycleArea").hidden = true;
		getGUIObjectByName("sdStatsArea").hidden = false;
	}

	var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);

	// Fill out general info and display it
	displayGeneralInfo(playerState, entState, template); // must come after layout functions

	// Show Panels
	detailsPanel.hidden = false;
	
	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(playerState, entState, commandsPanel, selection);
}
