/**
 * Gets an array of all classes for this identity template
 */
function GetIdentityClasses(template)
{
	var classList = [];
	if (template.Classes && template.Classes._string)
		classList = classList.concat(template.Classes._string.split(/\s+/));

	if (template.VisibleClasses && template.VisibleClasses._string)
		classList = classList.concat(template.VisibleClasses._string.split(/\s+/));

	if (template.Rank)
		classList = classList.concat(template.Rank);
	return classList;
}

/**
 * Gets an array with all classes for this identity template
 * that should be shown in the GUI
 */
function GetVisibleIdentityClasses(template)
{
	if (template.VisibleClasses && template.VisibleClasses._string)
		return template.VisibleClasses._string.split(/\s+/);
	return [];
}

/**
 * Check if the classes given in the identity template
 * match a list of classes
 * @param classes List of the classes to check against
 * @param match Either a string in the form 
 *     "Class1 Class2+Class3"
 * where spaces are handled as OR and '+'-signs as AND,
 * Or a list in the form
 *     [["Class1"], ["Class2", "Class3"]]
 * where the outer list is combined as OR, and the inner lists are AND-ed
 * Or a hybrid format containing a list of strings, where the list is
 * combined as OR, and the strings are split by space and '+' and AND-ed
 *
 * @return undefined if there are no classes or no match object
 * true if the the logical combination in the match object matches the classes
 * false otherwise
 */
function MatchesClassList(classes, match)
{
	if (!match || !classes)
		return undefined;
	// transform the string to an array
	if (typeof match == "string")
		match = match.split(/\s+/);
	
	for (var sublist of match)
	{
		// if the elements are still strings, split them by space or by '+'
		if (typeof sublist == "string")
			sublist = sublist.split(/[+\s]+/);
		if (sublist.every(function(c) { return classes.indexOf(c) != -1; }))
			return true;
	}

	return false;
}

