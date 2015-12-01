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
function displaySingle(entState)
{
	// Get general unit and player data
	var template = GetTemplateData(entState.template);
	var specificName = template.name.specific;
	var genericName = template.name.generic;
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
		playerName = sprintf(translate("\\[OFFLINE] %(player)s"), { player: playerName });

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
	Engine.GetGUIObjectByName("healthSection").hidden = !entState.hitpoints;
	if (entState.hitpoints)
	{
		var unitHealthBar = Engine.GetGUIObjectByName("healthBar");
		var healthSize = unitHealthBar.size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		unitHealthBar.size = healthSize;

		if (entState.foundation && entState.visibility == "visible" && entState.foundation.numBuilders !== 0)
		{
			// logic comes from Foundation component.
			var speed = Math.pow(entState.foundation.numBuilders, 0.7);
			var timeLeft = (1.0 - entState.foundation.progress / 100.0) * template.cost.time;
			var timeToCompletion = Math.ceil(timeLeft/speed);
			Engine.GetGUIObjectByName("health").tooltip = sprintf(translatePlural("This foundation will be completed in %(seconds)s second.", "This foundation will be completed in %(seconds)s seconds.", timeToCompletion), { "seconds": timeToCompletion });
		}
		else
			Engine.GetGUIObjectByName("health").tooltip = "";

		Engine.GetGUIObjectByName("healthStats").caption = sprintf(translate("%(hitpoints)s / %(maxHitpoints)s"), {
			hitpoints: Math.ceil(entState.hitpoints),
			maxHitpoints: entState.maxHitpoints
		});
	}

	// CapturePoints
	Engine.GetGUIObjectByName("captureSection").hidden = !entState.capturePoints;
	if (entState.capturePoints)
	{
		let setCaptureBarPart = function(playerID, startSize) {
			var unitCaptureBar = Engine.GetGUIObjectByName("captureBar["+playerID+"]");
			var sizeObj = unitCaptureBar.size;
			sizeObj.rleft = startSize;

			var size = 100*Math.max(0, Math.min(1, entState.capturePoints[playerID] / entState.maxCapturePoints));
			sizeObj.rright = startSize + size;
			unitCaptureBar.size = sizeObj;
			unitCaptureBar.sprite = "color: " + rgbToGuiColor(g_Players[playerID].color, 128);
			unitCaptureBar.hidden=false;
			return startSize + size;
		};

		// first handle the owner's points, to keep those points on the left for clarity
		let size = setCaptureBarPart(entState.player, 0);

		for (let i in entState.capturePoints)
			if (i != entState.player)
				size = setCaptureBarPart(i, size);

		Engine.GetGUIObjectByName("captureStats").caption = sprintf(translate("%(capturePoints)s / %(maxCapturePoints)s"), {
			capturePoints: Math.ceil(entState.capturePoints[entState.player]),
			maxCapturePoints: entState.maxCapturePoints
		});
	}

	// TODO: Stamina

	// Experience
	Engine.GetGUIObjectByName("experience").hidden = !entState.promotion;
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
	}

	// Resource stats
	Engine.GetGUIObjectByName("resourceSection").hidden = !entState.resourceSupply;
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
			Engine.GetGUIObjectByName("resourceSection").size = Engine.GetGUIObjectByName("captureSection").size;
		else
			Engine.GetGUIObjectByName("resourceSection").size = Engine.GetGUIObjectByName("healthSection").size;
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
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = sprintf(translate("Gain: %(gain)s"), { "gain": getTradingTooltip(entState.trader.goods.amount) });
	}
	// And for number of workers
	else if (entState.foundation && entState.visibility == "visible")
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = entState.foundation.numBuilders + "    ";
		if (entState.foundation.numBuilders !== 0)
		{
			var speedup = Math.pow((entState.foundation.numBuilders+1)/entState.foundation.numBuilders, 0.7);
			var timeLeft = (1.0 - entState.foundation.progress / 100.0) * template.cost.time;
			var timeSpeedup = Math.ceil(timeLeft - timeLeft/speedup);
			Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = sprintf(translatePlural("Number of builders.\nTasking another to this foundation would speed construction up by %(speedup)s second.", "Number of builders.\nTasking another to this foundation would speed construction up by %(speedup)s seconds.", timeSpeedup), { "speedup": timeSpeedup });
		}
		else
			Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Number of builders.");
	}
	else if (entState.repairable && entState.repairable.numBuilders > 0 && entState.visibility == "visible")
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = entState.repairable.numBuilders + "    ";
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Number of builders.");
	}
	else if (entState.resourceSupply && (!entState.resourceSupply.killBeforeGather || !entState.hitpoints) && entState.visibility == "visible")
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = sprintf(translate("%(amount)s / %(max)s"), { amount: entState.resourceSupply.numGatherers, max: entState.resourceSupply.maxGatherers }) + "    ";
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
	Engine.GetGUIObjectByName("playerColorBackground").sprite = "color: " + playerColor;
	
	if (genericName !== specificName)
		Engine.GetGUIObjectByName("generic").caption = sprintf(translate("(%(genericName)s)"), { genericName: genericName });
	else
		Engine.GetGUIObjectByName("generic").caption = "";

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
		Engine.GetGUIObjectByName("icon").sprite = "stretched:session/portraits/" + template.icon;
	else
		// TODO: we should require all entities to have icons, so this case never occurs
		Engine.GetGUIObjectByName("icon").sprite = "bkFillBlack";

	var armorString = getArmorTooltip(entState.armour);

	// Attack and Armor
	if ("attack" in entState && entState.attack)
		Engine.GetGUIObjectByName("attackAndArmorStats").tooltip = getAttackTooltip(entState) + "\n" + armorString;
	else
		Engine.GetGUIObjectByName("attackAndArmorStats").tooltip = armorString;

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

	if (template.auras)
		iconTooltip += getAurasTooltip(template);

	if (template.tooltip)
		iconTooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

	Engine.GetGUIObjectByName("iconBorder").tooltip = iconTooltip;

	// Unhide Details Area
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
}

