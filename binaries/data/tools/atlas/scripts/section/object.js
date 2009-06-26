function setObjectFilter(objectList, objects, type)
{
	objectList.freeze();
	objectList.clear();
	var ids = [];
	for each (var object in objects) {
		if (object.type == type) {
			ids.push(object.id);
			objectList.append(object.name);
		}
	}
	objectList.objectIDs = ids;
	objectList.thaw();
}

var g_observer;

function init(window, bottomWindow)
{
	window.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	
	var objects = Atlas.Message.GetObjectsList().objects;
	
	var objectList = new wxListBox(window, -1, wxDefaultPosition, wxDefaultSize, [],
		wxListBox.SINGLE | wxListBox.HSCROLL);
	objectList.onListBox = function (evt) {
		if (evt.selection == -1)
			return;
		var id = objectList.objectIDs[evt.selection];
		Atlas.SetCurrentToolWith('PlaceObject', id);
	};
	setObjectFilter(objectList, objects, 0);
	
	var groupSelector = new wxChoice(window, -1, wxDefaultPosition, wxDefaultSize,
		["Entities", "Actors (all)"]
	);
	groupSelector.onChoice = function (evt) {
		setObjectFilter(objectList, objects, evt.selection);
	};
	
	window.sizer.add(groupSelector, 0, wxStretch.EXPAND);
	window.sizer.add(objectList, 1, wxStretch.EXPAND);
	
	
	bottomWindow.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	var playerSelector = new wxChoice(bottomWindow, -1, wxDefaultPosition, wxDefaultSize,
		["Gaia", "Player 1", "Player 2", "Player 3", "Player 4", "Player 5", "Player 6", "Player 7", "Player 8"]
	);
	bottomWindow.sizer.add(playerSelector);

	playerSelector.selection = Atlas.State.objectSettings.playerID;
	playerSelector.onChoice = function (evt) {
		Atlas.State.objectSettings.playerID = evt.selection;
		Atlas.State.objectSettings.notifyObservers();
	};
	function updatePlayerSelector() {
		playerSelector.selection = Atlas.State.objectSettings.playerID;
	}


	var variationControl = new wxScrolledWindow(bottomWindow, -1);
	variationControl.setScrollRate(0, 5);
	variationControl.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	var variationControlBox = new wxStaticBoxSizer(new wxStaticBox(bottomWindow, -1, "Actor Variation"), wxOrientation.VERTICAL);
	variationControl.sizer.minSize = new wxSize(160, -1);
	variationControlBox.add(variationControl, 1);
	bottomWindow.sizer.add(variationControlBox, 1);

	function onVariationSelect() {
		// It's possible for a variant name to appear in multiple groups.
		// If so, assume that all the names in each group are the same, so
		// we don't have to worry about some impossible combinations (e.g.
		// one group "a,b", a second "b,c", and a third "c,a", where's there's
		// no set of selections that matches one (and only one) of each group).
		//
		// So... When a combo box is changed from 'a' to 'b', add 'b' to the new
		// selections and make sure any other combo boxes containing both 'a' and
		// 'b' no longer contain 'a'.

		var sel = this.stringSelection;
		var selections = [ sel ];
		for each (var c in variationControl.children)
			if (c.findString(sel) == wxNOT_FOUND)
				selections.push(c.stringSelection);

		Atlas.State.objectSettings.actorSelections = selections;
		Atlas.State.objectSettings.notifyObservers();
	}

	function updateVariationControl() {
		variationControl.sizer.clear(true);
		var settings = Atlas.State.objectSettings;
		var variation = settings.getActorVariation();
		for (var i = 0; i < settings.variantGroups.length; ++i) {
			var choice = new wxChoice(variationControl, -1, wxDefaultPosition, new wxSize(80, -1),
				settings.variantGroups[i]);
			choice.onChoice = onVariationSelect;
			choice.stringSelection = variation[i];
			variationControl.sizer.add(choice, 0, wxStretch.EXPAND);
		}
		variationControlBox.layout();
		variationControl.sizer.layout();
		bottomWindow.sizer.layout();
	}

	g_observer = function() {
		updatePlayerSelector();
		updateVariationControl();
	};
	Atlas.State.objectSettings.registerObserver(g_observer);

	// Initialise the controls
	g_observer();
}

function deinit()
{
	Atlas.State.objectSettings.unregisterObserver(g_observer);
}

