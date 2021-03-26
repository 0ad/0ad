/**
 * Override style so we can get a bigger primary name.
 */
g_TooltipTextFormats.namePrimaryBig.font = "sans-bold-20";
g_TooltipTextFormats.namePrimarySmall.font = "sans-bold-16";
g_TooltipTextFormats.nameSecondary.font = "sans-bold-16";

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
