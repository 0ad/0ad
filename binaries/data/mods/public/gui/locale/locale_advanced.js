function init(initData)
{
	var languageList = Engine.GetGUIObjectByName("languageList");
	var countryList = Engine.GetGUIObjectByName("countryList");
	var resultingLocaleText = Engine.GetGUIObjectByName("resultingLocale");
	var scriptInput = Engine.GetGUIObjectByName("scriptInput");
	
	// get languageList data. Only list languages for which we have a dictionary.
	var languageListData = [];
	var languageListTmp = Engine.GetSupportedLocaleBaseNames();
	var currentLocaleLanguage = Engine.GetLocaleLanguage(initData.locale);
	for (var i=0; i<languageListTmp.length; ++i)
	{
		var lang = Engine.GetLocaleLanguage(languageListTmp[i]);
		if (lang != "" && languageListData.indexOf(lang) == -1)
			languageListData.push(lang);
	}

	
	// get countryList data (we get all countries and not only the ones we have dictionaries for)
	var countryListData = [];
	countryListData.push(translateWithContext("localeCountry", "None"));
	var countryListTmp = Engine.GetAllLocales();
	var currentLocaleCountry = Engine.GetLocaleCountry(initData.locale);
	for (var i=0; i<countryListTmp.length; ++i)
	{
		var country = Engine.GetLocaleCountry(countryListTmp[i]);
		if (country != "" && countryListData.indexOf(country) == -1)
			countryListData.push(country);
	}
	
	// fill the languageList
	languageList.list = languageListData;
	languageList.list_data = languageListData;
	if (languageList.list_data.indexOf(currentLocaleLanguage) != -1)
		languageList.selected = languageList.list_data.indexOf(currentLocaleLanguage);
	
	// fill the country list
	countryList.list = countryListData;
	countryList.list_data = countryListData;
	if (currentLocaleCountry != "")
		countryList.selected = countryList.list_data.indexOf(currentLocaleCountry);
	else
		countryList.selected = 0;
	
	// fill the script
	scriptInput.caption = Engine.GetLocaleScript(initData.locale);
}

// TODO: an onChanged event for input boxes would be useful and would allow us to avoid a tick event here.
function onTick()
{
	updateResultingLocale();
}

function cancelSetup()
{
	Engine.PopGuiPage();
}

function updateResultingLocale()
{
	var languageList = Engine.GetGUIObjectByName("languageList");
	var countryList = Engine.GetGUIObjectByName("countryList");
	var resultingLocaleText = Engine.GetGUIObjectByName("resultingLocale");
	var scriptInput = Engine.GetGUIObjectByName("scriptInput");
	var variantInput = Engine.GetGUIObjectByName("variantInput");
	var dictionaryFile = Engine.GetGUIObjectByName("dictionaryFile");
	var resultingLocaleTmp = "";

	var resultingLocaleTmp = languageList.list_data[languageList.selected];
	
	if (scriptInput.caption != "")
		resultingLocaleTmp = resultingLocaleTmp + "_" + scriptInput.caption;
	
	if (countryList.selected != -1 && countryList.list_data[countryList.selected] != translateWithContext("localeCountry", "None"))
		resultingLocaleTmp = resultingLocaleTmp + "_" + countryList.list_data[countryList.selected];

	if (Engine.ValidateLocale(resultingLocaleTmp))
	{
		resultingLocaleText.caption = resultingLocaleTmp;
		var dictionaryFileList = Engine.GetDictionariesForLocale(Engine.GetDictionaryLocale(resultingLocaleTmp));
		var dictionaryFileString = "";
		dictionaryFileList.forEach( function (entry) { dictionaryFileString = dictionaryFileString + entry + "\n"; });
		dictionaryFile.caption = dictionaryFileString;
		var acceptButton = Engine.GetGUIObjectByName("acceptButton");
		acceptButton.enabled = true;
	}
	else
	{
		resultingLocaleText.caption = translate("<invalid>");
		dictionaryFile.caption = "";
		var acceptButton = Engine.GetGUIObjectByName("acceptButton");
		acceptButton.enabled = false;
	}
}

function autoDetectLocale()
{
	var languageList = Engine.GetGUIObjectByName("languageList");
	var countryList = Engine.GetGUIObjectByName("countryList");
	var scriptInput = Engine.GetGUIObjectByName("scriptInput");
	var variantInput = Engine.GetGUIObjectByName("variantInput");
	var dictionaryFile = Engine.GetGUIObjectByName("dictionaryFile");
	
	variantInput.caption = "";
	dictionaryFile.caption = "";
	var locale = Engine.GetDictionaryLocale("");
	
	languageList.selected = languageList.list_data.indexOf(Engine.GetLocaleLanguage(locale));
	countryList.selected = countryList.selected = countryList.list_data.indexOf(Engine.GetLocaleCountry(locale));
	scriptInput.caption = Engine.GetLocaleScript(locale);
}

function applySelectedLocale()
{
	var resultingLocaleText = Engine.GetGUIObjectByName("resultingLocale");
	Engine.PopGuiPageCB(resultingLocaleText.caption);
}
