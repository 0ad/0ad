var g_TermsPage;
var g_TermsFile;

function init(data)
{
	g_TermsPage = data.page;
	g_TermsFile = data.file;

	Engine.GetGUIObjectByName("title").caption = data.title;
	initURLButtons(data.urlButtons);
	initLanguageSelection();
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

function initLanguageSelection()
{
	let languageLabel = Engine.GetGUIObjectByName("languageLabel");
	let languageLabelWidth = Engine.GetTextWidth(languageLabel.font, languageLabel.caption)
	languageLabel.size = "0 0 " + languageLabelWidth + " 100%";

	let languageDropdown = Engine.GetGUIObjectByName("languageDropdown");
	languageDropdown.size = (languageLabelWidth + 10) + " 4 100% 100%";

	languageDropdown.list = (() => {
		let displayNames = Engine.GetSupportedLocaleDisplayNames();
		let baseNames = Engine.GetSupportedLocaleBaseNames();

		// en-US
		let list = [displayNames[0]];

		// current locale
		let currentLocaleDict = Engine.GetFallbackToAvailableDictLocale(Engine.GetCurrentLocale());
		if (currentLocaleDict != baseNames[0])
			list.push(displayNames[baseNames.indexOf(currentLocaleDict)]);

		return list;
	})();

	languageDropdown.onSelectionChange = () => {
		Engine.GetGUIObjectByName("mainText").caption =
			languageDropdown.selected == 1 ?
				Engine.TranslateLines(Engine.ReadFile(g_TermsFile)) :
				Engine.ReadFile(g_TermsFile);
	};

	languageDropdown.selected = languageDropdown.list.length - 1;
}

function closeTerms(accepted)
{
	Engine.PopGuiPageCB({
		"page": g_TermsPage,
		"accepted": accepted
	});
}
