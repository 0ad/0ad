/**
 * This class creates and handles a tribute button for the given player for every tributable resource.
 */
DiplomacyDialogPlayerControl.prototype.TributeButtonManager = class
{
	constructor(playerID)
	{
		let resCodes = g_ResourceData.GetTributableCodes();
		let buttonCount = Engine.GetGUIObjectByName("diplomacyPlayer[0]_tribute").children.length;

		Engine.GetGUIObjectByName("diplomacyHeaderTribute").hidden = !resCodes.length;

		if (resCodes.length > buttonCount)
			warn("There are " + resCodes.length + " tributable resources, but only " + buttonCount + " buttons!");

		this.buttons = [];

		for (let i = 0; i < Math.min(resCodes.length, buttonCount); ++i)
			this.buttons[i] = new this.TributeButton(playerID, resCodes[i], i);
	}

	update(playerInactive)
	{
		for (let button of this.buttons)
			button.update(playerInactive);
	}
};

DiplomacyDialogPlayerControl.prototype.TributeButtonManager.getWidthOffset = function()
{
	let tributeButtonSize = Engine.GetGUIObjectByName("diplomacyPlayer[0]_tribute[0]").size;
	return g_ResourceData.GetTributableCodes().length * (tributeButtonSize.right - tributeButtonSize.left);
};

/**
 * This class manages one tribute button for one tributable resource for one receiving player.
 * Players may tribute mass amounts of resources by clicking multiple times on the button while holding a hotkey.
 */
DiplomacyDialogPlayerControl.prototype.TributeButtonManager.prototype.TributeButton = class
{
	constructor(playerID, resCode, resIndex)
	{
		this.playerID = playerID;
		this.resCode = resCode;
		this.amount = undefined;

		let name = "diplomacyPlayer[" + (playerID - 1) + "]_tribute[" + resIndex + "]";

		this.button = Engine.GetGUIObjectByName(name);
		this.button.onPress = this.onPress.bind(this);
		setPanelObjectPosition(this.button, resIndex, resIndex + 1, 0);

		Engine.GetGUIObjectByName(name + "_hotkey").onRelease = this.onMassTributeRelease.bind(this);
		Engine.GetGUIObjectByName(name + "_image").sprite = "stretched:" + this.ResourceIconPath + resCode + ".png";

		this.setAmount(this.DefaultAmount);
	}

	update(playerInactive)
	{
		this.button.hidden = playerInactive;

		if (!this.button.hidden)
			this.button.enabled = controlsPlayer(g_ViewedPlayer);
	}

	onPress()
	{
		if (Engine.HotkeyIsPressed("session.masstribute"))
			this.setAmount(this.nextAmount());
		else
			this.performTribute();
	}

	onMassTributeRelease()
	{
		if (this.amount >= this.MassAmount)
			this.performTribute();
	}

	setAmount(amount)
	{
		this.amount = amount;
		this.button.tooltip = sprintf(
			translate(this.Tooltip), {
				"resourceAmount": this.amount,
				"greaterAmount": this.nextAmount(),
				"resourceType": resourceNameWithinSentence(this.resCode),
				"playerName": colorizePlayernameByID(this.playerID),
			});
	}

	nextAmount()
	{
		return this.MassAmount * (Math.floor(this.amount / this.MassAmount) + 1);
	}

	performTribute()
	{
		Engine.PostNetworkCommand({
			"type": "tribute",
			"player": this.playerID,
			"amounts": {
				[this.resCode]: this.amount
			}
		});
		this.setAmount(this.DefaultAmount);
	}
};

DiplomacyDialogPlayerControl.prototype.TributeButtonManager.prototype.TributeButton.prototype.Tooltip =
	markForTranslation("Tribute %(resourceAmount)s %(resourceType)s to %(playerName)s. Shift-click to tribute %(greaterAmount)s.");

DiplomacyDialogPlayerControl.prototype.TributeButtonManager.prototype.TributeButton.prototype.ResourceIconPath =
	"session/icons/resources/";

DiplomacyDialogPlayerControl.prototype.TributeButtonManager.prototype.TributeButton.prototype.DefaultAmount = 100;

DiplomacyDialogPlayerControl.prototype.TributeButtonManager.prototype.TributeButton.prototype.MassAmount = 500;
