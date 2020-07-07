/**
 * This needs to stay in the global scope, as it is used by various functions
 * within gui/common/tooltip.js
 */
var g_ResourceData = new Resources();

var g_Page;

/**
 * This is needed because getEntityCostTooltip in tooltip.js needs to get
 * the template data of the different wallSet pieces. In the session this
 * function does some caching, but here we do that in the TemplateLoader
 * class already.
 */
function GetTemplateData(templateName)
{
	let template = g_Page.TemplateLoader.loadEntityTemplate(templateName, g_Page.activeCiv);
	return GetTemplateDataHelper(template, null, g_Page.TemplateLoader.auraData, g_Page.TemplateParser.getModifiers(g_Page.activeCiv));
}

/**
 * This would ideally be an Engine method.
 * Or part of globalscripts. Either would be better than here.
 */
function TechnologyTemplateExists(templateName)
{
	return Engine.FileExists(g_Page.TemplateLoader.TechnologyPath + templateName + ".json");
}
