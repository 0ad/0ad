function layoutSelectionSingle()
{
	getGUIObjectByName("detailsAreaSingle").hidden = false;
	getGUIObjectByName("detailsAreaMultiple").hidden = true;
}

function layoutSelectionMultiple()
{
	getGUIObjectByName("detailsAreaMultiple").hidden = false;
	getGUIObjectByName("detailsAreaSingle").hidden = true;
}

// Fills out information that most entities have
function displaySingle(entState, template)
{
	// Get general unit and player data
	var specificName = template.name.specific;
	var genericName = template.name.generic != template.name.specific? template.name.generic : "";
	var playerState = g_Players[entState.player];

	var civName = g_CivData[playerState.civ].Name;
	var civEmblem = g_CivData[playerState.civ].Emblem;

	var playerName = playerState.name;
	var playerColor = playerState.color.r + " " + playerState.color.g + " " + playerState.color.b + " 128";

	// Indicate disconnected players by prefixing their name
	if (g_Players[entState.player].offline)
	{
		playerName = "[OFFLINE] " + playerName;
	}

	// Rank
	if (entState.identity && entState.identity.rank && entState.identity.classes)
	{
		getGUIObjectByName("rankIcon").tooltip = entState.identity.rank + " Rank";
		getGUIObjectByName("rankIcon").sprite = getRankIconSprite(entState);					
		getGUIObjectByName("rankIcon").hidden = false;
	}
	else
	{
		getGUIObjectByName("rankIcon").hidden = true;
		getGUIObjectByName("rankIcon").tooltip = "";
	}
								
	// Hitpoints
	if (entState.hitpoints)
	{
		var unitHealthBar = getGUIObjectByName("healthBar");
		var healthSize = unitHealthBar.size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		unitHealthBar.size = healthSize;

		var hitpoints = entState.hitpoints + " / " + entState.maxHitpoints;
		getGUIObjectByName("healthStats").caption = hitpoints;
		getGUIObjectByName("healthSection").hidden = false;
	}
	else
	{
		getGUIObjectByName("healthSection").hidden = true;
	}
	
	// TODO: Stamina
	var player = Engine.GetPlayerID();
	if (entState.stamina && (entState.player == player || g_DevSettings.controlAll))
	{
		getGUIObjectByName("staminaSection").hidden = false;
	}
	else
	{
		getGUIObjectByName("staminaSection").hidden = true;
	}

	// Experience
	if (entState.promotion)
	{
		var experienceBar = getGUIObjectByName("experienceBar");
		var experienceSize = experienceBar.size;
		experienceSize.rtop = 100 - (100 * Math.max(0, Math.min(1, 1.0 * +entState.promotion.curr / +entState.promotion.req)));
		experienceBar.size = experienceSize;
 
		var experience = "[font=\"serif-bold-13\"]Experience: [/font]" + Math.floor(entState.promotion.curr);
		if (entState.promotion.curr < entState.promotion.req)
			experience += " / " + entState.promotion.req;
		getGUIObjectByName("experience").tooltip = experience;
		getGUIObjectByName("experience").hidden = false;
	}
	else
	{
		getGUIObjectByName("experience").hidden = true;
	}

	// Resource stats
	if (entState.resourceSupply)
	{
		var resources = Math.ceil(+entState.resourceSupply.amount) + " / " + entState.resourceSupply.max;
		var resourceType = entState.resourceSupply.type["generic"];
		if (resourceType == "treasure")
			resourceType = entState.resourceSupply.type["specific"];

		var unitResourceBar = getGUIObjectByName("resourceBar");
		var resourceSize = unitResourceBar.size;

		resourceSize.rright = 100 * Math.max(0, Math.min(1, +entState.resourceSupply.amount / +entState.resourceSupply.max));
		unitResourceBar.size = resourceSize;
		getGUIObjectByName("resourceLabel").caption = toTitleCase(resourceType) + ":";
		getGUIObjectByName("resourceStats").caption = resources;

		if (entState.hitpoints)
			getGUIObjectByName("resourceSection").size = getGUIObjectByName("staminaSection").size;
		else
			getGUIObjectByName("resourceSection").size = getGUIObjectByName("healthSection").size;

		getGUIObjectByName("resourceSection").hidden = false;
	}
	else
	{
		getGUIObjectByName("resourceSection").hidden = true;
	}

	// Resource carrying
	if (entState.resourceCarrying && entState.resourceCarrying.length)
	{
		// We should only be carrying one resource type at once, so just display the first
		var carried = entState.resourceCarrying[0];

		getGUIObjectByName("resourceCarryingIcon").hidden = false;
		getGUIObjectByName("resourceCarryingText").hidden = false;
		getGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/resources/"+carried.type+".png";
		getGUIObjectByName("resourceCarryingText").caption = carried.amount + " / " + carried.max;
	}
	// Use the same indicators for traders
	else if (entState.trader && entState.trader.goods.amount > 0)
	{
		getGUIObjectByName("resourceCarryingIcon").hidden = false;
		getGUIObjectByName("resourceCarryingText").hidden = false;
		getGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/resources/"+entState.trader.goods.type+".png";
		getGUIObjectByName("resourceCarryingText").caption = entState.trader.goods.amount;
	}
	else
	{
		getGUIObjectByName("resourceCarryingIcon").hidden = true;
		getGUIObjectByName("resourceCarryingText").hidden = true;
	}

	// Set Player details
	getGUIObjectByName("specific").caption = specificName;
		getGUIObjectByName("player").caption = playerName;
	getGUIObjectByName("playerColorBackground").sprite = "colour: " + playerColor;
	
	if (genericName)
	{
		getGUIObjectByName("generic").caption = "(" + genericName + ")";
	}
	else
	{
		getGUIObjectByName("generic").caption = "";

	}

	if ("Gaia" != civName)
	{
		getGUIObjectByName("playerCivIcon").sprite = "stretched:grayscale:" + civEmblem;
		getGUIObjectByName("player").tooltip = civName;
	}
	else
	{
		getGUIObjectByName("playerCivIcon").sprite = "";
		getGUIObjectByName("player").tooltip = "";
	}

	// Icon image
	if (template.icon)
	{
		getGUIObjectByName("icon").sprite = "stretched:session/portraits/" + template.icon;
	}
	else
	{
		// TODO: we should require all entities to have icons, so this case never occurs
		getGUIObjectByName("icon").sprite = "bkFillBlack";
	}

	// Attack and Armor
	getGUIObjectByName("attackAndArmorStats").tooltip = "[font=\"serif-bold-13\"]Attack:[/font] " + damageTypeDetails(entState.attack) + 
							    "\n[font=\"serif-bold-13\"]Armor:[/font] " + damageTypeDetails(entState.armour);

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
	}

	if (averageHealth > 0)
	{
		var unitHealthBar = getGUIObjectByName("healthBarMultiple");
		var healthSize = unitHealthBar.size;	
		healthSize.rtop = 100-100*Math.max(0, Math.min(1, averageHealth / maxHealth));
		unitHealthBar.size = healthSize;

		var hitpoints = "[font=\"serif-bold-13\"]Hitpoints [/font]" + averageHealth + " / " + maxHealth;
		var healthMultiple = getGUIObjectByName("healthMultiple");
		healthMultiple.tooltip = hitpoints;
		healthMultiple.hidden = false;	}
	else
	{
		getGUIObjectByName("healthMultiple").hidden = true;
	}
	
	// TODO: Stamina
	// getGUIObjectByName("staminaBarMultiple");

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
		getGUIObjectByName("detailsAreaMultiple").hidden = true;
		getGUIObjectByName("detailsAreaSingle").hidden = true;
		hideUnitCommands();
	
		supplementalDetailsPanel.hidden = true;
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
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

	// Show Panels
	supplementalDetailsPanel.hidden = false;
	detailsPanel.hidden = false;
	commandsPanel.hidden = false;

	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection);
}
