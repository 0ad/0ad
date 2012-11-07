/**
 * Return template.Identity.Classes._string if exists
 */
function GetTemplateIdentityClassesString(template)
{
	var identityClassesString = undefined;
	if (template.Identity && template.Identity.Classes && "_string" in template.Identity.Classes)
		identityClassesString = template.Identity.Classes._string;
	return identityClassesString;
}

/**
 * Check whether template.Identity.Classes contains specified class
 */
function TemplateHasIdentityClass(template, className)
{
	var identityClassesString = GetTemplateIdentityClassesString(template);
	var hasClass = identityClassesString && identityClassesString.indexOf(className) != -1;
	return hasClass;
}

Engine.RegisterGlobal("GetTemplateIdentityClassesString", GetTemplateIdentityClassesString);
Engine.RegisterGlobal("TemplateHasIdentityClass", TemplateHasIdentityClass);

