/**
 * This class architecture is an example of how to use classes
 * to encapsulate and to avoid fragmentation and globals.
 */
var g_SavegamePage;

function init(data)
{
	g_SavegamePage = new SavegamePage(data);
}

/**
 * This class is responsible for loading the affected GUI control classes,
 * and setting them up to communicate with each other.
 */
class SavegamePage
{
	constructor(data)
	{
		this.savegameList = new SavegameList();

		this.savegameDetails = new SavegameDetails();
		this.savegameList.registerSelectionChangeHandler(this.savegameDetails);

		this.savegameDeleter = new SavegameDeleter();
		this.savegameDeleter.registerSavegameListChangeHandler(this.savegameList);
		this.savegameList.registerSelectionChangeHandler(this.savegameDeleter);

		let savePage = Engine.IsGameStarted();
		if (savePage)
		{
			this.savegameWriter = new SavegameWriter(data && data.savedGameData || {});
			this.savegameList.registerSelectionChangeHandler(this.savegameWriter);
		}
		else
		{
			this.savegameLoader = new SavegameLoader();
			this.savegameList.registerSelectionChangeHandler(this.savegameLoader);
			this.savegameList.selectFirst();
		}

		Engine.GetGUIObjectByName("title").caption = savePage ? translate("Save Game") : translate("Load Game");
		Engine.GetGUIObjectByName("cancel").onPress = () => { Engine.PopGuiPage(); };
	}
}
