/**
 * This class stores the checkboxes that are part of the developer overlay.
 * These checkboxes may own their own helper class instances, such as the EntityState or TimeWarp feature.
 */
class DeveloperOverlay
{
	constructor(playerViewControl, selection)
	{
		this.devCommandsOverlay = Engine.GetGUIObjectByName("devCommandsOverlay");
		this.devCommandsOverlay.onPress = this.toggle.bind(this);

		this.checkBoxes = this.getCheckboxNames().map((name, i) =>
			new DeveloperOverlayControlCheckbox(
				new DeveloperOverlayControlCheckboxes.prototype[name](playerViewControl, selection),
				i));
		this.dropDowns = this.getDropDownNames().map((name, i) =>
			new DeveloperOverlayControlDropDown(
				new DeveloperOverlayControlDrowDowns.prototype[name](playerViewControl, selection),
				i + this.checkBoxes.length));

		this.resize();
	}

	/**
	 * Mods may overwrite this to change the order.
	 */
	getCheckboxNames()
	{
		return Object.keys(DeveloperOverlayControlCheckboxes.prototype);
	}

	getDropDownNames()
	{
		return Object.keys(DeveloperOverlayControlDrowDowns.prototype);
	}

	toggle()
	{
		if (g_IsNetworked && !g_InitAttributes.settings.CheatsEnabled)
			return;

		this.devCommandsOverlay.hidden = !this.devCommandsOverlay.hidden;
		this.sendNotification();
		this.checkBoxes.forEach(checkbox => {
			checkbox.setHidden(this.devCommandsOverlay.hidden);
		});
		this.dropDowns.forEach(dropDown => {
			dropDown.setHidden(this.devCommandsOverlay.hidden);
		});
	}

	sendNotification()
	{
		let message = this.devCommandsOverlay.hidden ? this.CloseNotification : this.OpenNotification;

		// Only players can send the simulation chat command
		if (Engine.GetPlayerID() == -1)
			g_Chat.submitChat(message);
		else
			Engine.PostNetworkCommand({
				"type": "aichat",
				"message": message,
				"translateMessage": true,
				"translateParameters": [],
				"parameters": {}
			});
	}

	resize()
	{
		let size = this.devCommandsOverlay.size;
		size.bottom =
			size.top +
			this.checkBoxes.reduce((height, checkbox) => height + checkbox.getHeight(), 0) +
			this.dropDowns.reduce((height, dropDown) => height + dropDown.getHeight(), 0);
		this.devCommandsOverlay.size = size;
	}
}

DeveloperOverlay.prototype.OpenNotification = markForTranslation("The Developer Overlay was opened.");
DeveloperOverlay.prototype.CloseNotification = markForTranslation("The Developer Overlay was closed.");
