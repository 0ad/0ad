class CivSelectDropdown
{
	constructor(civData)
	{
		this.handlers = new Set();

		let civList = Object.keys(civData).map(civ => ({
			"name": civData[civ].Name,
			"code": civ,
		})).sort(sortNameIgnoreCase);

		this.civSelectionHeading = Engine.GetGUIObjectByName("civSelectionHeading");
		this.civSelectionHeading.caption = this.Caption;

		this.civSelection = Engine.GetGUIObjectByName("civSelection");
		this.civSelection.list = civList.map(c => c.name);
		this.civSelection.list_data = civList.map(c => c.code);
		this.civSelection.onSelectionChange = () => this.onSelectionChange(this);
	}

	onSelectionChange()
	{
		let civCode = this.civSelection.list_data[this.civSelection.selected];

		for (let handler of this.handlers)
			handler(civCode);
	}

	registerHandler(handler)
	{
		this.handlers.add(handler);
	}

	unregisterHandler(handler)
	{
		this.handlers.delete(handler);
	}

	hasCivs()
	{
		return this.civSelection.list.length != 0;
	}

	selectCiv(civCode)
	{
		if (!civCode)
			return;

		let index = this.civSelection.list_data.indexOf(civCode);
		if (index == -1)
			return;

		this.civSelection.selected = index;
	}

	selectFirstCiv()
	{
		this.civSelection.selected = 0;
	}
}

CivSelectDropdown.prototype.Caption =
	translate("Civilization:");
