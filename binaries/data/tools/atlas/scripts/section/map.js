
function setupSimTest(window, rmsPanel)
{
	var state = 'inactive'; // inactive, playing
	var speed = 0;
	
	function play(newSpeed)
	{
		if (state == 'inactive')
		{
			Atlas.Message.SimStateSave('default');
			Atlas.Message.GuiSwitchPage('page_session.xml');
		}
		Atlas.Message.SimPlay(newSpeed);
		state = 'playing';
		speed = newSpeed;
		updateEnableStatus();
	}

	function reset()
	{
		if (state == 'playing')
		{
			Atlas.Message.SimStateRestore('default');
			Atlas.Message.GuiSwitchPage('page_atlas.xml');
		}
		Atlas.Message.SimPlay(0);
		state = 'inactive';
		speed = 0;
		updateEnableStatus();
	}
	
	function updateEnableStatus()
	{
		for (var i in buttons)
		{
			buttons[i][3].enable = buttons[i][2]();
		}
		
		if (state == 'playing')
		{
			// Disable RMS controls
			rmsPanel.enable = false;
		}
		else
		{
			rmsPanel.enable = true;
		}
	}

	var speeds = [ 1/8, 1, 8 ];
	
	var buttons = [
		/* button label; click command; active condition */
		[ 'Play', function () { play(speeds[1]) }, function () { return !(state == 'playing' && speed == speeds[1]) } ],
		[ 'Fast', function () { play(speeds[2]) }, function () { return !(state == 'playing' && speed == speeds[2]) } ],
		[ 'Slow', function () { play(speeds[0]) }, function () { return !(state == 'playing' && speed == speeds[0]) } ],
		[ 'Pause', function () { play(0) }, function () { return state == 'playing' && speed != 0 } ],
		[ 'Reset', function () { reset() }, function () { return state != 'inactive' } ],
	];
	
	var sizer = new wxStaticBoxSizer(new wxStaticBox(window, -1, 'Simulation test'), wxOrientation.HORIZONTAL);
	for each (var b in buttons)
	{
		var button = new wxButton(window, -1, b[0]);
		button.onClicked = b[1];
		sizer.add(button, 1);
		b[3] = button;
	}
	updateEnableStatus();
	window.sizer.add(sizer, 0, wxStretch.EXPAND | wxDirection.LEFT|wxDirection.RIGHT, 2);
	
}

//TODO: Would be nicer not to use an extra settings property, this is to avoid the observer data getting written to maps
//	perhaps that single element can be excluded
var defaults = {
	mapSettings : {
		settings: {
			Name: 'Untitled',
			Description : '',
			PlayerData : []
		}
	}
};

var g_observer;

// Flag if we should only notify other scripts not this one (prevents recursion)
var g_externalNotify;

// Merges the 'defs' tree into 'obj', overwriting any old values with
// the same keys
function setDefaults(defs, obj)
{
	if (defs instanceof Object)
	{
		for (var k in defs) {
			if (obj instanceof Object && k in obj)
				setDefaults(defs[k], obj[k]);
			else
				obj[k] = defs[k];
		}
	}
}

// Read civ data and player defaults
function getStartingData()
{	
	// Load civilization data
	if (!Atlas.State.CivData)
	{
		Atlas.State.CivData = [];
		Atlas.State.CivNames = [];
		Atlas.State.CivCodes = [];
		
		var dataArray = Atlas.Message.GetCivData().data;
		if (dataArray)
		{	// parse JSON strings into objects
			for (var i = 0; i < dataArray.length; ++i)
			{
				var civ = JSON.parse(dataArray[i]);
				if (civ)
				{
					Atlas.State.CivData.push(civ);
					Atlas.State.CivNames.push(civ.Name);
					Atlas.State.CivCodes.push(civ.Code);
				}
			}
		}
	}
	
	// Load player default data (names, civs, colors, etc)
	if (!Atlas.State.PlayerDefaults)
	{
		var rawData = Atlas.Message.GetPlayerDefaults().defaults;
		if(rawData)
		{
			Atlas.State.PlayerDefaults = JSON.parse(rawData).PlayerData;
		}
	}
}

