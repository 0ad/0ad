const RESOURCE_ICON_CELL_IDS = {food : 0, wood : 1, stone : 2, metal : 3};

function layoutSelectionMultiple()
{
//	getGUIObjectByName("specific").hidden = true;
//	getGUIObjectByName("iconBorder").hidden = true;
//	getGUIObjectByName("statsArea").hidden = true;
//	getGUIObjectByName("health").hidden = true;
//	getGUIObjectByName("stamina").hidden = true;


	getGUIObjectByName("detailsArea").hidden = true;

}

function layoutSelectionSingle(entState)
{
	getGUIObjectByName("detailsArea").hidden = false;


	getGUIObjectByName("specific").hidden = false;
	getGUIObjectByName("iconBorder").hidden = false;
//	getGUIObjectByName("sdStatsArea").hidden = false;

	if (entState.hitpoints != undefined)
		getGUIObjectByName("health").hidden = false;
	else
		getGUIObjectByName("health").hidden = true;

	var player = Engine.GetPlayerID();
	if (entState.player == player || g_DevSettings.controlAll)
	{
		//if (entState.stamina != undefined)
			getGUIObjectByName("stamina").hidden = false;
		//else
		//	getGUIObjectByName("stamina").hidden = true;
	}
}

// Fills out information that most entities have
function displayGeneralInfo(entState, template)
{
	// Get general unit and player data
	var specificName = template.name.specific;
	var genericName = template.name.generic != template.name.specific? template.name.generic : "";

	var civName = getFormalCivName(toTitleCase(g_Players[entState.player].civ));

	var playerName = g_Players[entState.player].name;
	var playerColor = g_Players[entState.player].color.r + " " + g_Players[entState.player].color.g + " " +
								g_Players[entState.player].color.b+ " " + g_Players[entState.player].color.a;

	// Rank					
	getGUIObjectByName("rankIcon").cell_id = getRankIconCellId(entState);							
								
	// Hitpoints
	var hitpoints = "";

	if (entState.hitpoints)
	{
		var unitHealthBar = getGUIObjectByName("healthBar");
		var healthSize = unitHealthBar.size;
		healthSize.rtop = 100-100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		unitHealthBar.size = healthSize;

		hitpoints = "[font=\"serif-bold-13\"]Hitpoints [/font]" + entState.hitpoints + "/" + entState.maxHitpoints;
		getGUIObjectByName("health").tooltip = hitpoints;
	}

	// Resource stats
	var resources = "";
	var resourceType = "";

	if (entState.resourceSupply)
	{
		resources = Math.ceil(+entState.resourceSupply.amount) + "/" + entState.resourceSupply.max;
		resourceType = entState.resourceSupply.type["generic"];

		var unitResourceBar = getGUIObjectByName("resourceBar");
		var resourceSize = unitResourceBar.size;

		resourceSize.rtop = 100-100*Math.max(0, Math.min(1, +entState.resourceSupply.amount / entState.resourceSupply.max));
		unitResourceBar.size = resourceSize;

		var unitResources = getGUIObjectByName("resources");
		unitResources.tooltip = "[font=\"serif-bold-13\"]Resources: [/font]" + resources + " " + resourceType;

		if (!entState.hitpoints)
			unitResources.size = getGUIObjectByName("health").size;
		else
			unitResources.size = getGUIObjectByName("stamina").size;

		getGUIObjectByName("resources").hidden = false;
	}
	else
	{
		getGUIObjectByName("resources").hidden = true;
	}
	
	// Set Captions
	getGUIObjectByName("specific").caption = specificName;
	getGUIObjectByName("player").caption = civName == GAIA? playerName : playerName + " (" + civName + ")"; // Don't need civ tooltip for Gaia Player - redundant
//	getGUIObjectByName("player").textcolor = playerColor;


	// Icon image
	if (template.icon_sheet && typeof template.icon_cell)
	{
		getGUIObjectByName("icon").sprite = template.icon_sheet;
		getGUIObjectByName("icon").cell_id = template.icon_cell;
	}
	else
	{
		// TODO: we should require all entities to have icons, so this case never occurs
		getGUIObjectByName("icon").sprite = "bkFillBlack";
	}

	// Tooltips
//	getGUIObjectByName("specific").tooltip = genericName;
	getGUIObjectByName("health").tooltip = hitpoints;
	getGUIObjectByName("attackIcon").tooltip = damageTypesToText(entState.attack);
	getGUIObjectByName("armourIcon").tooltip = damageTypesToText(entState.armour);


	// Icon Tooltip
	var iconTooltip = "";

	if (genericName)
		iconTooltip = "[font=\"serif-bold-16\"]" + genericName + "[/font]";

	if (template.tooltip)
		iconTooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

	getGUIObjectByName("iconBorder").tooltip = iconTooltip;
}

// Updates middle entity Selection Details Panel
function updateSelectionDetails()
{
	var supplementalDetailsPanel = getGUIObjectByName("supplementalSelectionDetails");
	var detailsPanel = getGUIObjectByName("selectionDetails");
	var commandsPanel = getGUIObjectByName("unitCommands");

	g_Selection.update();
	var selection = g_Selection.toList();
	
	if (selection.length == 0)
	{
		supplementalDetailsPanel.hidden = true;
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		getGUIObjectByName("unitSelectionPanel").hidden = true;
		return;
	}

	/* If the unit has no data (e.g. it was killed), don't try displaying any
	 data for it. (TODO: it should probably be removed from the selection too;
	 also need to handle multi-unit selections) */
	var entState = GetEntityState(selection[0]);
	if (!entState)
		return;

	// Choose the highest ranked version of the primary selection
	// Different selection details are shown based on whether multiple units or a single unit is selected
	/*
	if (selection.length > 1)
		layoutSelectionMultiple();
	else
		layoutSelectionSingle(entState);
	*/

	var template = GetTemplateData(entState.template);

	// Fill out general info and display it
	if (selection.length == 1)
	{
		displayGeneralInfo(entState, template); // must come after layout functions
		getGUIObjectByName("detailsArea").hidden = false;
	}
	else
	{
		getGUIObjectByName("detailsArea").hidden = true;
	}

	// Show Panels
	detailsPanel.hidden = false;
	
	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection);
}
