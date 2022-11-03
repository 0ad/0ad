/**
 * Handles the logic related to the 'default formation' feature.
 * When given a walking order, units that aren't in formation will be put
 * in the default formation, to improve pathfinding and reactivity.
 * However, when given other tasks (such as e.g. gather), they will be removed
 * from any formation they are in, as those orders don't work very well with formations.
 *
 * Set the default formation to the null formation to disable this entirely.
 *
 * TODO: it would be nice to let players choose default formations for different orders,
 * but that would be neater if orders where defined somewhere unique,
 * instead of mostly in unit_actions.js
 */
class AutoFormation
{
	constructor()
	{
		this.defaultFormation = Engine.ConfigDB_GetValue("user", "gui.session.defaultformation");
		if (!this.defaultFormation)
			this.setDefault(NULL_FORMATION);
		this.lastDefault = this.defaultFormation === NULL_FORMATION ?
			"special/formations/box" : this.defaultFormation;
		Engine.SetGlobalHotkey("session.toggledefaultformation", "Press", () => {
			if (this.defaultFormation === NULL_FORMATION)
				this.setDefault(this.lastDefault);
			else
				this.setDefault(NULL_FORMATION);
		});
	}

	/**
	 * Set the default formation to @param formation.
	 * TODO: would be good to validate, particularly since some formations aren't
	 * usable with any arbitrary unit type, we may want to warn then.
	 */
	setDefault(formation)
	{
		this.defaultFormation = formation;
		if (formation !== NULL_FORMATION)
			this.lastDefault = this.defaultFormation;
		Engine.ConfigDB_ConfigDB_CreateAndSaveValue("user", "gui.session.defaultformation", this.defaultFormation);
		return true;
	}

	isDefault(formation)
	{
		return formation == this.defaultFormation;
	}

	/**
	 * @return the default formation, or "undefined" if the null formation was chosen,
	 * otherwise units in formation would disband on any order, which isn't desirable.
	 */
	getDefault()
	{
		return this.defaultFormation == NULL_FORMATION ? undefined : this.defaultFormation;
	}

	/**
	 * @return the null formation, or "undefined", depending on "walkOnly" preference.
	 */
	getNull()
	{
		let walkOnly = Engine.ConfigDB_GetValue("user", "gui.session.formationwalkonly") === "true";
		return walkOnly ? NULL_FORMATION : undefined;
	}
}
