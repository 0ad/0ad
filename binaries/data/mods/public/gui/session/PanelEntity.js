/**
 * This class sets up a shortcut to a specific entity in the GUI panel.
 * The button shows the portrait a tooltip with state information and a health and/or capture bar.
 * Clicking the button selects and jumps to to the entity.
 */
class PanelEntity
{
	constructor(selection, entityID, buttonID, orderKey)
	{
		this.selection = selection;
		this.hitpoints = undefined;
		this.capturePoints = undefined;

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
		this.panelEntityCaptureBar = Engine.GetGUIObjectByName("panelEntityCapture[" + buttonID + "]");
		this.panelEntButton = Engine.GetGUIObjectByName("panelEntityButton[" + buttonID + "]");
		this.panelEntButton.onPress = this.onPress.bind(this);
		this.panelEntButton.onDoublePress = this.onDoublePress.bind(this);
		this.panelEntButton.hidden = false;

		let entityState = GetEntityState(entityID);
		let template = GetTemplateData(entityState.template);
		this.nameTooltip = setStringTags(template.name.specific, this.NameTags) + "\n";

		Engine.GetGUIObjectByName("panelEntityHealthSection[" + buttonID + "]").hidden = !entityState.hitpoints;

		let captureSection = Engine.GetGUIObjectByName("panelEntityCaptureSection[" + buttonID + "]");
		captureSection.hidden = !entityState.capturePoints;
		if (entityState.capturePoints)
		{
			this.capturePoints = entityState.capturePoints;
			if (!entityState.hitpoints)
				captureSection.size = Engine.GetGUIObjectByName("panelEntitySectionPosTop[" + buttonID + "]").size;
		}

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
		this.updateHitpointsBar(entityState);
		this.updateCapturePointsBar(entityState);

		this.panelEntButton.tooltip =
			this.nameTooltip +
			this.Tooltips.map(tooltip => tooltip(entityState)).filter(tip => tip).join("\n");
	}

	updateHitpointsBar(entityState)
	{
		if (!entityState.hitpoints)
			return;

		if (this.hitpoints != entityState.hitpoints)
		{
			let size = this.panelEntityHealthBar.size;
			size.rright = 100 * entityState.hitpoints / entityState.maxHitpoints;
			this.panelEntityHealthBar.size = size;
		}
		if (entityState.hitpoints < this.hitpoints)
			this.onAttacked();
		this.hitpoints = entityState.hitpoints;
	}

	updateCapturePointsBar(entityState)
	{
		if (!entityState.capturePoints)
			return;

		let playerParts = this.panelEntityCaptureBar.children;
		let setCaptureBarPart = function(player, startSize) {
			let captureBar = playerParts[player];
			let size = captureBar.size;
			size.rleft = startSize;
			size.rright = startSize + 100 * Math.max(0, Math.min(1, entityState.capturePoints[player] / entityState.maxCapturePoints));
			captureBar.size = size;
			captureBar.sprite = "color:" + g_DiplomacyColors.getPlayerColor(player, 128);
			captureBar.hidden = false;
			return size.rright;
		};

		let size = setCaptureBarPart(entityState.player, 0);
		for (let i in entityState.capturePoints)
			if (i != entityState.player)
				size = setCaptureBarPart(i, size);

		if (entityState.capturePoints[entityState.player] < this.capturePoints[entityState.player])
			this.onAttacked();

		this.capturePoints = entityState.capturePoints;
	}

	onAttacked()
	{
		startColorFade(this.overlayName, 100, 0, colorFade_attackUnit, true, smoothColorFadeRestart_attackUnit);
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
	getCurrentCaptureTooltip,
	getAttackTooltip,
	getResistanceTooltip,
	getEntityTooltip,
	getAurasTooltip
];
