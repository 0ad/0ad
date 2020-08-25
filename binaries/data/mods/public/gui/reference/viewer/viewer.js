/**
 * Override style so we can get a bigger specific name.
 */
g_TooltipTextFormats.nameSpecificBig.font = "sans-bold-20";
g_TooltipTextFormats.nameSpecificSmall.font = "sans-bold-16";
g_TooltipTextFormats.nameGeneric.font = "sans-bold-16";

/**
 * Page initialisation. May also eventually pre-draw/arrange objects.
 *
 * @param {Object} data - Contains the civCode and the name of the template to display.
 * @param {string} data.templateName
 * @param {string} [data.civ]
 */
function init(data)
{
	g_Page = new ViewerPage();
	g_Page.selectTemplate(data);
}
