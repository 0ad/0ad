GameSettings.prototype.Attributes.VictoryConditions = class VictoryConditions extends GameSetting
{
	constructor(settings)
	{
		super(settings);
		// Set of victory condition names.
		this.active = new Set();
		this.disabled = new Set();
		this.conditions = {};
	}

	init()
	{
		this.settings.map.watch(() => this.onMapChange(), ["map"]);

		let conditions = loadVictoryConditions();
		for (let cond of conditions)
			this.conditions[cond.Name] = cond;

		let defaults = [];
		for (let cond in this.conditions)
			if (this.conditions[cond].Default)
				defaults.push(this.conditions[cond].Name);
		this._add(this.active, this.disabled, defaults);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.VictoryConditions = Array.from(this.active);
	}

	fromInitAttributes(attribs)
	{
		const initAttribs = this.getLegacySetting(attribs, "VictoryConditions");
		if (initAttribs)
			this.fromList(initAttribs);
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		// If a map specifies victory conditions, replace them all.
		const conditions = this.getMapSetting("VictoryConditions");
		if (conditions)
			this.fromList(conditions);
	}

	fromList(conditionList)
	{
		conditionList = conditionList.filter(cond => cond in this.conditions);
		if (!conditionList)
			return;

		this._add(new Set(), new Set(), conditionList);
	}

	_reconstructDisabled(active)
	{
		let disabled = new Set();
		for (let cond of active)
			if (this.conditions[cond].DisabledWhenChecked)
				this.conditions[cond].DisabledWhenChecked.forEach(x => disabled.add(x));

		return disabled;
	}

	_add(currentActive, currentDisabled, names)
	{
		let active = clone(currentActive);
		for (const name of names) {
			if (currentDisabled.has(name))
				continue;
			active.add(name);
			// Assume we want to remove incompatible ones.
			if (this.conditions[name].DisabledWhenChecked)
				this.conditions[name].DisabledWhenChecked.forEach(x => active.delete(x));
		}
		// TODO: sanity check
		this.disabled = this._reconstructDisabled(active);
		this.active = active;
	}

	_delete(names)
	{
		let active = clone(this.active);
		for (const name of names)
			active.delete(name);
		// TODO: sanity check
		this.disabled = this._reconstructDisabled(active);
		this.active = active;
	}

	setEnabled(name, enabled)
	{
		if (enabled)
			this._add(this.active, this.disabled, [name]);
		else
			this._delete([name]);
	}
};
