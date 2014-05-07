function init()
{
	var languageList = Engine.GetGUIObjectByName("languageList");
	languageList.list = Engine.GetSupportedLocaleDisplayNames();
	languageList.list_data = Engine.GetSupportedLocaleBaseNames();

	var currentLocale = Engine.GetCurrentLocale();
	var currentLocaleDictName = Engine.GetFallbackToAvailableDictLocale(currentLocale);
	var useLongStrings = Engine.UseLongStrings();
	var index = -1;
	if (useLongStrings)
		index = languageList.list_data.indexOf("long");
	if (index == -1)
		index = languageList.list_data.indexOf(currentLocaleDictName);

	if (index != -1)
		languageList.selected = index;
	
	var localeText = Engine.GetGUIObjectByName("localeText");
	if (useLongStrings)
		localeText.caption = "long";
	else
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
	if (locale == "long")
		warn("'long' is not an actual language, just a collection of all longest strings extracted from some languages");
	else if(!Engine.ValidateLocale(locale))
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
	
	var currentLocaleDictName = Engine.GetFallbackToAvailableDictLocale(locale);
	var index = -1;
	index = languageList.list_data.indexOf(currentLocaleDictName);
	
	if (index != -1)
		languageList.selected = index;
	
	var localeText = Engine.GetGUIObjectByName("localeText");
	localeText.caption = locale;
}
