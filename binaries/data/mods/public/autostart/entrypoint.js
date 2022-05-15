/**
 * Implement autostart for the public mod.
 * We want to avoid relying on too many specific files, so we'll mock a few engine functions.
 * Depending on the path, these may get overwritten with the real function.
 */

Engine.HasXmppClient = () => false;
Engine.SetRankedGame = () => {};
Engine.TextureExists = () => false;
Engine.PushGuiPage = () => {};
Engine.SwitchGuiPage = () => {};

var translateObjectKeys = () => {}
var translate = x => x;
var translateWithContext = x => x;

// Required for functions such as sprintf.
Engine.LoadScript("globalscripts/");
// MsgBox is used in the failure path.
// TODO: clean this up and show errors better in the non-visual path.
Engine.LoadScript("gui/common/functions_msgbox.js");

var autostartInstance;

function autostartClient(initData)
{
	autostartInstance = new AutoStartClient(initData);
}

/**
 * This path depends on files currently stored under gui/, which should be moved.
 * The best place would probably be a new 'engine' mod, independent from the 'mod' mod and the public mod.
 */
function autostartHost(initData, networked = false)
{
	Engine.LoadScript("gui/common/color.js");
	Engine.LoadScript("gui/common/functions_utility.js");
	Engine.LoadScript("gui/common/Observable.js");
	Engine.LoadScript("gui/common/settings.js");

	Engine.LoadScript("gui/maps/MapCache.js")

	Engine.LoadScript("gamesettings/");
	Engine.LoadScript("gamesettings/attributes/");

	if (networked)
		autostartInstance = new AutoStartHost(initData);
	else
		autostartInstance = new AutoStart(initData);
}

/**
 * @returns false if the loop should carry on.
 */
function onTick()
{
	return autostartInstance.onTick();
}
