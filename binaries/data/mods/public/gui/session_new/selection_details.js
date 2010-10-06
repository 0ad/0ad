const RESOURCE_ICON_CELL_IDS = {food : 0, wood : 1, stone : 2, metal : 3};

function layoutSelectionSingle()
{
	getGUIObjectByName("detailsAreaSingle").hidden = false;
	getGUIObjectByName("detailsAreaMultiple").hidden = true;

//	getGUIObjectByName("specific").hidden = false;
//	getGUIObjectByName("iconBorder").hidden = false;

	//getGUIObjectByName("attackIcon").size = "0 0 48 48";
	//getGUIObjectByName("armourIcon").size = "0 48 48 96";
	//getGUIObjectByName("barsArea").size = "50%+48 40 100% 136"
}

function layoutSelectionMultiple()
{
//	getGUIObjectByName("specific").hidden = true;
//	getGUIObjectByName("iconBorder").hidden = true;

	//getGUIObjectByName("attackIcon").size = "-4 10 32 46";
	//getGUIObjectByName("armourIcon").size = "-4 46 32 82";
	//getGUIObjectByName("barsArea").size = "50%+60 40 100% 136"
	getGUIObjectByName("detailsAreaMultiple").hidden = false;
	getGUIObjectByName("detailsAreaSingle").hidden = true;
}

// Fills out information that most entities have
function displaySingle(entState, template)
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
		getGUIObjectByName("health").hidden = false;
	}
	else
	{
		getGUIObjectByName("health").hidden = true;
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

	// Unhide Details Area
	getGUIObjectByName("detailsAreaSingle").hidden = false;
	getGUIObjectByName("detailsAreaMultiple").hidden = true;
}

// Fills out information for multiple entities
function displayMultiple(selection, template)
{
	var averageHealth = 0;
	var maxHealth = 0;

	for (var i = 0; i < selection.length; i++)
	{
		var entState = GetEntityState(selection[i])
		if (entState)
		{
			if (entState.hitpoints)
			{
				averageHealth += entState.hitpoints;
				maxHealth += entState.maxHitpoints;
			}
		}

		if (i == selection.length-1)
		{
			var unitHealthBar = getGUIObjectByName("healthBarMultiple");
			var healthSize = unitHealthBar.size;
							
			healthSize.rtop = 100-100*Math.max(0, Math.min(1, averageHealth / maxHealth));
			unitHealthBar.size = healthSize;
			
			var hitpoints = "[font=\"serif-bold-13\"]Hitpoints [/font]" + averageHealth + "/" + maxHealth;
			getGUIObjectByName("healthMultiple").tooltip = hitpoints;
		}
	}

	getGUIObjectByName("numberOfUnits").caption = selection.length;

	// Unhide Details Area
	getGUIObjectByName("detailsAreaMultiple").hidden = false;
	getGUIObjectByName("detailsAreaSingle").hidden = true;
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

	var template = GetTemplateData(entState.template);

	// Fill out general info and display it
	if (selection.length == 1)
		displaySingle(entState, template);
	else
		displayMultiple(selection, template);

	var player = Engine.GetPlayerID();
	if (entState.player == player || g_DevSettings.controlAll)
	{
		//if (entState.stamina != undefined)
			getGUIObjectByName("stamina").hidden = false;
		//else
		//	getGUIObjectByName("stamina").hidden = true;
	}

	// Show Panels
	detailsPanel.hidden = false;
	
	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection);
}
