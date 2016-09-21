const g_TooltipTextFormats = {
	"unit": ['[font="sans-10"][color="orange"]', '[/color][/font]'],
	"header": ['[font="sans-bold-13"]', '[/font]'],
	"body": ['[font="sans-13"]', '[/font]'],
	"comma": ['[font="sans-12"]', '[/font]']
};

const g_AttackTypes = {
	"Melee": translate("Melee Attack:"),
	"Ranged": translate("Ranged Attack:"),
	"Capture": translate("Capture Attack:")
};

const g_DamageTypes = {
	"hack": translate("Hack"),
	"pierce": translate("Pierce"),
	"crush": translate("Crush"),
};

function costIcon(resource)
{
	return '[icon="icon_' + resource + '"]';
}

function bodyFont(text)
{
	return g_TooltipTextFormats.body[0] + text + g_TooltipTextFormats.body[1];
}

function headerFont(text)
{
	return g_TooltipTextFormats.header[0] + text + g_TooltipTextFormats.header[1];
}

function unitFont(text)
{
	return g_TooltipTextFormats.unit[0] + text + g_TooltipTextFormats.unit[1];
}

function commaFont(text)
{
	return g_TooltipTextFormats.comma[0] + text + g_TooltipTextFormats.comma[1];
}

function getEntityTooltip(template)
{
	if (!template.tooltip)
		return "";

	return bodyFont(template.tooltip);
}

function getHealthTooltip(template)
{
	if (!template.health)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Health:")),
		"details": template.health
	});
}

function attackRateDetails(template, type)
{
	// Either one arrow shot by UnitAI,
	let time = template.attack[type].repeatTime / 1000;
	let timeString = sprintf(translatePlural("%(time)s %(second)s", "%(time)s %(second)s", time), {
		"time": time,
		"second": unitFont(translatePlural("second", "seconds", time))
	});

	// or multiple arrows shot by BuildingAI
	if (!template.buildingAI || type != "Ranged")
		return timeString;

	// Show either current rate from simulation or default count if the sim is not running
	let arrows = template.buildingAI.arrowCount || template.buildingAI.defaultArrowCount;
	let arrowString = sprintf(translatePlural("%(arrowcount)s %(arrows)s", "%(arrowcount)s %(arrows)s", arrows), {
		"arrowcount": arrows,
		"arrows": unitFont(translatePlural("arrow", "arrows", arrows))
	});

	return sprintf(translate("%(arrowString)s / %(timeString)s"), {
		"arrowString": arrowString,
		"timeString": timeString
	});
}

/**
 * Converts an armor level into the actual reduction percentage
 */
function armorLevelToPercentageString(level)
{
	return sprintf(translate("%(percentage)s%%"), {
		"percentage": (100 - Math.round(Math.pow(0.9, level) * 100))
	});
}

function getArmorTooltip(template)
{
	if (!template.armour)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Armor:")),
		"details":
			Object.keys(template.armour).map(
				dmgType => sprintf(translate("%(damage)s %(damageType)s %(armorPercentage)s"), {
					"damage": template.armour[dmgType].toFixed(1),
					"damageType": unitFont(g_DamageTypes[dmgType]),
					"armorPercentage":
						'[font="sans-10"]' +
						sprintf(translate("(%(armorPercentage)s)"), {
							"armorPercentage": armorLevelToPercentageString(template.armour[dmgType])
						}) + '[/font]'
				})
			).join(commaFont(translate(", ")))
	});
}

function damageTypesToText(dmg)
{
	if (!dmg)
		return '[font="sans-12"]' + translate("(None)") + '[/font]';

	return Object.keys(g_DamageTypes).filter(
		dmgType => dmg[dmgType]).map(
		dmgType => sprintf(translate("%(damage)s %(damageType)s"), {
			"damage": dmg[dmgType].toFixed(1),
			"damageType": unitFont(g_DamageTypes[dmgType])
		})).join(commaFont(translate(", ")));
}

