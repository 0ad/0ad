// Define the default state settings.
// (It's done this way so that this script can be dynamically reloaded,
// and won't overwrite the previous runtime state but will still pick
// up any new properties)
var defaults = {
	objectSettings: {
		view: undefined,
		selectedObjects: [],
		playerID: 1,
		actorSelections: [],
		variantGroups: []
	}
};

defaults.objectSettings.toSObjectSettings = function () {
	return {
		player: this.playerID,
		selections: this.actorSelections
	};
}

defaults.objectSettings.onSelectionChange = function () {
	if (! this.selectedObjects.length)
		return; // TODO: do something sensible here

	// TODO: Support multiple selections
	var selection = this.selectedObjects[0];

	var settings = Atlas.Message.GetObjectSettings(this.view, selection).settings;

	this.playerID = settings.player;
	this.actorSelections = settings.selections;
	this.variantGroups = settings.variantgroups;

	this.notifyObservers();
}

/**
 * Returns the current actor variation (as a list of chosen variants), based
 * on its variant groups and the selection strings. This is equivalent to the
 * variation rendered by the game.
 */
defaults.objectSettings.getActorVariation = function ()
{
	var selectionMap = {};
	for each (var s in this.actorSelections)
		selectionMap[s] = 1;

	var variation = [];
	GROUP: for each (var group in this.variantGroups) {
		for each (var variant in group) {
			if (variant in selectionMap) {
				variation.push(variant);
				continue GROUP;
			}
		}
		// None selected; default to first
		variation.push(group[0]);
	}
	return variation;
}

// Merges the 'defs' tree into 'obj', overwriting any old values with
// the same keys
function setDefaults(defs, obj)
{
	for (var k in defs) {
		if (k in obj)
			setDefaults(defs[k], obj[k]);
		else
			obj[k] = defaults[k];
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

function postObjectSettingsToGame(objectSettings)
{
	if (objectSettings.selectedObjects.length)
		Atlas.Message.SetObjectSettings(objectSettings.view,
			objectSettings.selectedObjects[0], objectSettings.toSObjectSettings());
}

function init()
{
	setDefaults(defaults, Atlas.State);

	makeObservable(Atlas.State.objectSettings);
	Atlas.State.objectSettings.registerObserver(postObjectSettingsToGame);
}

function deinit() {
	Atlas.State.objectSettings.unregisterObserver(postObjectSettingsToGame);
}

