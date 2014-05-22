function layoutSelectionSingle()
{
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
}

function layoutSelectionMultiple()
{
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = true;
}

function getResourceTypeDisplayName(resourceType)
{
	var resourceCode = resourceType["generic"];
	var displayName = "";
	if (resourceCode == "treasure")
		displayName = getLocalizedResourceName(resourceType["specific"], "firstWord");
	else
		displayName = getLocalizedResourceName(resourceCode, "firstWord");
	return displayName;
}

// Fills out information that most entities have
function displaySingle(entState, template)
{
	// Get general unit and player data
	var specificName = template.name.specific;
	var genericName = template.name.generic != template.name.specific ? template.name.generic : "";
	// If packed, add that to the generic name (reduces template clutter)
	if (genericName && template.pack && template.pack.state == "packed")
		genericName = sprintf(translate("%(genericName)s — Packed"), { genericName: genericName });
	var playerState = g_Players[entState.player];

	var civName = g_CivData[playerState.civ].Name;
	var civEmblem = g_CivData[playerState.civ].Emblem;

	var playerName = playerState.name;
	var playerColor = playerState.color.r + " " + playerState.color.g + " " + playerState.color.b + " 128";

	// Indicate disconnected players by prefixing their name
	if (g_Players[entState.player].offline)
	{
		playerName = sprintf(translate("[OFFLINE] %(player)s"), { player: playerName });
	}

	// Rank
	if (entState.identity && entState.identity.rank && entState.identity.classes)
	{
		Engine.GetGUIObjectByName("rankIcon").tooltip = sprintf(translate("%(rank)s Rank"), { rank: translateWithContext("Rank", entState.identity.rank) });
		Engine.GetGUIObjectByName("rankIcon").sprite = getRankIconSprite(entState);
		Engine.GetGUIObjectByName("rankIcon").hidden = false;
	}
	else
	{
		Engine.GetGUIObjectByName("rankIcon").hidden = true;
		Engine.GetGUIObjectByName("rankIcon").tooltip = "";
	}
								
	// Hitpoints
	if (entState.hitpoints)
	{
		var unitHealthBar = Engine.GetGUIObjectByName("healthBar");
		var healthSize = unitHealthBar.size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		unitHealthBar.size = healthSize;

		Engine.GetGUIObjectByName("healthStats").caption = sprintf(translate("%(hitpoints)s / %(maxHitpoints)s"), {
			hitpoints: Math.ceil(entState.hitpoints),
			maxHitpoints: entState.maxHitpoints
		});
		Engine.GetGUIObjectByName("healthSection").hidden = false;
	}
	else
	{
		Engine.GetGUIObjectByName("healthSection").hidden = true;
	}
	
	// TODO: Stamina
	var player = Engine.GetPlayerID();
	if (entState.stamina && (entState.player == player || g_DevSettings.controlAll))
		Engine.GetGUIObjectByName("staminaSection").hidden = false;
	else
		Engine.GetGUIObjectByName("staminaSection").hidden = true;

	// Experience
	if (entState.promotion)
	{
		var experienceBar = Engine.GetGUIObjectByName("experienceBar");
		var experienceSize = experienceBar.size;
		experienceSize.rtop = 100 - (100 * Math.max(0, Math.min(1, 1.0 * +entState.promotion.curr / +entState.promotion.req)));
		experienceBar.size = experienceSize;
 
		if (entState.promotion.curr < entState.promotion.req)
			Engine.GetGUIObjectByName("experience").tooltip = sprintf(translate("%(experience)s %(current)s / %(required)s"), {
				experience: "[font=\"sans-bold-13\"]" + translate("Experience:") + "[/font]",
				current: Math.floor(entState.promotion.curr),
				required: entState.promotion.req
			});
		else
			Engine.GetGUIObjectByName("experience").tooltip = sprintf(translate("%(experience)s %(current)s"), {
				experience: "[font=\"sans-bold-13\"]" + translate("Experience:") + "[/font]",
				current: Math.floor(entState.promotion.curr)
			});
		Engine.GetGUIObjectByName("experience").hidden = false;
	}
	else
	{
		Engine.GetGUIObjectByName("experience").hidden = true;
	}

	// Resource stats
	if (entState.resourceSupply)
	{
		var resources = entState.resourceSupply.isInfinite ? translate("∞") :  // Infinity symbol
						sprintf(translate("%(amount)s / %(max)s"), { amount: Math.ceil(+entState.resourceSupply.amount), max: entState.resourceSupply.max });
		var resourceType = getResourceTypeDisplayName(entState.resourceSupply.type);

		var unitResourceBar = Engine.GetGUIObjectByName("resourceBar");
		var resourceSize = unitResourceBar.size;

		resourceSize.rright = entState.resourceSupply.isInfinite ? 100 :
						100 * Math.max(0, Math.min(1, +entState.resourceSupply.amount / +entState.resourceSupply.max));
		unitResourceBar.size = resourceSize;
		Engine.GetGUIObjectByName("resourceLabel").caption = sprintf(translate("%(resource)s:"), { resource: resourceType });
		Engine.GetGUIObjectByName("resourceStats").caption = resources;

		if (entState.hitpoints)
			Engine.GetGUIObjectByName("resourceSection").size = Engine.GetGUIObjectByName("staminaSection").size;
		else
			Engine.GetGUIObjectByName("resourceSection").size = Engine.GetGUIObjectByName("healthSection").size;

		Engine.GetGUIObjectByName("resourceSection").hidden = false;
	}
	else
	{
		Engine.GetGUIObjectByName("resourceSection").hidden = true;
	}

	// Resource carrying
	if (entState.resourceCarrying && entState.resourceCarrying.length)
	{
		// We should only be carrying one resource type at once, so just display the first
		var carried = entState.resourceCarrying[0];

		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/resources/"+carried.type+".png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = sprintf(translate("%(amount)s / %(max)s"), { amount: carried.amount, max: carried.max });
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = "";
	}
	// Use the same indicators for traders
	else if (entState.trader && entState.trader.goods.amount)
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/resources/"+entState.trader.goods.type+".png";
		var totalGain = entState.trader.goods.amount.traderGain;
		if (entState.trader.goods.amount.market1Gain)
			totalGain += entState.trader.goods.amount.market1Gain;
		if (entState.trader.goods.amount.market2Gain)
			totalGain += entState.trader.goods.amount.market2Gain;
		Engine.GetGUIObjectByName("resourceCarryingText").caption = totalGain;
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = sprintf(translate("Gain: %(amount)s"), { amount: getTradingTooltip(entState.trader.goods.amount) });
	}
	// And for number of workers
	else if (entState.foundation)
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = entState.foundation.numBuilders + "    ";
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Number of builders");
	}
	else if (entState.resourceSupply && (!entState.resourceSupply.killBeforeGather || !entState.hitpoints))
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = sprintf(translate("%(amount)s / %(max)s"), { amount: entState.resourceSupply.gatherers.length, max: entState.resourceSupply.maxGatherers }) + "    ";
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Current/max gatherers");
	}
	else
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = true;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = true;
	}

	// Set Player details
	Engine.GetGUIObjectByName("specific").caption = specificName;
	Engine.GetGUIObjectByName("player").caption = playerName;
	Engine.GetGUIObjectByName("playerColorBackground").sprite = "colour: " + playerColor;
	
	if (genericName)
	{
		Engine.GetGUIObjectByName("generic").caption = sprintf(translate("(%(genericName)s)"), { genericName: genericName });
	}
	else
	{
		Engine.GetGUIObjectByName("generic").caption = "";

	}

	if ("gaia" != playerState.civ)
	{
		Engine.GetGUIObjectByName("playerCivIcon").sprite = "stretched:grayscale:" + civEmblem;
		Engine.GetGUIObjectByName("player").tooltip = civName;
	}
	else
	{
		Engine.GetGUIObjectByName("playerCivIcon").sprite = "";
		Engine.GetGUIObjectByName("player").tooltip = "";
	}

	// Icon image
	if (template.icon)
	{
		Engine.GetGUIObjectByName("icon").sprite = "stretched:session/portraits/" + template.icon;
	}
	else
	{
		// TODO: we should require all entities to have icons, so this case never occurs
		Engine.GetGUIObjectByName("icon").sprite = "bkFillBlack";
	}

	var armorLabel = "[font=\"sans-bold-13\"]" + translate("Armor:") + "[/font]"
	var armorString = sprintf(translate("%(label)s %(details)s"), { label: armorLabel, details: armorTypeDetails(entState.armour) });

	// Attack and Armor
	if ("attack" in entState && entState.attack)
	{
		// Rate
		if (entState.buildingAI)
			var rateLabel = "[font=\"sans-bold-13\"]" + translate("Interval:") + "[/font]";
		else
			var rateLabel = "[font=\"sans-bold-13\"]" + translate("Rate:") + "[/font]";

		var rate = sprintf(translate("%(label)s %(details)s"), {
			label: rateLabel,
			details: attackRateDetails(entState)
		});

		var attack;
		var label = "[font=\"sans-bold-13\"]" + getAttackTypeLabel(entState.attack.type) + "[/font]"
		if (entState.attack.type == "Ranged")
		{
			var realRange = entState.attack.elevationAdaptedRange;
			var range =  entState.attack.maxRange;
			var rangeLabel = "[font=\"sans-bold-13\"]" + translate("Range:") + "[/font]"
			var relativeRange = Math.round((realRange - range));
			var meters = "[font=\"sans-10\"][color=\"orange\"]" + translate("meters") + "[/color][/font]";

			if (relativeRange > 0)
				attack = sprintf(translate("%(label)s %(details)s, %(rangeLabel)s %(range)s %(meters)s (%(relative)s), %(rate)s"), {
					label: label,
					details: damageTypeDetails(entState.attack),
					rangeLabel: rangeLabel,
					range: Math.round(range),
					meters: meters,
					relative: "+" + relativeRange,
					rate: rate
				});
			else if (relativeRange < 0)
				attack = sprintf(translate("%(label)s %(details)s, %(rangeLabel)s %(range)s %(meters)s (%(relative)s), %(rate)s"), {
					label: label,
					details: damageTypeDetails(entState.attack),
					rangeLabel: rangeLabel,
					range: Math.round(range),
					meters: meters,
					relative: relativeRange,
					rate: rate
				});
			else // don't show when it's 0
				attack = sprintf(translate("%(label)s %(details)s, %(rangeLabel)s %(range)s %(meters)s, %(rate)s"), {
					label: label,
					details: damageTypeDetails(entState.attack),
					rangeLabel: rangeLabel,
					range: Math.round(range),
					meters: meters,
					rate: rate
				});
		}
		else
		{
			attack = sprintf(translate("%(label)s %(details)s, %(rate)s"), {
				label: label,
				details: damageTypeDetails(entState.attack),
				rate: rate
			});
		}

		Engine.GetGUIObjectByName("attackAndArmorStats").tooltip = attack + "\n" + armorString;	
	}
	else
	{
		Engine.GetGUIObjectByName("attackAndArmorStats").tooltip = armorString;
	}

	// Icon Tooltip
	var iconTooltip = "";

	if (genericName)
		iconTooltip = "[font=\"sans-bold-16\"]" + genericName + "[/font]";

	if (template.visibleIdentityClasses && template.visibleIdentityClasses.length)
	{
		iconTooltip += "\n[font=\"sans-bold-13\"]" + translate("Classes:") + "[/font] ";
		iconTooltip += "[font=\"sans-13\"]" + translate(template.visibleIdentityClasses[0]) ;
		for (var i = 1; i < template.visibleIdentityClasses.length; i++)
			iconTooltip += ", " + translate(template.visibleIdentityClasses[i]);
		iconTooltip += "[/font]";
	}

	if (template.tooltip)
		iconTooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

	Engine.GetGUIObjectByName("iconBorder").tooltip = iconTooltip;

	// Unhide Details Area
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
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
		var unitHealthBar = Engine.GetGUIObjectByName("healthBarMultiple");
		var healthSize = unitHealthBar.size;	
		healthSize.rtop = 100-100*Math.max(0, Math.min(1, averageHealth / maxHealth));
		unitHealthBar.size = healthSize;

		var hitpointsLabel = "[font=\"sans-bold-13\"]" + translate("Hitpoints:") + "[/font]"
		var hitpoints = sprintf(translate("%(label)s %(current)s / %(max)s"), { label: hitpointsLabel, current: averageHealth, max: maxHealth });
		var healthMultiple = Engine.GetGUIObjectByName("healthMultiple");
		healthMultiple.tooltip = hitpoints;
		healthMultiple.hidden = false;
	}
	else
	{
		Engine.GetGUIObjectByName("healthMultiple").hidden = true;
	}
	
	// TODO: Stamina
	// Engine.GetGUIObjectByName("staminaBarMultiple");

	Engine.GetGUIObjectByName("numberOfUnits").caption = selection.length;

	// Unhide Details Area
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = true;
}

