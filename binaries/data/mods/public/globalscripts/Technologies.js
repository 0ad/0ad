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
 * Returns whether the given modification applies to the entity containing the given class list
 */
function DoesModificationApply(modification, classes)
{
	return MatchesClassList(classes, modification.affects);
}
