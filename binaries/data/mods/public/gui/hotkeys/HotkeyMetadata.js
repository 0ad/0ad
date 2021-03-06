/**
 *
 */
class HotkeyMetadata
{
	constructor()
	{
		this.DEFAULT_CATEGORY = "other";
		this.categories = {};
		this.hotkeys = {};

		this.parseSpec();
	}

	parseSpec()
	{
		let files = this.getFiles();
		let hotkey_i = 0;
		let categories = {
			[this.DEFAULT_CATEGORY]: {
				"name": markForTranslation("Other Hotkeys"),
				"desc": markForTranslation("Other Hotkeys"),
			}
		};
		for (let file of files)
		{
			let data = Engine.ReadJSONFile(file);
			if (data.categories)
				for (let cat in data.categories)
					categories[cat] = data.categories[cat];
			if (data.hotkeys)
				for (let hotkey in data.hotkeys)
				{
					this.hotkeys[hotkey] = data.hotkeys[hotkey];
					this.hotkeys[hotkey].order = hotkey_i++;
					this.hotkeys[hotkey].categories = data.hotkeys[hotkey].categories || [this.DEFAULT_CATEGORY];
				}
			if (data.mapped_hotkeys)
				for (let cat in data.mapped_hotkeys)
					for (let hotkey in data.mapped_hotkeys[cat])
					{
						if (data.mapped_hotkeys[cat][hotkey].categories)
							warn("Categories will be overwritten for mapped hotkey " + hotkey);
						this.hotkeys[hotkey] = data.mapped_hotkeys[cat][hotkey];
						this.hotkeys[hotkey].order = hotkey_i++;
						this.hotkeys[hotkey].categories = [cat];
					}
		}
		// Sort categories (JS objects are (in this case) sorted by insertion order).
		this.categories = {};
		let keys = Object.keys(categories).sort((a, b) => {
			if (a === this.DEFAULT_CATEGORY || b === this.DEFAULT_CATEGORY)
				return a === this.DEFAULT_CATEGORY ? 1 : -1;
			if (categories[a].order === undefined || categories[b].order === undefined)
				return categories[a].order === undefined ? -1 : 1; // Likely to keep alphabetical order.
			return categories[a].order - categories[b].order;
		});
		for (let key of keys)
			this.categories[key] = categories[key];
		// TODO: validate that categories exist.
	}

	getFiles()
	{
		return Engine.ListDirectoryFiles("gui/hotkeys/spec/", "*.json");
	}
}