// Fills out information for multiple entities
function displayMultiple(selection)
{
	var averageHealth = 0;
	var maxHealth = 0;
	var maxCapturePoints = 0;
	var capturePoints = (new Array(9)).fill(0);
	var playerID = 0;

	for (let i = 0; i < selection.length; i++)
	{
		let entState = GetEntityState(selection[i]);
		if (!entState)
			continue;
		playerID = entState.player; // trust that all selected entities have the same owner
		if (entState.hitpoints)
		{
			averageHealth += entState.hitpoints;
			maxHealth += entState.maxHitpoints;
		}
		if (entState.capturePoints)
		{
			maxCapturePoints += entState.maxCapturePoints;
			capturePoints = entState.capturePoints.map(function(v, i) { return v + capturePoints[i]; });
		}
	}

	Engine.GetGUIObjectByName("healthMultiple").hidden = averageHealth <= 0;
	if (averageHealth > 0)
	{
		var unitHealthBar = Engine.GetGUIObjectByName("healthBarMultiple");
		var healthSize = unitHealthBar.size;
		healthSize.rtop = 100-100*Math.max(0, Math.min(1, averageHealth / maxHealth));
		unitHealthBar.size = healthSize;

		var hitpointsLabel = "[font=\"sans-bold-13\"]" + translate("Hitpoints:") + "[/font]";
		var hitpoints = sprintf(translate("%(label)s %(current)s / %(max)s"), { label: hitpointsLabel, current: averageHealth, max: maxHealth });
		Engine.GetGUIObjectByName("healthMultiple").tooltip = hitpoints;
	}

	Engine.GetGUIObjectByName("captureMultiple").hidden = maxCapturePoints <= 0;
	if (maxCapturePoints > 0)
	{
		let setCaptureBarPart = function(playerID, startSize)
		{
			var unitCaptureBar = Engine.GetGUIObjectByName("captureBarMultiple["+playerID+"]");
			var sizeObj = unitCaptureBar.size;
			sizeObj.rtop = startSize;

			var size = 100*Math.max(0, Math.min(1, capturePoints[playerID] / maxCapturePoints));
			sizeObj.rbottom = startSize + size;
			unitCaptureBar.size = sizeObj;
			unitCaptureBar.sprite = "color: " + rgbToGuiColor(g_Players[playerID].color, 128);
			unitCaptureBar.hidden=false;
			return startSize + size;
		};

		let size = 0;
		for (let i in capturePoints)
			if (i != playerID)
				size = setCaptureBarPart(i, size);

		// last handle the owner's points, to keep those points on the bottom for clarity
		setCaptureBarPart(playerID, size);

		var capturePointsLabel = "[font=\"sans-bold-13\"]" + translate("Capture points:") + "[/font]";
		var capturePointsTooltip = sprintf(translate("%(label)s %(current)s / %(max)s"), { label: capturePointsLabel, current: Math.ceil(capturePoints[playerID]), max: Math.ceil(maxCapturePoints) });
		Engine.GetGUIObjectByName("captureMultiple").tooltip = capturePointsTooltip;
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

	// Fill out general info and display it
	if (selection.length == 1)
		displaySingle(entState);
	else
		displayMultiple(selection);

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
	}
}
