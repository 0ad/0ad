var g_TooltipTextFormats = {
	"unit": ['[font="sans-10"][color="orange"]', '[/color][/font]'],
	"header": ['[font="sans-bold-13"]', '[/font]'],
	"body": ['[font="sans-13"]', '[/font]'],
	"comma": ['[font="sans-12"]', '[/font]']
};

var g_AttackTypes = {
	"Melee": translate("Melee Attack:"),
	"Ranged": translate("Ranged Attack:"),
	"Capture": translate("Capture Attack:")
};

var g_DamageTypes = {
	"hack": translate("Hack"),
	"pierce": translate("Pierce"),
	"crush": translate("Crush"),
};

var g_SplashDamageTypes = {
	"Circular": translate("Circular Splash Damage"),
	"Linear": translate("Linear Splash Damage")
};

var g_RangeTooltipString = {
	"relative": {
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 2 to 10 (+2) meters, Interval: 3 arrows / 2 seconds
		"minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(minRange)s to %(maxRange)s (%(relativeRange)s) %(rangeUnit)s, %(rate)s"),
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 10 (+2) meters, Interval: 3 arrows / 2 seconds
		"no-minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(maxRange)s (%(relativeRange)s) %(rangeUnit)s, %(rate)s"),
	},
	"non-relative": {
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 2 to 10 meters, Interval: 3 arrows / 2 seconds
		"minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(minRange)s to %(maxRange)s %(rangeUnit)s, %(rate)s"),
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 10 meters, Interval: 3 arrows / 2 seconds
		"no-minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(maxRange)s %(rangeUnit)s, %(rate)s"),
	}
};

function resourceIcon(resource)
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

function getSecondsString(seconds)
{
	return sprintf(translatePlural("%(time)s %(second)s", "%(time)s %(second)s", seconds), {
		"time": seconds,
		"second": unitFont(translatePlural("second", "seconds", seconds))
	});
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

function getCurrentHealthTooltip(entState, label)
{
	if (!entState.maxHitpoints)
		return "";

	return sprintf(translate("%(healthLabel)s %(current)s / %(max)s"), {
		"healthLabel": headerFont(label || translate("Health:")),
		"current": Math.round(entState.hitpoints),
		"max": Math.round(entState.maxHitpoints)
	});
}

function attackRateDetails(template, type)
{
	// Either one arrow shot by UnitAI,
	let timeString = getSecondsString(template.attack[type].repeatTime / 1000);

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

function getAttackTooltip(template)
{
	if (!template.attack)
		return "";

	let tooltips = [];
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
			tooltips.push(sprintf(translate("%(attackLabel)s %(details)s, %(rate)s"), {
				"attackLabel": attackLabel,
				"details":
					type == "Capture" ?
						template.attack.Capture.value :
						damageTypesToText(template.attack[type]),
				"rate": rate
			}));
			continue;
		}

		let minRange = Math.round(template.attack[type].minRange);
		let maxRange = Math.round(template.attack[type].maxRange);
		let realRange = template.attack[type].elevationAdaptedRange;
		let relativeRange = realRange ? Math.round(realRange - maxRange) : 0;

		tooltips.push(sprintf(g_RangeTooltipString[relativeRange ? "relative" : "non-relative"][minRange ? "minRange" : "no-minRange"], {
			"attackLabel": attackLabel,
			"damageTypes": damageTypesToText(template.attack[type]),
			"rangeLabel": headerFont(translate("Range:")),
			"minRange": minRange,
			"maxRange": maxRange,
			"relativeRange": relativeRange > 0 ? sprintf(translate("+%(number)s"), { "number": relativeRange }) : relativeRange,
			"rangeUnit":
				unitFont(minRange || relativeRange ?
					// Translation: For example "0.5 to 1 meters", "1 (+1) meters" or "1 to 2 (+3) meters"
					translate("meters") :
					translatePlural("meter", "meters", maxRange)),
			"rate": rate,
		}));
	}
	return tooltips.join("\n");
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
			sprintf(translate("%(label)s: %(value)s"), {
				"label": headerFont(g_SplashDamageTypes[splash.shape]),
				"value": damageTypesToText(splash)
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
			"garrisonLimit": template.garrisonHolder.capacity
		})
	];

	if (template.garrisonHolder.buffHeal)
		tooltips.push(
			sprintf(translate("%(healRateLabel)s %(value)s %(health)s / %(second)s"), {
				"healRateLabel": headerFont(translate("Heal:")),
				"value": Math.round(template.garrisonHolder.buffHeal),
				"health": unitFont(translate("Health")),
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
			template.garrisonHolder.capacity
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
		"health": unitFont(translate("Health")),
		"second": unitFont(translate("second")),
		"worker": unitFont(translate("Worker"))
	});
}

