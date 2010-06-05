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

var actorViewer = {
	active: false,
	distance: 20,
	angle: 0,
	elevation: Math.PI / 6,
	actor: "actor|structures/fndn_1x1.xml",
	animation: "idle",
	// Animation playback speed
	speed: 0,
	// List of controls which should be hidden, and only shown when the Actor Viewer is active
	controls: []
};

actorViewer.toggle = function () {
	if (this.active) {
		// TODO: maybe this should switch back to whatever was selected before,
		// not necessarily PlaceObject
		Atlas.SetCurrentToolWith('PlaceObject', this.actor);
	} else {
		Atlas.SetCurrentToolWithVal('ScriptedTool', this);
	}
};

actorViewer.postToGame = function () {
	Atlas.Message.SetActorViewer(this.actor, this.animation, this.speed, false);
};

actorViewer.postLookAt = function () {
	var offset = 0.3; // slight fudge so we turn nicely when going over the top of the unit
	var pos = {
		x: this.distance*Math.cos(this.elevation)*Math.sin(this.angle) + offset*Math.cos(this.angle),
		y: this.distance*Math.sin(this.elevation),
		z: this.distance*Math.cos(this.elevation)*Math.cos(this.angle) - offset*Math.sin(this.angle)
	};
	Atlas.Message.LookAt(Atlas.RenderView.ACTOR, pos, {x:0, y:0, z:0});
};

actorViewer.setActor = function (actor) {
	this.actor = actor;
	if (this.active) {
		this.postToGame();
		Atlas.State.objectSettings.onSelectionChange();
	}
}

actorViewer.onEnable = function () {
	this.active = true;
	this.button.label = this.buttonTextActive;
	for each (ctrl in actorViewer.controls)
		ctrl.show(true);
	this.postToGame();
	Atlas.State.objectSettings.view = Atlas.RenderView.ACTOR;
	Atlas.State.objectSettings.selectedObjects = [0];
	Atlas.State.objectSettings.onSelectionChange(); // (must come after postToGame)
	this.postLookAt();
	Atlas.Message.RenderEnable(Atlas.RenderView.ACTOR);
}

actorViewer.onDisable = function () {
	this.active = false;
	this.button.label = this.buttonTextInactive;
	for each (ctrl in actorViewer.controls)
		ctrl.show(false);
	Atlas.State.objectSettings.view = Atlas.RenderView.GAME;
	Atlas.State.objectSettings.selectedObjects = [];
	Atlas.State.objectSettings.onSelectionChange();
	Atlas.Message.RenderEnable(Atlas.RenderView.GAME);
}

actorViewer.onKey = function (evt, type) {
	var code0 = '0'.charCodeAt(0);
	var code9 = '9'.charCodeAt(0);
	if (type == 'down' && evt.keyCode >= code0 && evt.keyCode <= code9) {
		// (TODO: this should probably be 'char' not 'down'; but we don't get
		// 'char' unless we return false from this function, in which case the
		// scenario editor intercepts some other keys for itself)
		Atlas.State.objectSettings.playerID = evt.keyCode - code0;
		Atlas.State.objectSettings.notifyObservers();
	}

	// Prevent keys from passing through to the scenario editor
	return true;
}