// Updates middle entity Selection Details Panel
function updateSelectionDetails()
{
	var supplementalDetailsPanel = Engine.GetGUIObjectByName("supplementalSelectionDetails");
	var detailsPanel = Engine.GetGUIObjectByName("selectionDetails");
	var commandsPanel = Engine.GetGUIObjectByName("unitCommands");

	g_Selection.update();
	var selection = g_Selection.toList();

	if (selection.length == 0)
	{
		Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
		Engine.GetGUIObjectByName("detailsAreaSingle").hidden = true;
		hideUnitCommands();

		supplementalDetailsPanel.hidden = true;
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	/* If the unit has no data (e.g. it was killed), don't try displaying any
	 data for it. (TODO: it should probably be removed from the selection too;
	 also need to handle multi-unit selections) */
	var entState = GetExtendedEntityState(selection[0]);
	if (!entState)
		return;

	var template = GetTemplateData(entState.template);

	// Fill out general info and display it
	if (selection.length == 1)
		displaySingle(entState, template);
	else
		displayMultiple(selection, template);

	// Show basic details.
	detailsPanel.hidden = false;

	if (g_IsObserver)
	{
		// Observers don't need these displayed.
		supplementalDetailsPanel.hidden = true;
		commandsPanel.hidden = true;
	}
	else
	{
		// Fill out commands panel for specific unit selected (or first unit of primary group)
		updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection);
		// Show panels
		supplementalDetailsPanel.hidden = false;
		commandsPanel.hidden = false;
	}
}
