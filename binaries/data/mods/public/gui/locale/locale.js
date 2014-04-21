function init()
{
	var languageList = Engine.GetGUIObjectByName("languageList");
	languageList.list = Engine.GetSupportedLocaleDisplayNames();
	languageList.list_data = Engine.GetSupportedLocaleBaseNames();

	var currentLocale = Engine.GetCurrentLocale();
	var currentLocaleBaseName = Engine.GetLocaleBaseName(currentLocale);
	var currentLocaleLanguage = Engine.GetLocaleLanguage(currentLocale);
	if (languageList.list_data.indexOf(currentLocaleBaseName) != -1)
		languageList.selected = languageList.list_data.indexOf(currentLocaleBaseName);
	else if (languageList.list_data.indexOf(currentLocaleLanguage) != -1)
		languageList.selected = languageList.list_data.indexOf(currentLocaleLanguage);
	
	var localeText = Engine.GetGUIObjectByName("localeText");
	localeText.caption = currentLocale;
}

function cancelSetup()
{
	Engine.PopGuiPage();
}

function applySelectedLocale()
{
	var localeText = Engine.GetGUIObjectByName("localeText");
	if(!Engine.SaveLocale(localeText.caption))
	{
		warn("Selected locale could not be saved in the configuration!");
		return;
	}
	Engine.ReevaluateCurrentLocaleAndReload();
	Engine.SwitchGuiPage("page_pregame.xml");
}

function languageSelectionChanged()
{
	var languageList = Engine.GetGUIObjectByName("languageList");
	var locale = languageList.list_data[languageList.selected];
	if(!Engine.ValidateLocale(locale))
		warn("Selected locale is not valid! This is not expected, please report the issue.");
	var localeText = Engine.GetGUIObjectByName("localeText");
	localeText.caption = locale;
}

function openAdvancedMenu()
{
	var localeText = Engine.GetGUIObjectByName("localeText");
	Engine.PushGuiPage("page_locale_advanced.xml", { "callback": "applyFromAdvancedMenu", "locale": localeText.caption } );
}

function applyFromAdvancedMenu(locale)
{
	var languageList = Engine.GetGUIObjectByName("languageList");
	
	var localeBaseName = Engine.GetLocaleBaseName(locale);
	var localeLanguage = Engine.GetLocaleLanguage(locale);
	if (languageList.list_data.indexOf(localeBaseName) != -1)
		languageList.selected = languageList.list_data.indexOf(localeBaseName);
	else if (languageList.list_data.indexOf(localeLanguage) != -1)
		languageList.selected = languageList.list_data.indexOf(localeLanguage);
	
	var localeText = Engine.GetGUIObjectByName("localeText");
	localeText.caption = locale;
}
