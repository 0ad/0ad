/**
 * This file contains shared logic for applying tech modifications in GUI, AI,
 * and simulation scripts. As such it must be fully deterministic and not store
 * any global state, but each context should do its own caching as needed.
 * Also it cannot directly access the simulation and requires data passed to it.
 */

/**
 * Returns modified property value modified by the applicable tech
 * modifications.
 *
 * @param currentTechModifications Object with mapping of property names to
 * modification arrays, retrieved from the intended player's TechnologyManager.
 * @param classes Array contianing the class list of the template.
 * @param propertyName String encoding the name of the value.
 * @param propertyValue Number storing the original value. Can also be
 * non-numberic, but then only "replace" techs can be supported.
 */
function GetTechModifiedProperty(currentTechModifications, classes, propertyName, propertyValue)
{
	let modifications = currentTechModifications[propertyName] || [];

	let multiply = 1;
	let add = 0;

	for (let modification of modifications)
	{
		if (!DoesModificationApply(modification, classes))
			continue;
		if (modification.replace !== undefined)
			return modification.replace;
		if (modification.multiply)
			multiply *= modification.multiply;
		else if (modification.add)
			add += modification.add;
		else
			warn("GetTechModifiedProperty: modification format not recognised (modifying " + propertyName + "): " + uneval(modification));
	}

	// Note, some components pass non-numeric values (for which only the "replace" modification makes sense)
	if (typeof propertyValue == "number")
		return propertyValue * multiply + add;
	return propertyValue;
}

/**
 * Derives modifications (to be applied to entities) from a given technology.
 * 
 * @param {Object} techTemplate - The technology template to derive the modifications from.
 * @return {Object} containing the relevant modifications.
 */
function DeriveModificationsFromTech(techTemplate)
{
	if (!techTemplate.modifications)
		return {};

	let techMods = {};
	let techAffects = [];
	if (techTemplate.affects && techTemplate.affects.length)
		for (let affected of techTemplate.affects)
			techAffects.push(affected.split(/\s+/));
	else
		techAffects.push([]);

	for (let mod of techTemplate.modifications)
	{
		let affects = techAffects.slice();
		if (mod.affects)
		{
			let specAffects = mod.affects.split(/\s+/);
			for (let a in affects)
				affects[a] = affects[a].concat(specAffects);
		}

		let newModifier = { "affects": affects };
		for (let idx in mod)
			if (idx !== "value" && idx !== "affects")
				newModifier[idx] = mod[idx];

		if (!techMods[mod.value])
			techMods[mod.value] = [];
		techMods[mod.value].push(newModifier);
	}
	return techMods;
}

/**
 * Derives modifications (to be applied to entities) from a provided array
 * of technology template data.
 *
 * @param {array} techsDataArray
 * @return {object} containing the combined relevant modifications of all
 *                  the technologies.
 */
function DeriveModificationsFromTechnologies(techsDataArray)
{
	let derivedModifiers = {};
	for (let technology of techsDataArray)
	{
		if (!technology.reqs)
			continue;

		let modifiers = DeriveModificationsFromTech(technology);
		for (let modPath in modifiers)
		{
			if (!derivedModifiers[modPath])
				derivedModifiers[modPath] = [];
			derivedModifiers[modPath] = derivedModifiers[modPath].concat(modifiers[modPath]);
		}
	}
	return derivedModifiers;
}

/**
 * Returns whether the given modification applies to the entity containing the given class list
 */
function DoesModificationApply(modification, classes)
{
	return MatchesClassList(classes, modification.affects);
}

/**
 * Derives the technology requirements from a given technology template.
 * Takes into account the `supersedes` attribute.
 *
 * @param {object} template - The template object. Loading of the template must have already occured.
 *
 * @return Derived technology requirements. See `InterpretTechRequirements` for object's syntax.
 */
function DeriveTechnologyRequirements(template, civ)
{
	let requirements = [];

	if (template.requirements)
	{
		let op = Object.keys(template.requirements)[0];
		let val = template.requirements[op];
		requirements = InterpretTechRequirements(civ, op, val);
	}

	if (template.supersedes && requirements)
	{
		if (!requirements.length)
			requirements.push({});

		for (let req of requirements)
		{
			if (!req.techs)
				req.techs = [];
			req.techs.push(template.supersedes);
		}
	}

	return requirements;
}