actorViewer.onMouse = function (evt) {
	var cameraChanged = false;

	var speedModifier = this.getSpeedModifier();

	if (evt.wheelRotation) {
		var speed = -1 * speedModifier;
		this.distance += evt.wheelRotation * speed / evt.wheelDelta;
		cameraChanged = true;
	}

	if (evt.leftDown || evt.rightDown) {
		this.mouseLastX = evt.x;
		this.mouseLastY = evt.y;
		this.mouseLastValid = true;
	} else if (evt.dragging && this.mouseLastValid && (evt.leftIsDown || evt.rightIsDown)) {
		var dx = evt.x - this.mouseLastX;
		var dy = evt.y - this.mouseLastY;
		this.mouseLastX = evt.x;
		this.mouseLastY = evt.y;
		this.angle += dx * Math.PI/256 * speedModifier;
		if (evt.leftIsDown)
			this.distance += (dy / 8) * speedModifier;
		else // evt.rightIsDown
			this.elevation += (dy * Math.PI/256) * speedModifier;
		cameraChanged = true;
	} else if ((evt.leftUp || evt.rightUp) && ! (evt.leftDown || evt.rightDown)) {
		// In some situations (e.g. double-clicking the title bar to
		// maximise the window) we get a dragging event without the matching
		// buttondown; so disallow dragging when all buttons were released since
		// the last buttondown.
		// (TODO: does this problem affect the scenario editor too?)
		this.mouseLastValid = false;
	}

	if (cameraChanged) {
		this.distance = Math.max(this.distance, 1/64); // don't let people fly through the origin
		this.postLookAt();
	}

	return true;
};

actorViewer.getSpeedModifier = function () { // TODO: this should be shared with the rest of the application
	if (wxGetKeyState(wxKeyCode.SHIFT) && wxGetKeyState(wxKeyCode.CONTROL))
		return 1/64;
	else if (wxGetKeyState(wxKeyCode.CONTROL))
		return 1/4;
	else if (wxGetKeyState(wxKeyCode.SHIFT))
		return 4;
	else
		return 1;
};


var g_observer;

