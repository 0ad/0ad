var g_Terms = {};

function initTerms(terms)
{
	g_Terms = terms;
}

function openTerms(page)
{
	Engine.PushGuiPage("page_termsdialog.xml", {
		"file": g_Terms[page].file,
		"title": g_Terms[page].title,
		"urlButtons": g_Terms[page].urlButtons || [],
		"page": page,
		"callback": "acceptTerms"
	});
}

function acceptTerms(data)
{
	g_Terms[data.page].accepted = data.accepted;

	let value = data.accepted ? getTermsHash(data.page) : "0";
	Engine.ConfigDB_CreateValue("user", g_Terms[data.page].config, value);
	Engine.ConfigDB_WriteValueToFile("user", g_Terms[data.page].config, value, "config/user.cfg");

	if (g_Terms[data.page].callback)
		g_Terms[data.page].callback(data);
}

function checkTerms()
{
	for (let page in g_Terms)
		if (!g_Terms[page].accepted)
			return g_Terms[page].instruction || page;

	return "";
}

function getTermsHash(page)
{
	return Engine.CalculateMD5(
		(g_Terms[page].salt ? g_Terms[page].salt() : "") +
		(Engine.FileExists(g_Terms[page].file) ? Engine.ReadFile(g_Terms[page].file) : g_Terms[page].file));
}

function loadTermsAcceptance()
{
	for (let page in g_Terms)
		g_Terms[page].accepted = Engine.ConfigDB_GetValue("user", g_Terms[page].config) == getTermsHash(page);
}
