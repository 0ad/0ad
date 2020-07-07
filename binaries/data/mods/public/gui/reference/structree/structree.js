/**
 * Initialize the page
 *
 * @param {object} data - Parameters passed from the code that calls this page into existence.
 */
function init(data = {})
{
	g_Page = new StructreePage(data);

	if (data.civ)
		g_Page.civSelection.selectCiv(data.civ);
	else
		g_Page.civSelection.selectFirstCiv();
}
