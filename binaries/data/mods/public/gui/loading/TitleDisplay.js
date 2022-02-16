/**
 * This class choses the title of the loading screen page.
 */
class TitleDisplay
{
	constructor(data)
	{
		let loadingMapName = Engine.GetGUIObjectByName("loadingMapName");
		loadingMapName.caption = sprintf(
			data.attribs.mapType == "random" ? this.Generating : this.Loading,
			{ "map": translate(data.attribs.settings.mapName) });
	}
}

TitleDisplay.prototype.Generating = translate("Generating “%(map)s”");

TitleDisplay.prototype.Loading = translate("Loading “%(map)s”");
