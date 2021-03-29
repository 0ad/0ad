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
 * @param modifications array of modificiations
 * @param classes Array containing the class list of the template.
 * @param originalValue Number storing the original value. Can also be
 * non-numeric, but then only "replace" and "tokens" techs can be supported.
 */
function GetTechModifiedProperty(modifications, classes, originalValue)
{
	if (!modifications.length)
		return originalValue;

	// From indicative profiling, splitting in two sub-functions or checking directly
	// is about as efficient, but splitting makes it easier to report errors.
	if (typeof originalValue === "string")
		return GetTechModifiedProperty_string(modifications, classes, originalValue);
	if (typeof originalValue === "number")
		return GetTechModifiedProperty_numeric(modifications, classes, originalValue);
	return GetTechModifiedProperty_generic(modifications, classes, originalValue);
}

function GetTechModifiedProperty_generic(modifications, classes, originalValue)
{
	for (let modification of modifications)
	{
		if (!DoesModificationApply(modification, classes))
			continue;
		if (!modification.replace)
			warn("GetTechModifiedProperty: modification format not recognised : " + uneval(modification));

		return modification.replace;
	}

	return originalValue;
}

function GetTechModifiedProperty_numeric(modifications, classes, originalValue)
{
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
			warn("GetTechModifiedProperty: numeric modification format not recognised : " + uneval(modification));
	}
	return originalValue * multiply + add;
}

function GetTechModifiedProperty_string(modifications, classes, originalValue)
{
	let value = originalValue;
	for (let modification of modifications)
	{
		if (!DoesModificationApply(modification, classes))
			continue;
		if (modification.replace !== undefined)
			return modification.replace;
		// Multiple token replacement works, though ordering is not technically guaranteed.
		// In practice, the order will be that of 'research', which ought to be fine,
		// and operations like adding tokens are order-independent anyways,
		// but modders beware if replacement or deletions are implemented.
		if (modification.tokens !== undefined)
			value = HandleTokens(value, modification.tokens);
		else
			warn("GetTechModifiedProperty: string modification format not recognised : " + uneval(modification));
	}
	return value;
}


/**
 * Returns whether the given modification applies to the entity containing the given class list
 * NB: returns true if modifications.affects is empty, to allow "affects anything" modifiers.
 */
function DoesModificationApply(modification, classes)
{
	if (!modification.affects || !modification.affects.length)
		return true;
	return MatchesClassList(classes, modification.affects);
}

/**
 * Returns a modified list of tokens.
 * Supports "A>B" to replace A by B, "-A" to remove A, and the rest will add tokens.
 */
function HandleTokens(originalValue, modification)
{
	let tokens = originalValue === "" ? [] : originalValue.split(/\s+/);
	let newTokens = modification === "" ? [] : modification.split(/\s+/);
	for (let token of newTokens)
	{
		if (token.indexOf(">") !== -1)
		{
			let [oldToken, newToken] = token.split(">");
			let index = tokens.indexOf(oldToken);
			if (index !== -1)
				tokens[index] = newToken;
		}
		else if (token[0] == "-")
		{
			let index = tokens.indexOf(token.substr(1));
			if (index !== -1)
				tokens.splice(index, 1);
		}
		else
			tokens.push(token);
	}
	return tokens.join(" ");
}

/**
 * Derives the technology requirements from a given technology template.
 * Takes into account the `supersedes` attribute.
 *
 * @param {Object} template - The template object. Loading of the template must have already occured.
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
 * @param {Object} phases - The current available store of phases.
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
