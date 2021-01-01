class CivInfoPage extends ReferencePage
{
	constructor(data)
	{
		super();

		this.civSelection = new CivSelectDropdown(this.civData);
		this.civSelection.registerHandler(this.selectCiv.bind(this));

		this.gameplaySection = new GameplaySection(this);
		this.historySection = new HistorySection(this);

		let structreeButton = new StructreeButton(this);
		let closeButton = new CloseButton(this);
		Engine.SetGlobalHotkey("civinfo", "Press", this.closePage.bind(this));
	}

	switchToStructreePage()
	{
		Engine.PopGuiPage({ "civ": this.activeCiv, "nextPage": "page_structree.xml" });
	}

	closePage()
	{
		Engine.PopGuiPage({ "civ": this.activeCiv, "page": "page_civinfo.xml" });
	}

	/**
	 * Updates the GUI after the user selected a civ from dropdown.
	 *
	 * @param code {string}
	 */
	selectCiv(civCode)
	{
		this.setActiveCiv(civCode);

		let civInfo = this.civData[civCode];

		if(!civInfo)
			error(sprintf("Error loading civ data for \"%(code)s\"", { "code": civCode }));

		// Update civ gameplay display
		this.gameplaySection.update(this.activeCiv, civInfo);

		// Update civ history display
		this.historySection.update(civInfo);
	}

	/**
	 * Give the first character a larger font.
	 */
	bigFirstLetter(text, size)
	{
		return setStringTags(text[0], { "font": "sans-bold-" + (size + 6) }) + text.substring(1);
	}

	/**
	 * Set heading font - bold and mixed caps
	 *
	 * @param text {string}
	 * @param size {number} - Font size
	 * @returns {string}
	 */
	formatHeading(text, size)
	{
		let textArray = [];

		for (let word of text.split(" "))
		{
			let wordCaps = word.toUpperCase();

			// Usually we wish a big first letter, however this isn't always desirable. Check if
			// `.toLowerCase()` changes the character to avoid false positives from special characters.
			if (word.length && word[0].toLowerCase() != word[0])
				word = this.bigFirstLetter(wordCaps, size);

			textArray.push(setStringTags(word, { "font": "sans-bold-" + size }));
		}

		return textArray.join(" ");
	}

	/**
	 * @returns {string}
	 */
	formatEntry(name, tooltip, description)
	{
		let tooltip_icon = "";
		if (tooltip)
			tooltip_icon = '[icon="iconInfo" tooltip="' + escapeQuotation(tooltip) + '" tooltip_style="civInfoTooltip"]';

		let description_text = "";
		if (description)
			// Translation: Description of an item in the CivInfo page, on a new line and indented.
			description_text = sprintf(translate('\n     %(description)s'), { "description": description, });

		return sprintf(
			// Translation: An entry in the CivInfo Page. The newline and indentation of the description is handled elsewhere.
			// Example:
			// > • Name of a Special Something (i)
			// >     A brief description of the aforementioned something.
			translate("• %(name)s %(info_icon)s%(description)s"),
			{
				"name": setStringTags(name, { "font": "sans-bold-14" }),
				"info_icon": tooltip_icon,
				"description": description_text,
			}
		);
	}
}

CivInfoPage.prototype.CloseButtonTooltip =
	translate("%(hotkey)s: Close Civilization Overview.");

CivInfoPage.prototype.SectionHeaderSize = 16;
CivInfoPage.prototype.SubsectionHeaderSize = 12;
