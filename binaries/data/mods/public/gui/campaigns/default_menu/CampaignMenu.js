/**
 * This is the main menu screen of the campaign.
 * It shows you the currently available scenarios, scenarios you've already completed, etc.
 * This particular variant is extremely simple and shows a list similar to Age 1's campaigns,
 * but conceptually nothing really prevents more complex systems.
 */
class CampaignMenu extends AutoWatcher
{
	constructor(campaignRun)
	{
		super("render");

		this.run = campaignRun;

		this.selectedLevel = -1;
		this.levelSelection = Engine.GetGUIObjectByName("levelSelection");
		this.levelSelection.onSelectionChange = () => { this.selectedLevel = this.levelSelection.selected; };

		this.levelSelection.onMouseLeftDoubleClickItem = () => this.startScenario();
		Engine.GetGUIObjectByName('startButton').onPress = () => this.startScenario();
		Engine.GetGUIObjectByName('backToMain').onPress = () => this.goBackToMainMenu();
		Engine.GetGUIObjectByName('savedGamesButton').onPress = () => Engine.PushGuiPage('page_loadgame.xml', {
			'campaignRun': this.run.filename
		});

		this.mapCache = new MapCache();

		this._ready = true;
	}

	goBackToMainMenu()
	{
		this.run.save();
		Engine.SwitchGuiPage("page_pregame.xml", {});
	}

	startScenario()
	{
		let level = this.getSelectedLevelData();
		if (!meetsRequirements(this.run, level))
			return;
		Engine.SwitchGuiPage("page_gamesetup.xml", {
			"mapType": level.MapType,
			"map": "maps/" + level.Map,
			"autostart": true,
			"campaignData": {
				"run": this.run.filename,
				"levelID": this.levelSelection.list_data[this.selectedLevel],
				"data": this.run.data
			}
		});
	}

	getSelectedLevelData()
	{
		if (this.selectedLevel === -1)
			return undefined;
		return this.run.template.Levels[this.levelSelection.list_data[this.selectedLevel]];
	}

	shouldShowLevel(levelData)
	{
		if (this.run.template.ShowUnavailable)
			return true;

		return meetsRequirements(this.run, levelData);
	}

	getLevelName(levelData)
	{
		if (levelData.Name)
			return translateWithContext("Campaign Template", levelData.Name);
		return translate(this.mapCache.getTranslatableMapName(levelData.MapType, "maps/" + levelData.Map));
	}

	getLevelDescription(levelData)
	{
		if (levelData.Description)
			return translateWithContext("Campaign Template", levelData.Description);
		return this.mapCache.getTranslatedMapDescription(levelData.MapType, "maps/" + levelData.Map);

	}

	displayLevelsList()
	{
		let list = [];
		for (let key in this.run.template.Levels)
		{
			let level = this.run.template.Levels[key];

			if (!this.shouldShowLevel(level))
				continue;

			let status = "";
			let name = this.getLevelName(level);
			if (isCompleted(this.run, key))
				status = translateWithContext("campaign status", "Completed");
			else if (meetsRequirements(this.run, level))
				status = coloredText(translateWithContext("campaign status", "Available"), "green");
			else
				name = coloredText(name, "gray");

			list.push({ "ID": key, "name": name, "status": status });
		}

		list.sort((a, b) => this.run.template.Order.indexOf(a.ID) - this.run.template.Order.indexOf(b.ID));

		list = prepareForDropdown(list);

		this.levelSelection.list_name = list.name || [];
		this.levelSelection.list_status = list.status || [];

		// COList needs these changed last or crashes.
		this.levelSelection.list = list.ID || [];
		this.levelSelection.list_data = list.ID || [];
	}

	displayLevelDetails()
	{
		if (this.selectedLevel === -1)
		{
			Engine.GetGUIObjectByName("startButton").enabled = false;
			Engine.GetGUIObjectByName("startButton").hidden = false;
			return;
		}

		let level = this.getSelectedLevelData();

		Engine.GetGUIObjectByName("scenarioName").caption = this.getLevelName(level);
		Engine.GetGUIObjectByName("scenarioDesc").caption = this.getLevelDescription(level);
		if (level.Preview)
			Engine.GetGUIObjectByName('levelPreviewBox').sprite = "cropped:" + 400/512 + "," + 300/512 + ":" + level.Preview;
		else
			Engine.GetGUIObjectByName('levelPreviewBox').sprite = "cropped:" + 400/512 + "," + 300/512 + ":session/icons/mappreview/nopreview.png";

		Engine.GetGUIObjectByName("startButton").enabled = meetsRequirements(this.run, level);
		Engine.GetGUIObjectByName("startButton").hidden = false;
		Engine.GetGUIObjectByName("loadSavedButton").hidden = true;
	}

	render()
	{
		Engine.GetGUIObjectByName("campaignTitle").caption = this.run.getLabel();
		this.displayLevelDetails();
		this.displayLevelsList();
	}
}


var g_CampaignMenu;

function init(initData)
{
	let run;
	try {
		run = new CampaignRun(initData.filename).load();
	} catch (err) {
		error(sprintf(translate("Error loading campaign run %s: %s."), initData.filename, err));
		Engine.SwitchGuiPage("page_pregame.xml", {});
	}
	g_CampaignMenu = new CampaignMenu(run);
}