function init(window, bottomWindow)
{
	window.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	
	var objects = Atlas.Message.GetObjectsList().objects;
	
	var objectList = new wxListBox(window, -1, wxDefaultPosition, wxDefaultSize, [],
		wxListBox.SINGLE | wxListBox.HSCROLL);
	var objectType = 0;
	objectList.onListBox = function (evt) {
		if (evt.selection == -1)
			return;
		var id = objectList.objectIDs[evt.selection];

		actorViewer.setActor(id);
		if (! actorViewer.active)
			Atlas.SetCurrentToolWith('PlaceObject', id);
	};
	setObjectFilter(objectList, objects, objectType);
	
	var groupSelector = new wxChoice(window, -1, wxDefaultPosition, wxDefaultSize,
		["Entities", "Actors (all)"]
	);
	groupSelector.onChoice = function (evt) {
		objectType = evt.selection;
		setObjectFilter(objectList, objects, objectType);
	};
	
	window.sizer.add(groupSelector, 0, wxStretch.EXPAND);
	window.sizer.add(objectList, 1, wxStretch.EXPAND);


	var viewerButton = new wxButton(window, -1, "Switch to Actor Viewer");
	actorViewer.button = viewerButton;
	actorViewer.buttonTextInactive = "Switch to Actor Viewer";
	actorViewer.buttonTextActive = "Switch to game view";
	viewerButton.onClicked = function () { actorViewer.toggle(); }
	window.sizer.add(viewerButton, 0, wxStretch.EXPAND);



	// Actor viewer settings:
	var displaySettingsBoxBox = new wxStaticBox(bottomWindow, -1, "Display settings");
	actorViewer.controls.push(displaySettingsBoxBox);
	var displaySettingsBox = new wxStaticBoxSizer(displaySettingsBoxBox, wxOrientation.VERTICAL);
	displaySettingsBox.minSize = new wxSize(140, -1);
	var displaySettings = [
		["Wireframe", "Toggle wireframe / solid rendering", "wireframe", false],
		["Move", "Toggle movement along ground when playing walk/run animations", "walk", false],
		["Ground", "Toggle the ground plane", "ground", true],
		["Shadows", "Toggle shadow rendering", "shadows", true],
		["Poly count", "Toggle polygon-count statistics - turn off ground and shadows for more useful data", "stats", false]
	];
	// NOTE: there's also a background colour setting, which isn't exposed
	// by this UI because I don't know if it's worth the effort
	for each (var setting in displaySettings) {
		var button = new wxButton(bottomWindow, -1, setting[0]);
		actorViewer.controls.push(button);
		button.toolTip = setting[1];
		// Set the default value
		Atlas.Message.SetViewParamB(Atlas.RenderView.ACTOR, setting[2], setting[3]);
		// Toggle the value on clicks
		(function (s) { // local scope for closure
			button.onClicked = function () {
				s[3] = !s[3];
				Atlas.Message.SetViewParamB(Atlas.RenderView.ACTOR, s[2], s[3]);
			};
		})(setting);
		displaySettingsBox.add(button, 0, wxStretch.EXPAND);
	}
	// TODO: It might be nice to add an "edit this actor" button
	// in the actor viewer (when we have working actor hotloading)


	var playerSelector = new wxChoice(bottomWindow, -1, wxDefaultPosition, wxDefaultSize,
		["Gaia", "Player 1", "Player 2", "Player 3", "Player 4", "Player 5", "Player 6", "Player 7", "Player 8"]
	);
	playerSelector.selection = Atlas.State.objectSettings.playerID;
	playerSelector.onChoice = function (evt) {
		Atlas.State.objectSettings.playerID = evt.selection;
		Atlas.State.objectSettings.notifyObservers();
	};
	function updatePlayerSelector() {
		playerSelector.selection = Atlas.State.objectSettings.playerID;
	}

	var animationBoxBox = new wxStaticBox(bottomWindow, -1, "Animation");
	actorViewer.controls.push(animationBoxBox);
	var animationBox = new wxStaticBoxSizer(animationBoxBox, wxOrientation.VERTICAL);
	var animationSelector = new wxChoice(bottomWindow, -1, wxDefaultPosition, wxDefaultSize,
		[ "build", "corpse", "death",
		  "gather_fruit", "gather_grain", "gather_meat", "gather_metal", "gather_stone", "gather_wood",
		  "idle", "melee", "run", "walk" ] // TODO: this list should come from the actor
	);
	animationSelector.stringSelection = "idle";
	actorViewer.controls.push(animationSelector);
	animationSelector.onChoice = function (evt) {
		actorViewer.animation = evt.string;
		actorViewer.postToGame();
	};
	var animationSpeedSizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	var speeds = [ ['Play', 1], ['Pause', 0], ['Slow', 0.1] ];
	for each (var speed in speeds) {
		var button = new wxButton(bottomWindow, -1, speed[0], wxDefaultPosition, new wxSize(50, -1));
		actorViewer.controls.push(button);
		(function (s) { // local scope for closure
			button.onClicked = function () {
				actorViewer.speed = s;
				actorViewer.postToGame();
			};
		})(speed[1]);
		animationSpeedSizer.add(button);
	}
	animationBox.add(animationSelector, 0, wxStretch.EXPAND);
	animationBox.add(animationSpeedSizer, 0, wxStretch.EXPAND);


	var animationSizer = new wxBoxSizer(wxOrientation.VERTICAL);
	animationSizer.minSize = new wxSize(160, -1);
	animationSizer.add(playerSelector, 0, wxStretch.EXPAND);
	animationSizer.add(animationBox, 0, wxStretch.EXPAND);


	for each (ctrl in actorViewer.controls)
		ctrl.show(false);


	var variationControl = new wxScrolledWindow(bottomWindow, -1);
	variationControl.setScrollRate(0, 5);
	variationControl.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	var variationControlBox = new wxStaticBoxSizer(new wxStaticBox(bottomWindow, -1, "Actor Variation"), wxOrientation.VERTICAL);
	variationControl.sizer.minSize = new wxSize(160, -1);
	variationControlBox.add(variationControl, 1);

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
		variationControl.freeze();
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
		// TODO: this sizer stuff is a bit dodgy - it often doesn't quite
		// update the sizes and scrollbars at the right points
		variationControl.thaw();
		variationControlBox.layout();
		variationControl.sizer.layout();
		bottomWindow.sizer.layout();
	}


	bottomWindow.sizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	bottomWindow.sizer.add(displaySettingsBox);
	bottomWindow.sizer.add(animationSizer);
	bottomWindow.sizer.add(variationControlBox, 0, wxStretch.EXPAND);


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

