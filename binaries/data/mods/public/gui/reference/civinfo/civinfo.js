/**
 * Initialize the dropdown containing all the available civs.
 */
function init(data = {})
{
	g_Page = new CivInfoPage(data);

	if (data.civ)
		g_Page.civSelection.selectCiv(data.civ);
	else
		g_Page.civSelection.selectFirstCiv();
}
