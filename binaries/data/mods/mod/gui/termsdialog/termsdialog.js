/**
 * This implements a basic "Clickwrap agreement", which is an industry standard:
 *
 * The European Court of Justice decided in the case El Majdoub (case nr C-322/14) that click-wrap agreements are acceptable under certain circumstances
 * as proof of the acceptance of terms and conditions (in the meaning of Regulation 44/2001, now replaced by Regulation 1215/2012).
 * See https://eur-lex.europa.eu/legal-content/en/TXT/HTML/?uri=uriserv%3AOJ.C_.2015.236.01.0019.01.ENG
 * The user should be able to save and print the text of the terms.
 */

var g_TermsPage;
var g_TermsFile;
var g_TermsSprintf;

function init(data)
{
	g_TermsPage = data.page;
	g_TermsFile = data.file;
	g_TermsSprintf = data.sprintf;

	Engine.GetGUIObjectByName("title").caption = data.title;
	initURLButtons(data.termsURL, data.urlButtons);
	initLanguageSelection();
}

function initURLButtons(termsURL, urlButtons)
{
	if (termsURL)
		urlButtons.unshift({
			// Translation: Label of a button that when pressed opens the Terms and Conditions in the default webbrowser.
			"caption": translate("View online"),
			"url": termsURL
		});

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
			sprintf(
				languageDropdown.selected == 1 ?
					Engine.TranslateLines(Engine.ReadFile(g_TermsFile)) :
					Engine.ReadFile(g_TermsFile),
				g_TermsSprintf);
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
