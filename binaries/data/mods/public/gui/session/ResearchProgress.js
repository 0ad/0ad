/**
 * This class is responsible for displaying the currently researched technologies in an overlay.
 */
class ResearchProgress
{
	constructor(playerViewControl, selection)
	{
		this.buttons = Engine.GetGUIObjectByName("researchStartedButtons").children;
		this.buttonHandlers = this.buttons.map((button, i) => new ResearchProgressButton(selection, i));

		/**
		 * Top coordinate of the research list.
		 * Changes depending on the number of displayed counters.
		 */
		this.topOffset = g_OverlayCounterManager.lastHeight;

		let updater = this.updateResearchProgress.bind(this);
		registerSimulationUpdateHandler(updater);
		playerViewControl.registerViewedPlayerChangeHandler(updater);
		g_OverlayCounterManager.registerResizeHandler(this.setTopOffset.bind(this));
	}

	setTopOffset(offset)
	{
		this.topOffset = offset;
	}

	updateResearchProgress()
	{
		let researchStarted = Engine.GuiInterfaceCall("GetStartedResearch", g_ViewedPlayer);

		let i = 0;
		for (let techName in researchStarted)
		{
			if (i == this.buttons.length)
				break;

			this.buttonHandlers[i++].onResearchedProgress(this.topOffset, techName, researchStarted[techName]);
		}

		while (i < this.buttons.length)
			this.buttons[i++].hidden = true;
	}
}

/**
 * This is an individual button displaying a tech currently researched by the currently viewed player.
 */
class ResearchProgressButton
{
	constructor(selection, i)
	{
		this.selection = selection;
		this.button = Engine.GetGUIObjectByName("researchStartedButton[" + i + "]");
		this.sprite = Engine.GetGUIObjectByName("researchStartedIcon[" + i + "]");
		this.progress = Engine.GetGUIObjectByName("researchStartedProgressSlider[" + i + "]");
		this.timeRemaining = Engine.GetGUIObjectByName("researchStartedTimeRemaining[" + i + "]");
		this.paused = Engine.GetGUIObjectByName("researchPausedIcon[" + i + "]");

		this.buttonHeight = this.button.size.bottom - this.button.size.top;
		this.buttonTop = this.Margin + (this.Margin + this.buttonHeight) * i;
		this.progressHeight = this.progress.size.bottom - this.progress.size.top;
		this.progressTop = this.progress.size.top;
		this.button.onPress = this.onPress.bind(this);
	}

	onResearchedProgress(offset, techName, researchStatus)
	{
		this.researcher = researchStatus.researcher;

		let template = GetTechnologyData(techName, g_Players[g_ViewedPlayer].civ);
		let modifier = "stretched:";
		if (researchStatus.paused)
			modifier += "color:0 0 0 127:grayscale:";
		this.sprite.sprite = modifier + this.PortraitDirectory + template.icon;

		let size = this.button.size;
		size.top = offset + this.buttonTop;
		size.bottom = size.top + this.buttonHeight;
		this.button.size = size;
		this.button.tooltip = getEntityNames(template);
		if (researchStatus.paused)
			this.button.tooltip += "\n" + translate(this.PausedResearchString);
		this.button.hidden = false;

		size = this.progress.size;
		size.top = this.progressTop + this.progressHeight * researchStatus.progress;
		this.progress.size = size;

		this.timeRemaining.caption =
			Engine.FormatMillisecondsIntoDateStringGMT(
				researchStatus.timeRemaining,
				translateWithContext("countdown format", this.CountdownFormat));

		this.paused.hidden = !researchStatus.paused;
	}

	onPress()
	{
		this.selection.selectAndMoveTo(this.researcher);
	}
}

/**
 * Distance between consecutive buttons.
 */
ResearchProgressButton.prototype.Margin = 4;

/**
 * Directory containing all icons.
 */
ResearchProgressButton.prototype.PortraitDirectory = "session/portraits/";

/**
 * This format is used when displaying the remaining time of the currently viewed techs in research.
 */
ResearchProgressButton.prototype.CountdownFormat = markForTranslationWithContext("countdown format", "m:ss");

// Translation: String displayed when the research is paused. E.g. by being garrisoned or when not the first item in the queue.
ResearchProgressButton.prototype.PausedResearchString = markForTranslation("This item is paused.");
