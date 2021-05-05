/**
 * Mimics CampaignRun's necessary interface, but for a broken run
 * (i.e. a run that can't be loaded), thus allowing to delete it.
 */
class BrokenRun
{
	constructor(file)
	{
		this.filename = file;
	}

	getLabel(forList)
	{
		if (!forList)
			return this.filename + ".0adcampaign";
		return coloredText(sprintf("%(filename)s (%(error)s)", {
			"filename": this.filename + ".0adcampaign",
			"error": translate("file cannot be loaded")
		}), "red");
	}

	destroy()
	{
		Engine.DeleteCampaignSave("saves/campaigns/" + this.filename + ".0adcampaign");
		if (CampaignRun.getCurrentRunFilename() === this.filename)
			CampaignRun.clearCurrentRun();
	}
}

/**
 * Lets you load/delete/look at existing campaign runs in your user folder.
 */
class LoadModal extends AutoWatcher
{
	constructor(campaignTemplate)
	{
		super("render");

		// _watch so render() is called anytime currentRuns are modified.
		this.currentRuns = _watch(this.getRuns(), () => this.render());

		Engine.GetGUIObjectByName('cancelButton').onPress = () => Engine.SwitchGuiPage("page_pregame.xml", {});
		Engine.GetGUIObjectByName('deleteGameButton').onPress = () => this.deleteSelectedRun();
		Engine.GetGUIObjectByName('startButton').onPress = () => this.startSelectedRun();

		this.noCampaignsText = Engine.GetGUIObjectByName("noCampaignsText");

		this.selectedRun = -1;
		this.runSelection = Engine.GetGUIObjectByName("runSelection");
		this.runSelection.onSelectionChange = () => {
			this.selectedRun = this.runSelection.selected;
			if (this.selectedRun === -1)
				Engine.GetGUIObjectByName('runDescription').caption = "";
			else
				Engine.GetGUIObjectByName('runDescription').caption = this.currentRuns[this.selectedRun].getLabel();
		};

		this.runSelection.onMouseLeftDoubleClickItem = () => this.startSelectedRun();

		this._ready = true;
	}

	getRuns()
	{
		let out = [];
		let files = Engine.ListDirectoryFiles("saves/campaigns/", "*.0adcampaign", false);
		for (let file of files)
		{
			let name = file.replace("saves/campaigns/", "").replace(".0adcampaign", "");
			try
			{
				out.push(new CampaignRun(name).load());
			}
			catch(err)
			{
				warn(err.toString());
				out.push(new BrokenRun(name));
			}
		}
		return out;
	}

	loadCampaign()
	{
		let filename = this.currentRuns[this.selectedRun].filename;
		let run = new CampaignRun(filename)
			.load()
			.setCurrent();

		Engine.SwitchGuiPage(run.getMenuPath(), {
			"filename": run.filename
		});
	}

	deleteSelectedRun()
	{
		if (this.selectedRun === -1)
			return;

		let run = this.currentRuns[this.selectedRun];

		messageBox(
			400, 200,
			sprintf(translate("Are you sure you want to delete run %s? This cannot be undone."), run.getLabel()),
			translate("Confirmation"),
			[translate("No"), translate("Yes")],
			[null, () => {
				run.destroy();
				this.currentRuns.splice(this.selectedRun, 1);
				this.selectedRun = -1;
			}]
		);
	}

	startSelectedRun()
	{
		if (this.currentRuns[this.selectedRun] instanceof CampaignRun)
			this.loadCampaign();
	}

	displayCurrentRuns()
	{
		this.runSelection.list = this.currentRuns.map(run => run.getLabel(true));
		this.runSelection.list_data = this.currentRuns.map(run => run.filename);
	}

	render()
	{
		this.noCampaignsText.hidden = !!this.currentRuns.length;
		Engine.GetGUIObjectByName('deleteGameButton').enabled = this.selectedRun !== -1;
		Engine.GetGUIObjectByName('startButton').enabled = this.selectedRun !== -1 && this.currentRuns[this.selectedRun] instanceof CampaignRun;
		this.displayCurrentRuns();
	}
}


var g_LoadModal;

function init()
{
	g_LoadModal = new LoadModal();
}
