/**
 * This file is called from the visual & non-visual paths when autostarting.
 * To avoid relying on the GUI, this script has access to a special 'LoadScript' function.
 * See implementation in the public mod for more details.
 */

function autostartClient(initData)
{
	throw new Error("Autostart is not implemented in the 'mod' mod");
}

function autostartHost(initData, networked = false)
{
	throw new Error("Autostart is not implemented in the 'mod' mod");
}

/**
 * @returns false if the loop should carry on.
 */
function onTick()
{
	return true;
}
