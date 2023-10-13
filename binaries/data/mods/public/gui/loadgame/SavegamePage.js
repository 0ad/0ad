/**
 * This class is responsible for loading the affected GUI control classes,
 * and setting them up to communicate with each other.
 */
class SavegamePage
{
	constructor(data)
	{
		this.savegameList = new SavegameList(data && data.campaignRun || null);

		this.savegameDetails = new SavegameDetails();
		this.savegameList.registerSelectionChangeHandler(this.savegameDetails);

		this.savegameDeleter = new SavegameDeleter();
		this.savegameDeleter.registerSavegameListChangeHandler(this.savegameList);
		this.savegameList.registerSelectionChangeHandler(this.savegameDeleter);

		const savePage = !!data?.savedGameData;
		if (savePage)
		{
			this.savegameWriter = new SavegameWriter(data.savedGameData);
			this.savegameList.registerSelectionChangeHandler(this.savegameWriter);
			let size = this.savegameList.gameSelection.size;
			size.bottom -= 24;
			this.savegameList.gameSelection.size = size;
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

var g_SavegamePage;

function init(data)
{
	g_SavegamePage = new SavegamePage(data);
}
