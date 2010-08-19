const RESOURCE_ICON_CELL_IDS = {food : 0, wood : 1, stone : 2, metal : 3};

function layoutSelectionMultiple()
{
	getGUIObjectByName("sdSpecific").hidden = true;
	getGUIObjectByName("sdIcon").hidden = true;
	getGUIObjectByName("sdStatsArea").hidden = true;
	getGUIObjectByName("sdHealth").hidden = true;
	getGUIObjectByName("sdStamina").hidden = true;
}

function layoutSelectionSingle(entState)
{
	getGUIObjectByName("sdSpecific").hidden = false;
	getGUIObjectByName("sdIcon").hidden = false;
	getGUIObjectByName("sdStatsArea").hidden = false;
	
	if (entState.hitpoints != undefined)
		getGUIObjectByName("sdHealth").hidden = false;
	else
		getGUIObjectByName("sdHealth").hidden = true;

	var player = Engine.GetPlayerID();
	if (entState.player == player || g_DevSettings.controlAll)
	{
		if (entState.stamina != undefined)
			getGUIObjectByName("sdStamina").hidden = false;
		else
			getGUIObjectByName("sdStamina").hidden = true;
	}
}

// Fills out information that most entities have
function displayGeneralInfo(entState, template)
{
	// Get general unit and player data
	var specificName = "[font=\"serif-bold-18\"]" + template.name.specific + "[/font]";
	var genericName = template.name.generic != template.name.specific? template.name.generic : "";

	var rank = entState.identity.rank? "[font=\"serif-bold-18\"]" + entState.identity.rank + " [/font]" : "";
	var civName = getFormalCivName(toTitleCase(g_Players[entState.player].civ));

	var playerName = g_Players[entState.player].name;
	var playerColor = g_Players[entState.player].color.r + " " + g_Players[entState.player].color.g + " " +
								g_Players[entState.player].color.b+ " " + g_Players[entState.player].color.a;

	// Hitpoints
	var hitpoints = "";

	if (entState.hitpoints)
	{
		var unitHealthBar = getGUIObjectByName("sdHealthBar");
		var healthSize = unitHealthBar.size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		unitHealthBar.size = healthSize;
		
		hitpoints = "[font=\"serif-bold-13\"]Hitpoints [/font]" + entState.hitpoints + "/" + entState.maxHitpoints;
		getGUIObjectByName("sdHealth").tooltip = hitpoints;
	}

	// Resource stats
	var resources = "";
	var resourceType = "";
	var resourceCellID = -1;

	if (entState.resourceSupply)
	{
		resources = Math.ceil(+entState.resourceSupply.amount) + "/" + entState.resourceSupply.max + " ";
		resourceType = entState.resourceSupply.type["generic"];
		resourceCellID = RESOURCE_ICON_CELL_IDS[resourceType];

		getGUIObjectByName("sdResources").hidden = false;
		getGUIObjectByName("sdAttack").hidden = true;
		getGUIObjectByName("sdArmour").hidden = true;
	}
	else
	{
		getGUIObjectByName("sdResources").hidden = true;
		getGUIObjectByName("sdAttack").hidden = false;
		getGUIObjectByName("sdArmour").hidden = false;
	}

	// Set Captions
	getGUIObjectByName("sdSpecific").caption = rank + specificName;
	getGUIObjectByName("sdPlayer").caption = playerName;
	getGUIObjectByName("sdPlayer").textcolor = playerColor;
	getGUIObjectByName("sdAttackStats").caption = damageTypesToTextStacked(entState.attack);
	getGUIObjectByName("sdArmourStats").caption = damageTypesToTextStacked(entState.armour);
	getGUIObjectByName("sdResourceStats").caption = resources;
	getGUIObjectByName("sdResourceIcon").cell_id = resourceCellID;

	// Icon image
	if (template.icon_sheet && typeof template.icon_cell)
	{
		getGUIObjectByName("sdIconImage").sprite = template.icon_sheet;
		getGUIObjectByName("sdIconImage").cell_id = template.icon_cell;
	}
	else
	{
		// TODO: we should require all entities to have icons, so this case never occurs
		getGUIObjectByName("sdIconImage").sprite = "bkFillBlack";
	}

	// TODO: need to change color of icon outline with the playerColor
	//getGUIObjectByName("sdIconOutline");

	// Tooltips
//	getGUIObjectByName("sdSpecific").tooltip = genericName;
	getGUIObjectByName("sdPlayer").tooltip = civName != GAIA? civName : ""; // Don't need civ tooltip for Gaia Player - redundant
	getGUIObjectByName("sdHealth").tooltip = hitpoints;

	// Icon Tooltip
	var iconTooltip = "";
	
	if (genericName)
		iconTooltip = "[font=\"serif-bold-16\"]" + genericName + "[/font]";

	if (template.tooltip)
		iconTooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";
		
	/*
	if (entState.hitpoints)
		iconTooltip += "\n" + hitpoints;
	if (entState.attack)
		iconTooltip += "\n[font=\"serif-bold-13\"]Attack: [/font]" + damageTypesToText(entState.attack);
	if (entState.armour)
		iconTooltip += "\n[font=\"serif-bold-13\"]Armour: [/font]" + damageTypesToText(entState.armour);
	if (entState.resourceSupply)
		iconTooltip += "\n[font=\"serif-bold-13\"]Resources: [/font]" + resources + "[font=\"serif-12\"]" + resourceType + "[/font]";
	*/

	getGUIObjectByName("sdIcon").tooltip = iconTooltip;
}

// Updates middle entity Selection Details Panel
function updateSelectionDetails()
{
	var detailsPanel = getGUIObjectByName("selectionDetails");
	var commandsPanel = getGUIObjectByName("unitCommands");

	g_Selection.update();
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
	var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
	if (!entState)
		return;

	// Choose the highest ranked version of the primary selection
	// Different selection details are shown based on whether multiple units or a single unit is selected
	if (selection.length > 1)
		layoutSelectionMultiple();
	else
		layoutSelectionSingle(entState);

	var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);

	// Fill out general info and display it
	displayGeneralInfo(entState, template); // must come after layout functions

	// Show Panels
	detailsPanel.hidden = false;
	
	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(entState, commandsPanel, selection);
}