function getMapSettings()
{
	// Get map settings to populate GUI
	var settings = Atlas.Message.GetMapSettings().settings;
	if (settings)
	{
		Atlas.State.mapSettings.settings = JSON.parse(settings);
	}
}

function makeObservable(obj)
{
	// If the object has already been set up as observable, don't do any more work.
	// (In particular, don't delete any old observers, so they don't get lost when
	// this script is reloaded.)
	if (typeof obj._observers != 'undefined')
		return;

	obj._observers = [];
	obj.registerObserver = function (callback) {
		this._observers.push(callback);
	}
	obj.unregisterObserver = function (callback) {
		this._observers = this._observers.filter(function (o) { return o !== callback });
	}
	obj.notifyObservers = function () {
		for each (var o in this._observers)
			o(this);
	};
}

function init(window)
{
	window.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	
	// Don't overwrite old data when script is hotloaded
	setDefaults(defaults, Atlas.State);
		
	///////////////////////////////////////////////////////////////////////
	// Map setting controls
	// Map name
	var settingsSizer = new wxStaticBoxSizer(new wxStaticBox(window, -1, 'Map settings'), wxOrientation.VERTICAL);
	var boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	boxSizer.add(new wxStaticText(window, -1, 'Name'), 0, wxAlignment.CENTER_VERTICAL);
	boxSizer.add(10, 0);
	var mapNameCtrl = new wxTextCtrl(window, -1,  'Untitled');
	mapNameCtrl.toolTip = "Enter a name for the map here";
	mapNameCtrl.onText = function(evt)
		{
			Atlas.State.mapSettings.settings.Name = evt.string;
		};
	boxSizer.add(mapNameCtrl, 1);
	function updateMapName()
	{
		mapNameCtrl.value = Atlas.State.mapSettings.settings.Name ? Atlas.State.mapSettings.settings.Name : 'Untitled';
	}
	settingsSizer.add(boxSizer, 0, wxStretch.EXPAND);
	
	settingsSizer.add(5, 5, 0);	// Add space
	
	// Map description
	boxSizer = new wxBoxSizer(wxOrientation.VERTICAL);
	boxSizer.add(new wxStaticText(window, -1, 'Description'), wxAlignment.RIGHT);
	var mapDescCtrl = new wxTextCtrl(window, -1, '', wxDefaultPosition, new wxSize(-1, 100), wxTextCtrl.MULTILINE);
	mapDescCtrl.toolTip = "Enter an interesting description for the map here";
	mapDescCtrl.onText = function(evt)
		{
			Atlas.State.mapSettings.settings.Description = evt.string;
		};
	boxSizer.add(mapDescCtrl, 0, wxStretch.EXPAND);
	function updateMapDesc()
	{
		mapDescCtrl.value = Atlas.State.mapSettings.settings.Description ? Atlas.State.mapSettings.settings.Description : '';
	}
	settingsSizer.add(boxSizer, 0, wxStretch.EXPAND);
	
	settingsSizer.add(5, 5, 0);	// Add space
	
	// Reveal map
	boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	boxSizer.add(new wxStaticText(window, -1, 'Reveal map'), wxAlignment.LEFT);
	boxSizer.add(10, 0);
	var revealMapCtrl = new wxCheckBox(window, -1, '');
	revealMapCtrl.toolTip = 'If checked, players will see the whole map explored and their enemies';
	revealMapCtrl.onCheckBox = function(evt)
		{
			Atlas.State.mapSettings.settings.RevealMap = evt.checked;
		};
	boxSizer.add(revealMapCtrl);
	function updateRevealMap()
	{	// Defaults to false if setting doesn't exist
		revealMapCtrl.value = Atlas.State.mapSettings.settings.RevealMap;
	}
	settingsSizer.add(boxSizer, 0, wxStretch.EXPAND);
	
	settingsSizer.add(5, 5, 0);	// Add space
	
	// Game type
	boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	boxSizer.add(new wxStaticText(window, -1, 'Game type'), 0, wxAlignment.CENTER_VERTICAL);
	boxSizer.add(10, 0);
	var gameTypes = ["conquest", "endless"];
	var gameTypeCtrl = new wxChoice(window, -1, wxDefaultPosition, new wxSize(100,-1), gameTypes);
	gameTypeCtrl.toolTip = 'Select the game type / victory conditions';
	gameTypeCtrl.onChoice = function (evt)
		{
			if (evt.selection != -1)
			{
				Atlas.State.mapSettings.settings.GameType = evt.string;
			}
		};
	boxSizer.add(gameTypeCtrl);
	function updateGameType()
	{
		var idx = Atlas.State.mapSettings.settings.GameType ? gameTypes.indexOf(Atlas.State.mapSettings.settings.GameType) : 0;
		gameTypeCtrl.selection = idx;
	}
	settingsSizer.add(boxSizer, 0, wxStretch.EXPAND);
	
	settingsSizer.add(5, 5, 0);	// Add space
	
	// Number of players
	boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	boxSizer.add(new wxStaticText(window, -1, 'Number of Players'), 0, wxAlignment.CENTER_VERTICAL);
	boxSizer.add(10, 0);
	var numPlayers = new wxSpinCtrl(window, -1, '8', wxDefaultPosition, new wxSize(40, -1));
	numPlayers.toolTip = 'Select the number of players on this map';
	numPlayers.setRange(1, 8);
	numPlayers.onSpinCtrl = function(evt)
		{
			// Adjust player data accordingly
			if (!Atlas.State.mapSettings.settings.PlayerData)
				Atlas.State.mapSettings.settings.PlayerData = [];
				
			Atlas.State.numPlayers = evt.position;
			var curPlayers = Atlas.State.mapSettings.settings.PlayerData.length;
			
			if (curPlayers < evt.position)
			{	// Add extra players from default
				for (var i = curPlayers; i < evt.position; ++i)
				{
					Atlas.State.mapSettings.settings.PlayerData.push(Atlas.State.PlayerDefaults[i]);
				}
			}
			else
			{	//Remove extra players
				Atlas.State.mapSettings.settings.PlayerData = Atlas.State.mapSettings.settings.PlayerData.slice(0, evt.position);
			}
			
			g_externalNotify = true;
			Atlas.State.mapSettings.notifyObservers();
			g_externalNotify = false;
		};
	boxSizer.add(numPlayers);
	function updateNumPlayers()
	{
		numPlayers.value = Atlas.State.mapSettings.settings.PlayerData ? Atlas.State.mapSettings.settings.PlayerData.length : 8;
	}
	settingsSizer.add(boxSizer, 0, wxStretch.EXPAND);
	
	window.sizer.add(settingsSizer, 0, wxStretch.EXPAND | wxDirection.LEFT|wxDirection.RIGHT, 2);
	
	///////////////////////////////////////////////////////////////////////
	// Keyword list
	
	var keywordSizer = new wxStaticBoxSizer(new wxStaticBox(window, -1, 'Keywords'), wxOrientation.VERTICAL);
	var keywords = ["demo", "hidden"];
	var keywordCtrl = new wxListBox(window, -1, wxDefaultPosition, wxDefaultSize, keywords, wxListBox.MULTIPLE);
	keywordCtrl.toolTip = 'Select keyword(s) to apply to this map';
	keywordCtrl.onListBox = function()
		{
			var kws = [];
			// Convert selections to keywords array
			var selections = this.selections;
			for (var i = 0; i < selections.length; ++i)
			{
				kws.push(keywords[selections[i]]);
			}
			Atlas.State.mapSettings.settings.Keywords = kws;
		};
	function updateKeywords()
	{
		var kws = Atlas.State.mapSettings.settings.Keywords;
		if (kws)
		{
			for (var i = 0; i < kws.length; ++i)
			{
				var idx = keywords.indexOf(kws[i]);
				if (idx != -1)
				{
					// TODO: There doesn't appear to be a way to set selections
				}
				else
				{	// New keyword, add to list
					keywordCtrl.insertItems([kws[i]]);
				}
			}
		}
	}
	
	keywordSizer.add(keywordCtrl, 0, wxStretch.EXPAND);
	
	window.sizer.add(keywordSizer, 0, wxStretch.EXPAND | wxDirection.LEFT|wxDirection.RIGHT, 2);
	
	///////////////////////////////////////////////////////////////////////
	// Random map controls
	var rmsPanel = new wxPanel(window, -1);
	var rmsSizer = new wxStaticBoxSizer(new wxStaticBox(rmsPanel, -1, 'Random map'), wxOrientation.VERTICAL);
	
	var scriptNames = [];
	var scriptData = [];
	boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	var rmsChoice = new wxChoice(rmsPanel, -1, wxDefaultPosition, wxDefaultSize, scriptNames);
	rmsChoice.toolTip = "Select the random map script to run";
	function loadScriptChoices()
	{
		// Reload RMS data
		scriptNames = [];
		scriptData = [];
		
		// Get array of RMS data
		var rawData = Atlas.Message.GetRMSData().data;
		for (var i = 0; i < rawData.length; ++i)
		{
			// Parse JSON strings to objects
			var data = JSON.parse(rawData[i]);
			if (data && data.settings)
			{
				scriptData.push(data);
				scriptNames.push(data.settings.Name);
			}
		}
		
		// Add script names to choice control
		rmsChoice.clear();
		rmsChoice.append(scriptNames);
		rmsChoice.selection = 0;
	}
	boxSizer.add(rmsChoice, 1, wxAlignment.CENTER_VERTICAL);
	boxSizer.add(5, 0);
	var rmsReload = new wxButton(rmsPanel, -1, 'R', wxDefaultPosition, new wxSize(20, -1));
	rmsReload.toolTip = 'Click to refresh random map list';
	rmsReload.onClicked = function()
		{
			loadScriptChoices();
		};
	boxSizer.add(rmsReload, 0, wxAlignment.CENTER_VERTICAL);
	rmsSizer.add(boxSizer, 0, wxStretch.EXPAND | wxDirection.ALL, 2);
	
	boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	boxSizer.add(new wxStaticText(rmsPanel, -1, 'Map size'), 0, wxAlignment.CENTER_VERTICAL);
	boxSizer.add(10, 0);
	// TODO: Get this data from single location (currently specified here, Atlas\lists.xml, and game setup)
	var sizeNames = ["Tiny", "Small", "Medium", "Normal", "Large", "Very Large", "Giant"];
	var sizeTiles = [128, 192, 256, 320, 384, 448, 512];
	var sizeChoice = new wxChoice(rmsPanel, -1, wxDefaultPosition, wxDefaultSize, sizeNames);
	var numChoices = sizeNames.length;
	sizeChoice.toolTip = 'Select the desired map size\n'+sizeNames[0]+' = '+sizeTiles[0]+' patches, '+sizeNames[numChoices-1]+' = '+sizeTiles[numChoices-1]+' patches';
	sizeChoice.selection = 0;
	boxSizer.add(sizeChoice, 1);
	rmsSizer.add(boxSizer, 0, wxStretch.EXPAND | wxDirection.ALL, 2);
	
	boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	var generateBtn = new wxButton(rmsPanel, -1, 'Generate');
	generateBtn.toolTip = 'Generate random map using selected script';
	generateBtn.onClicked = function ()
		{
			var selection = rmsChoice.selection;
			if (selection != -1)
			{
				var RMSData = scriptData[selection].settings;
				if (RMSData)
				{
					if (useRandomCtrl.value)
					{	// Generate random seed
						generateRandomSeed();
					}
					
					// Base terrains must be array
					var terrainArray = [];
					if (RMSData.BaseTerrain instanceof Array)
					{
						terrainArray = RMSData.BaseTerrain;
					}
					else
					{	// Add string to array
						terrainArray.push(RMSData.BaseTerrain);
					}
					
					// Complete map settings
					Atlas.State.mapSettings.settings.Seed = Atlas.State.Seed ? Atlas.State.Seed : 0;
					Atlas.State.mapSettings.settings.Size = sizeTiles[sizeChoice.selection];
					Atlas.State.mapSettings.settings.BaseTerrain = terrainArray;
					Atlas.State.mapSettings.settings.BaseHeight = RMSData.BaseHeight;
					
					// Generate map
					var ret = Atlas.Message.GenerateMap(RMSData.Script, JSON.stringify(Atlas.State.mapSettings.settings));
					
					// Check for error
					if (ret.status < 0)
					{
						wxMessageBox("Random map script '"+RMSData.Script+"' failed. Loading blank map.");
						
						Atlas.Message.LoadMap("_default");
					}
					Atlas.State.mapSettings.notifyObservers();
				}
			}
		};
	boxSizer.add(generateBtn, 0);
	boxSizer.add(5, 0);
	boxSizer.add(new wxStaticText(rmsPanel, -1, 'Random seed'), 0, wxAlignment.CENTER_VERTICAL);
	boxSizer.add(5, 0);
	var useRandomCtrl = new wxCheckBox(rmsPanel, -1, '');
	useRandomCtrl.toolTip = 'Always generate a new random seed';
	useRandomCtrl.value = true;
	boxSizer.add(useRandomCtrl, 0, wxAlignment.CENTER_VERTICAL);
	rmsSizer.add(boxSizer, 0, wxStretch.EXPAND | wxDirection.ALL, 2);
	
	boxSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	boxSizer.add(new wxStaticText(rmsPanel, -1, 'Seed'), 0, wxAlignment.CENTER_VERTICAL);
	boxSizer.add(10, 0);
	var seedCtrl = new wxTextCtrl(rmsPanel, -1, '', wxDefaultPosition, new wxSize(60,-1));
	seedCtrl.toolTip = 'Enter a numeric seed value here';
	seedCtrl.onText = function()
		{
			var num = parseInt(this.value, 10);
			if (!isNaN(num))
			{
				Atlas.State.Seed = num;
			}
			else
			{
				// TODO: Can allow text by hashing it into a seed value - for fun?
			}
		};
	boxSizer.add(seedCtrl, 1);
	function generateRandomSeed()
	{
		seedCtrl.value = Math.floor(Math.random() * 2147483648)+ 1;
	}
	rmsSizer.add(boxSizer, 0, wxStretch.EXPAND | wxDirection.ALL, 2);
	
	// TODO: Possibly have controls for base height + terrain to override RMS
	
	rmsPanel.sizer = rmsSizer;
	window.sizer.add(rmsPanel, 0, wxStretch.EXPAND | wxDirection.LEFT|wxDirection.RIGHT, 2);
	
	///////////////////////////////////////////////////////////////////////
	// Simulation controls
	setupSimTest(window, rmsPanel);
	
	
	// Initial seed
	seedCtrl.value = 0;
	
	g_observer = function()
	{
		if (!g_externalNotify)
		{
			// If we don't have civ data or player defaults yet, get those
			getStartingData();
			
			// Load map settings from engine
			getMapSettings();
			
			// Load RMS names from engine
			loadScriptChoices();
			
			// Update UI controls
			updateMapName();
			updateMapDesc();
			updateRevealMap();
			updateGameType();
			updateNumPlayers();
			updateKeywords();
		}
	}
	
	// Set up observers (so that this script can be notified when map data is loaded)
	makeObservable(Atlas.State.mapSettings);
	
	Atlas.State.mapSettings.registerObserver(g_observer);
}

function deinit()
{
	Atlas.State.mapSettings.unregisterObserver(g_observer);
}