// TODO: should also show minRange
function getAttackTooltip(template)
{
	if (!template.attack)
		return "";

	let attacks = [];
	for (let type in template.attack)
	{
		if (type == "Slaughter")
			continue; // Slaughter is used to kill animals, so do not show it.

		let rate = sprintf(translate("%(label)s %(details)s"), {
			"label":
				headerFont(
					template.buildingAI && type == "Ranged" ?
						translate("Interval:") :
						translate("Rate:")),
			"details": attackRateDetails(template, type)
		});

		let attackLabel = headerFont(g_AttackTypes[type]);
		if (type == "Capture" || type != "Ranged")
		{
			attacks.push(sprintf(translate("%(attackLabel)s %(details)s, %(rate)s"), {
				"attackLabel": attackLabel,
				"details":
					type == "Capture" ?
						template.attack.Capture.value :
						damageTypesToText(template.attack[type]),
				"rate": rate
			}));
			continue;
		}

		let realRange = template.attack[type].elevationAdaptedRange;
		let range = Math.round(template.attack[type].maxRange);
		let relativeRange = realRange ? Math.round(realRange - range) : 0;

		let rangeString = relativeRange ?
			translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(rangeString)s (%(relative)s), %(rate)s") :
			translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(rangeString)s, %(rate)s");

		attacks.push(sprintf(rangeString, {
			"attackLabel": attackLabel,
			"damageTypes": damageTypesToText(template.attack[type]),
			"rangeLabel": translate("Range:"),
			"rangeString": sprintf(
				translatePlural("%(range)s %(meters)s", "%(range)s %(meters)s", range), {
					"range": range,
					"meters": unitFont(translatePlural("meter", "meters", range))
				}),
			"rate": rate,
			"relative": relativeRange > 0 ? "+" + relativeRange : relativeRange,
		}));
	}
	return attacks.join("\n");
}

function getSplashDamageTooltip(template)
{
	if (!template.attack)
		return "";

	let tooltips = [];
	for (let type in template.attack)
	{
		let splash = template.attack[type].splash;
		if (!splash)
			continue;

		tooltips.push([
			sprintf(translate("%(attackLabel)s %(damageTypes)s"), {
				"attackLabel": headerFont(translate("Splash Damage:")),
				"damageTypes": damageTypesToText(splash)
			}),
			sprintf(translate("Friendly Fire: %(enabled)s"), {
				"enabled": splash.friendlyFire ? translate("Yes") : translate("No")
			})
		].join(commaFont(translate(", "))));
	}

	// If multiple attack types deal splash damage, the attack type should be shown to differentiate.
	return tooltips.join("\n");
}

function getGarrisonTooltip(template)
{
	if (!template.garrisonHolder)
		return "";

	let tooltips = [
		sprintf(translate("%(label)s: %(garrisonLimit)s"), {
			"label": headerFont(translate("Garrison Limit")),
			"garrisonLimit": template.garrisonHolder.capacity || template.garrisonHolder.max
		})
	];

	if (template.garrisonHolder.buffHeal)
		tooltips.push(
			sprintf(translate("%(healRateLabel)s %(value)s %(health)s / %(second)s"), {
				"healRateLabel": headerFont(translate("Heal:")),
				"value": Math.round(template.garrisonHolder.buffHeal),
				"health": unitFont(translate("health")),
				"second": unitFont(translate("second")),
			})
		);

	return tooltips.join(commaFont(translate(", ")));
}

function getProjectilesTooltip(template)
{
	if (!template.garrisonHolder || !template.buildingAI)
		return "";

	let limit = Math.min(
		template.buildingAI.maxArrowCount || Infinity,
		template.buildingAI.defaultArrowCount +
			template.buildingAI.garrisonArrowMultiplier *
			(template.garrisonHolder.capacity || template.garrisonHolder.max)
	);

	if (!limit)
		return "";

	return [
		sprintf(translate("%(label)s: %(value)s"), {
			"label": headerFont(translate("Projectile Limit")),
			"value": limit
		}),

		sprintf(translate("%(label)s: %(value)s"), {
			"label": headerFont(translateWithContext("projectiles", "Default")),
			"value": template.buildingAI.defaultArrowCount
		}),

		sprintf(translate("%(label)s: %(value)s"), {
			"label": headerFont(translateWithContext("projectiles", "Per Unit")),
			"value": template.buildingAI.garrisonArrowMultiplier
		})
	].join(commaFont(translate(", ")));
}

function getRepairRateTooltip(template)
{
	if (!template.repairRate)
		return "";

	return sprintf(translate("%(repairRateLabel)s %(value)s %(health)s / %(second)s / %(worker)s"), {
		"repairRateLabel": headerFont(translate("Repair Rate:")),
		"value": template.repairRate.toFixed(1),
		"health": unitFont(translate("health")),
		"second": unitFont(translate("second")),
		"worker": unitFont(translate("worker"))
	});
}

function getBuildRateTooltip(template)
{
	if (!template.buildRate)
		return "";

	return sprintf(translate("%(buildRateLabel)s %(value)s %(health)s / %(second)s / %(worker)s"), {
		"buildRateLabel": headerFont(translate("Build Rate:")),
		"value": template.buildRate.toFixed(1),
		"health": unitFont(translate("health")),
		"second": unitFont(translate("second")),
		"worker": unitFont(translate("worker"))
	});
}