function getBuildRateTooltip(template)
{
	if (!template.buildRate)
		return "";

	return sprintf(translate("%(buildRateLabel)s %(value)s %(health)s / %(second)s / %(worker)s"), {
		"buildRateLabel": headerFont(translate("Build Rate:")),
		"value": template.buildRate.toFixed(1),
		"health": unitFont(translate("Health")),
		"second": unitFont(translate("second")),
		"worker": unitFont(translate("Worker"))
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
				"component": resourceIcon(type),
				"cost": totalCosts[type]
			}));

	return costs;
}

function getGatherTooltip(template)
{
	if (!template.resourceGatherRates)
		return "";

	// Average the resource rates (TODO: distinguish between subtypes)
	let rates = {};
	for (let resource of g_ResourceData.GetResources())
	{
		let types = [resource.code];
		for (let subtype in resource.subtypes)
			// We ignore ruins as those are not that common and skew the results
			if (subtype !== "ruins")
				types.push(resource.code + "." + subtype);

		let [rate, count] = types.reduce((sum, t) => {
				let r = template.resourceGatherRates[t];
				return [sum[0] + (r > 0 ? r : 0), sum[1] + (r > 0 ? 1 : 0)];
			}, [0, 0]);

		if (rate > 0)
			rates[resource.code] = +(rate / count).toFixed(1);
	}

	if (!Object.keys(rates).length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Gather Rates:")),
		"details":
			Object.keys(rates).map(
				type => sprintf(translate("%(resourceIcon)s %(rate)s"), {
					"resourceIcon": resourceIcon(type),
					"rate": rates[type]
				})
			).join("  ")
	});
}

function getResourceTrickleTooltip(template)
{
	if (!template.resourceTrickle)
		return "";

	let resCodes = Object.keys(template.resourceTrickle.rates).filter(res => template.resourceTrickle.rates[res]);
	if (!resCodes.length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Resource Trickle:")),
		"details": sprintf(translate("%(resources)s / %(time)s"), {
			"resources":
				resCodes.map(
					res => sprintf(translate("%(resourceIcon)s %(rate)s"), {
						"resourceIcon": resourceIcon(res),
						"rate": template.resourceTrickle.rates[res]
					})
				).join("  "),
			"time": getSecondsString(template.resourceTrickle.interval / 1000)
		})
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
				"resourceIcon": resourceIcon(resource),
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

function getRequiredTechnologyTooltip(technologyEnabled, requiredTechnology, civ)
{
	if (technologyEnabled)
		return "";

	return sprintf(translate("Requires %(technology)s"), {
		"technology": getEntityNames(GetTechnologyData(requiredTechnology, civ))
	});
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
			"component": '[font="sans-12"]' + resourceIcon(resource) + '[/font]',
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

	// Translation: Example: "Epibátēs Athēnaîos [font="sans-bold-16"](Athenian Marine)[/font]"
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

	const lootTypes = g_ResourceData.GetCodes().concat(["xp"]);
	let lootLabels = [];
	for (let type of lootTypes)
	{
		let loot =
			(template.loot && template.loot[type] || 0) +
			(resourcesCarried[type] || 0);

		if (!loot)
			continue;

		// Translation: %(component) will be the icon for the loot type and %(loot) will be the value.
		lootLabels.push(sprintf(translate("%(component)s %(loot)s"), {
			"component": resourceIcon(type),
			"loot": loot
		}));
	}

	if (!lootLabels.length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Loot:")),
		"details": lootLabels.join("  ")
	});
}
