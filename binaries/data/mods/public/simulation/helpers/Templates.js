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

Engine.RegisterGlobal("GetIdentityClasses", GetIdentityClasses);
Engine.RegisterGlobal("GetVisibleIdentityClasses", GetVisibleIdentityClasses);