/**
 * Multiplies the costs for a template by a given batch size.
 */
function multiplyEntityCosts(template, trainNum)
{
	let totalCosts = {};
	for (let r in template.cost)
		totalCosts[r] = Math.floor(template.cost[r] * trainNum);

	return totalCosts;
}

/**
 * Helper function for getEntityCostTooltip.
 */
function getEntityCostComponentsTooltipString(template, trainNum, entity)
{
	if (!trainNum)
		trainNum = 1;

	let totalCosts = multiplyEntityCosts(template, trainNum);
	if (template.cost.time)
		totalCosts.time = Math.ceil(template.cost.time * (entity ? Engine.GuiInterfaceCall("GetBatchTime", { "entity": entity, "batchSize": trainNum }) : 1));

	let costs = [];
	for (let type in template.cost)
		// Population bonus is shown in the tooltip
		if (type != "populationBonus" && totalCosts[type])
			costs.push(sprintf(translate("%(component)s %(cost)s"), {
				"component": costIcon(type),
				"cost": totalCosts[type]
			}));

	return costs;
}
function getGatherTooltip(template)
{
	if (!template.gather)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Gather Rates:")),
		"details":
			Object.keys(template.gather).map(
				type => sprintf(translate("%(resourceIcon)s %(rate)s"), {
					"resourceIcon": costIcon(type),
					"rate": template.gather[type]
				})
			).join("  ")
	});
}

/**
 * Returns an array of strings for a set of wall pieces. If the pieces share
 * resource type requirements, output will be of the form '10 to 30 Stone',
 * otherwise output will be, e.g. '10 Stone, 20 Stone, 30 Stone'.
 */
function getWallPieceTooltip(wallTypes)
{
	let out = [];
	let resourceCount = {};

	// Initialize the acceptable types for '$x to $y $resource' mode.
	for (let resource in wallTypes[0].cost)
		if (wallTypes[0].cost[resource])
			resourceCount[resource] = [wallTypes[0].cost[resource]];

	let sameTypes = true;
	for (let i = 1; i < wallTypes.length; ++i)
	{
		for (let resource in wallTypes[i].cost)
			// Break out of the same-type mode if this wall requires
			// resource types that the first didn't.
			if (wallTypes[i].cost[resource] && !resourceCount[resource])
			{
				sameTypes = false;
				break;
			}

		for (let resource in resourceCount)
			if (wallTypes[i].cost[resource])
				resourceCount[resource].push(wallTypes[i].cost[resource]);
			else
			{
				sameTypes = false;
				break;
			}
	}

	if (sameTypes)
		for (let resource in resourceCount)
			// Translation: This string is part of the resources cost string on
			// the tooltip for wall structures.
			out.push(sprintf(translate("%(resourceIcon)s %(minimum)s to %(resourceIcon)s %(maximum)s"), {
				"resourceIcon": costIcon(resource),
				"minimum": Math.min.apply(Math, resourceCount[resource]),
				"maximum": Math.max.apply(Math, resourceCount[resource])
			}));
	else
		for (let i = 0; i < wallTypes.length; ++i)
			out.push(getEntityCostComponentsTooltipString(wallTypes[i]).join(", "));

	return out;
}

/**
 * Returns the cost information to display in the specified entity's construction button tooltip.
 */
function getEntityCostTooltip(template, trainNum, entity)
{
	// Entities with a wallset component are proxies for initiating wall placement and as such do not have a cost of
	// their own; the individual wall pieces within it do.
	if (template.wallSet)
	{
		let templateLong = GetTemplateData(template.wallSet.templates.long);
		let templateMedium = GetTemplateData(template.wallSet.templates.medium);
		let templateShort = GetTemplateData(template.wallSet.templates.short);
		let templateTower = GetTemplateData(template.wallSet.templates.tower);

		let wallCosts = getWallPieceTooltip([templateShort, templateMedium, templateLong]);
		let towerCosts = getEntityCostComponentsTooltipString(templateTower);

		return sprintf(translate("Walls:  %(costs)s"), { "costs": wallCosts.join("  ") }) + "\n" +
		       sprintf(translate("Towers:  %(costs)s"), { "costs": towerCosts.join("  ") });
	}

	if (template.cost)
		return getEntityCostComponentsTooltipString(template, trainNum, entity).join("  ");

	return "";
}

/**
 * Returns the population bonus information to display in the specified entity's construction button tooltip.
 */
function getPopulationBonusTooltip(template)
{
	let popBonus = "";
	if (template.cost && template.cost.populationBonus)
		popBonus = sprintf(translate("%(label)s %(populationBonus)s"), {
			"label": headerFont(translate("Population Bonus:")),
			"populationBonus": template.cost.populationBonus
		});
	return popBonus;
}

