/**
 * This file contains shared logic for applying tech modifications in GUI, AI,
 * and simulation scripts. As such it must be fully deterministic and not store
 * any global state, but each context should do its own caching as needed.
 * Also it cannot directly access the simulation and requires data passed to it.
 */
 
/**
 * Returns modified property value if at least one tech modification is found
 *   applicable to the given entity template; else, returns its original value.
 *
 * currentTechModifications: mapping of property names to modification arrays,
 *   retrieved from the intended player's TechnologyManager.
 * entityTemplateData: raw entity template object.
 * propertyName: name of the tech modification to apply.
 * propertyValue: original value of property to be modified.
 */
function GetTechModifiedProperty(currentTechModifications, entityTemplateData, propertyName, propertyValue)
{
	// Get all modifications to this value
	var modifications = currentTechModifications[propertyName];
	if (!modifications) // no modifications so return the original value
		return propertyValue;

	// TODO: will we ever need the full template?
	// Get the classes which this entity template belongs to
	var classes = [];
	if (entityTemplateData && entityTemplateData.Identity)
		classes = GetIdentityClasses(entityTemplateData.Identity);

	var retValue = propertyValue;

	for (var i in modifications)
	{
		var modification = modifications[i];
		if (DoesModificationApply(modification, classes))
		{
			// We found a match, apply the modification

			// Nothing is cumulative so that ordering doesn't matter as much as possible
			if (modification.multiply)
				retValue += (modification.multiply - 1) * propertyValue;
			else if (modification.add)
				retValue += modification.add;
			else if (modification.replace !== undefined) // This will depend on ordering because there is no choice
				retValue = modification.replace;
			else
				warn("GetTechModifiedProperty: modification format not recognised (modifying " + propertyName + "): " + uneval(modification));
		}
	}

	return retValue;
}

/**
 * Returns whether the given modification applies to the entity containing the given class list
 */
function DoesModificationApply(modification, classes)
{
	return MatchesClassList(classes, modification.affects);
}
