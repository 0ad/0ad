var g_TermsPage = "";

function init(data)
{
	g_TermsPage = data.page;

	Engine.GetGUIObjectByName("title").caption = data.title;

	Engine.GetGUIObjectByName("mainText").caption =
		Engine.FileExists(data.file) ?
		Engine.TranslateLines(Engine.ReadFile(data.file)) :
		data.file;
}

function closeTerms(accepted)
{
	Engine.PopGuiPageCB({
		"page": g_TermsPage,
		"accepted": accepted
	});
}
