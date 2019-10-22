/**
 * This class sets up a shortcut to a specific entity in the GUI panel.
 * The button shows the portrait a tooltip with state information and a health bar.
 * Clicking the button selects and jumps to to the entity.
 */
class PanelEntity
{
	constructor(selection, entityID, buttonID, orderKey)
	{
		this.selection = selection;
		this.hitpoints = undefined;

		/**
		 * Public property
		 */
		this.entityID = entityID;

		/**
		 * Public property
		 */
		this.orderKey = orderKey;

		this.overlayName = "panelEntityHitOverlay[" + buttonID + "]";
		this.panelEntityHealthBar = Engine.GetGUIObjectByName("panelEntityHealthBar[" + buttonID + "]");
		this.panelEntButton = Engine.GetGUIObjectByName("panelEntityButton[" + buttonID + "]");
		this.panelEntButton.onPress = this.onPress.bind(this);
		this.panelEntButton.onDoublePress = this.onDoublePress.bind(this);
		this.panelEntButton.hidden = false;

		let entityState = GetEntityState(entityID);
		let template = GetTemplateData(entityState.template);
		this.nameTooltip = setStringTags(template.name.specific, this.NameTags) + "\n";

		Engine.GetGUIObjectByName("panelEntityImage[" + buttonID + "]").sprite =
			"stretched:" + this.PortraitDirectory + template.icon;
	}

	destroy()
	{
		this.panelEntButton.hidden = true;
		stopColorFade(this.overlayName);
	}

	update(i, reposition)
	{
		// TODO: Instead of instant position changes, animate button movement.
		if (reposition)
			setPanelObjectPosition(this.panelEntButton, i, Infinity);

		let entityState = GetEntityState(this.entityID);

		if (this.hitpoints != entityState.hitpoints)
		{
			let size = this.panelEntityHealthBar.size;
			size.rright = 100 * entityState.hitpoints / entityState.maxHitpoints;
			this.panelEntityHealthBar.size = size;
		}

		this.panelEntButton.tooltip =
			this.nameTooltip +
			this.Tooltips.map(tooltip => tooltip(entityState)).filter(tip => tip).join("\n");

		if (this.hitpoints > entityState.hitpoints)
			startColorFade(this.overlayName, 100, 0, colorFade_attackUnit, true, smoothColorFadeRestart_attackUnit);
		this.hitpoints = entityState.hitpoints;
	}

	onPress()
	{
		if (!Engine.HotkeyIsPressed("selection.add"))
			this.selection.reset();

		this.selection.addList([this.entityID]);
	}

	onDoublePress()
	{
		this.selection.selectAndMoveTo(getEntityOrHolder(this.entityID));
	}
}

PanelEntity.prototype.NameTags = { "font": "sans-bold-16" };

PanelEntity.prototype.PortraitDirectory = "session/portraits/";

PanelEntity.prototype.Tooltips = [
	getCurrentHealthTooltip,
	getAttackTooltip,
	getArmorTooltip,
	getEntityTooltip,
	getAurasTooltip
];