/**
 * Returns a message with the amount of each resource needed to create an entity.
 */
function getNeededResourcesTooltip(resources)
{
	if (!resources)
		return "";

	let formatted = [];
	for (let resource in resources)
		formatted.push(sprintf(translate("%(component)s %(cost)s"), {
			"component": '[font="sans-12"]' + costIcon(resource) + '[/font]',
			"cost": resources[resource]
		}));

	return '[font="sans-bold-13"][color="red"]' +
		translate("Insufficient resources:") +
		'[/color][/font]' + " " +
		formatted.join("  ");
}

function getSpeedTooltip(template)
{
	if (!template.speed)
		return "";

	let walk = template.speed.walk.toFixed(1);
	let run = template.speed.run.toFixed(1);

	if (walk == 0 && run == 0)
		return "";

	return sprintf(translate("%(label)s %(speeds)s"), {
		"label": headerFont(translate("Speed:")),
		"speeds":
			sprintf(translate("%(speed)s %(movementType)s"), {
				"speed": walk,
				"movementType": unitFont(translate("Walk"))
			}) +
			commaFont(translate(", ")) +
			sprintf(translate("%(speed)s %(movementType)s"), {
				"speed": run,
				"movementType": unitFont(translate("Run"))
			})
	});
}

function getHealerTooltip(template)
{
	if (!template.heal)
		return "";

	let hp = +(template.heal.hp.toFixed(1));
	let range = +(template.heal.range.toFixed(0));
	let rate = +((template.heal.rate / 1000).toFixed(1));

	return [
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", hp), {
			"label": headerFont(translate("Heal:")),
			"val": hp,
			// Translation: Short for hit points (or health points) that are healed in one healing action
			"unit": unitFont(translatePlural("HP", "HP", hp))
		}),
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", range), {
			"label": headerFont(translate("Range:")),
			"val": range,
			"unit": unitFont(translatePlural("meter", "meters", range))
		}),
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", rate), {
			"label": headerFont(translate("Rate:")),
			"val": rate,
			"unit": unitFont(translatePlural("second", "seconds", rate))
		})
	].join(translate(", "));
}

function getAurasTooltip(template)
{
	if (!template.auras)
		return "";

	let tooltips = Object.keys(template.auras).map(
		aura => sprintf(translate("%(auralabel)s %(aurainfo)s"), {
			"auralabel": headerFont(sprintf(translate("%(auraname)s:"), {
				"auraname": translate(template.auras[aura].name)
			})),
			"aurainfo": bodyFont(translate(template.auras[aura].description))
		}));
	return tooltips.join("\n");
}

function getEntityNames(template)
{
	if (!template.name.specific)
		return template.name.generic;

	if (template.name.specific == template.name.generic)
		return template.name.specific;

	return sprintf(translate("%(specificName)s (%(genericName)s)"), {
		"specificName": template.name.specific,
		"genericName": template.name.generic
	});

}
function getEntityNamesFormatted(template)
{
	if (!template.name.specific)
		return '[font="sans-bold-16"]' + template.name.generic + "[/font]";

	return sprintf(translate("%(specificName)s %(fontStart)s(%(genericName)s)%(fontEnd)s"), {
		"specificName":
			'[font="sans-bold-16"]' + template.name.specific[0] + '[/font]' +
			'[font="sans-bold-12"]' + template.name.specific.slice(1).toUpperCase() + '[/font]',
		"genericName": template.name.generic,
		"fontStart": '[font="sans-bold-16"]',
		"fontEnd": '[/font]'
	});
}

function getVisibleEntityClassesFormatted(template)
{
	if (!template.visibleIdentityClasses || !template.visibleIdentityClasses.length)
		return "";

	return headerFont(translate("Classes:")) + ' ' +
		bodyFont(template.visibleIdentityClasses.map(c => translate(c)).join(translate(", ")));
}

function getLootTooltip(template)
{
	if (!template.loot && !template.resourceCarrying)
		return "";

	let resourcesCarried = [];
	if (template.resourceCarrying)
		resourcesCarried = calculateCarriedResources(
			template.resourceCarrying,
			template.trader && template.trader.goods
		);

	const lootTypes = ["xp", "food", "wood", "stone", "metal"];
	let lootLabels = [];
	for (let type of lootTypes)
	{
		let loot =
			(template.loot && template.loot[type] || 0) +
			(resourcesCarried[type] || 0);

		if (!loot)
			continue;

		lootLabels.push(sprintf(translate("%(component)s %(loot)s"), {
			"component": costIcon(type),
			"loot": loot
		}));
	}

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Loot:")),
		"details": lootLabels.join("  ")
	});
}
