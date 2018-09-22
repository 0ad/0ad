var g_TermsPage;

function init(data)
{
	g_TermsPage = data.page;

	Engine.GetGUIObjectByName("title").caption = data.title;

	Engine.GetGUIObjectByName("mainText").caption =
		Engine.FileExists(data.file) ?
		Engine.TranslateLines(Engine.ReadFile(data.file)) :
		data.file;

	initURLButtons(data.urlButtons);
}

function initURLButtons(urlButtons)
{
	urlButtons.forEach((urlButton, i) => {

		let button = Engine.GetGUIObjectByName("button[" + i + "]");
		button.caption = urlButton.caption;
		button.hidden = false;
		button.tooltip = sprintf(translate("Open %(url)s in the browser."), {
			"url": urlButton.url
		});
		button.onPress = () => {
			openURL(urlButton.url);
		};
	});
}

function closeTerms(accepted)
{
	Engine.PopGuiPageCB({
		"page": g_TermsPage,
		"accepted": accepted
	});
}