/**
 * Interprets the prerequisite requirements of a technology.
 *
 * Takes the initial { key: value } from the short-form requirements object in entity templates,
 * and parses it into an object that can be more easily checked by simulation and gui.
 *
 * Works recursively if needed.
 *
 * The returned object is in the form:
 * ```
 *	{ "techs": ["tech1", "tech2"] },
 *	{ "techs": ["tech3"] }
 * ```
 * or
 * ```
 *	{ "entities": [[{
 *		"class": "human",
 *		"number": 2,
 *		"check": "count"
 *	}
 * or
 * ```
 *	false;
 * ```
 * (Or, to translate:
 * 1. need either both `tech1` and `tech2`, or `tech3`
 * 2. need 2 entities with the `human` class
 * 3. cannot research this tech at all)
 *
 * @param {string} civ - The civ code
 * @param {string} operator - The base operation. Can be "civ", "notciv", "tech", "entity", "all" or "any".
 * @param {mixed} value - The value associated with the above operation.
 *
 * @return Object containing the requirements for the given civ, or false if the civ cannot research the tech.
 */
function InterpretTechRequirements(civ, operator, value)
{
	let requirements = [];

	switch (operator)
	{
	case "civ":
		return !civ || civ == value ? [] : false;

	case "notciv":
		return civ == value ? false : [];

	case "entity":
	{
		let number = value.number || value.numberOfTypes || 0;
		if (number > 0)
			requirements.push({
				"entities": [{
					"class": value.class,
					"number": number,
					"check": value.number ? "count" : "variants"
				}]
			});
		break;
	}

	case "tech":
		requirements.push({
			"techs": [value]
		});
		break;

	case "all":
	{
		let civPermitted = undefined; // tri-state (undefined, false, or true)
		for (let subvalue of value)
		{
			let newOper = Object.keys(subvalue)[0];
			let newValue = subvalue[newOper];
			let result = InterpretTechRequirements(civ, newOper, newValue);

			switch (newOper)
			{
			case "civ":
				if (result)
					civPermitted = true;
				else if (civPermitted !== true)
					civPermitted = false;
				break;

			case "notciv":
				if (!result)
					return false;
				break;

			case "any":
				if (!result)
					return false;
				// else, fall through

			case "all":
				if (!result)
				{
					let nullcivreqs = InterpretTechRequirements(null, newOper, newValue);
					if (!nullcivreqs || !nullcivreqs.length)
						civPermitted = false;
					continue;
				}
				// else, fall through

			case "tech":
			case "entity":
			{
				if (result.length)
				{
					if (!requirements.length)
						requirements.push({});

					let newRequirements = [];
					for (let currReq of requirements)
						for (let res of result)
						{
							let newReq = {};
							for (let subtype in currReq)
								newReq[subtype] = currReq[subtype];

							for (let subtype in res)
							{
								if (!newReq[subtype])
									newReq[subtype] = [];
								newReq[subtype] = newReq[subtype].concat(res[subtype]);
							}
							newRequirements.push(newReq);
						}
					requirements = newRequirements;
				}
				break;
			}

			}
		}
		if (civPermitted === false) // if and only if false
			return false;
		break;
	}

	case "any":
	{
		let civPermitted = false;
		for (let subvalue of value)
		{
			let newOper = Object.keys(subvalue)[0];
			let newValue = subvalue[newOper];
			let result = InterpretTechRequirements(civ, newOper, newValue);

			switch (newOper)
			{

			case "civ":
				if (result)
					return [];
				break;

			case "notciv":
				if (!result)
					return false;
				civPermitted = true;
				break;

			case "any":
				if (!result)
				{
					let nullcivreqs = InterpretTechRequirements(null, newOper, newValue);
					if (!nullcivreqs || !nullcivreqs.length)
						continue;
					return false;
				}
				// else, fall through

			case "all":
				if (!result)
					continue;
				civPermitted = true;
				// else, fall through

			case "tech":
			case "entity":
				for (let res of result)
					requirements.push(res);
				break;

			}
		}
		if (!civPermitted && !requirements.length)
			return false;
		break;
	}

	default:
		warn("Unknown requirement operator: "+operator);
	}

	return requirements;
}

/**
 * Determine order of phases.
 *
 * @param {object} phases - The current available store of phases.
 * @return {array} List of phases
 */
function UnravelPhases(phases)
{
	let phaseMap = {};
	for (let phaseName in phases)
	{
		let phaseData = phases[phaseName];
		if (!phaseData.reqs.length || !phaseData.reqs[0].techs || !phaseData.replaces)
			continue;

		let myPhase = phaseData.replaces[0];
		let reqPhase = phaseData.reqs[0].techs[0];
		if (phases[reqPhase] && phases[reqPhase].replaces)
			reqPhase = phases[reqPhase].replaces[0];

		phaseMap[myPhase] = reqPhase;
		if (!phaseMap[reqPhase])
			phaseMap[reqPhase] = undefined;
	}

	let phaseList = Object.keys(phaseMap);
	phaseList.sort((a, b) => phaseList.indexOf(a) - phaseList.indexOf(phaseMap[b]));

	return phaseList;
}
