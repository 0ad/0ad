function DeveloperOverlay()
{
	this.commandHeight = 20;
	this.displayState = false;
	this.timeWarp = false;
	this.changePerspective = false;
	this.controlAll = false;

	Engine.GetGUIObjectByName("devCommandsOverlay").onPress = this.toggle;
	this.layout();
}

DeveloperOverlay.prototype.getCommands = function() {
	return [
		{
			"label": translate("Control all units"),
			"onPress": checked => {
				Engine.PostNetworkCommand({
					"type": "control-all",
					"flag": checked
				});
			},
			"checked": () => this.controlAll,
		},
		{
			"label": translate("Change perspective"),
			"onPress": checked => {
				this.setChangePerspective(checked);
			},
		},
		{
			"label": translate("Display selection state"),
			"onPress": checked => {
				this.displayState = checked;
			},
		},
		{
			"label": translate("Pathfinder overlay"),
			"onPress": checked => {
				Engine.GuiInterfaceCall("SetPathfinderDebugOverlay", checked);
			},
		},
		{
			"label": translate("Obstruction overlay"),
			"onPress": checked => {
				Engine.GuiInterfaceCall("SetObstructionDebugOverlay", checked);
			},
		},
		{
			"label": translate("Unit motion overlay"),
			"onPress": checked => {
				g_Selection.SetMotionDebugOverlay(checked);
			},
		},
		{
			"label": translate("Range overlay"),
			"onPress": checked => {
				Engine.GuiInterfaceCall("SetRangeDebugOverlay", checked);
			},
		},
		{
			"label": translate("Bounding box overlay"),
			"onPress": checked => {
				Engine.SetBoundingBoxDebugOverlay(checked);
			},
		},
		{
			"label": translate("Restrict camera"),
			"onPress": checked => {
				Engine.GameView_SetConstrainCameraEnabled(checked);
			},
			"checked": () => Engine.GameView_GetConstrainCameraEnabled(),
		},
		{
			"label": translate("Reveal map"),
			"onPress": checked => {
				Engine.PostNetworkCommand({
					"type": "reveal-map",
					"enable": checked
				});
			},
			"checked": () => Engine.GuiInterfaceCall("IsMapRevealed"),
		},
		{
			"label": translate("Enable time warp"),
			"onPress": checked => {
				this.timeWarp = checked;
				if (checked)
					messageBox(
						500, 250,
						translate("Note: time warp mode is a developer option, and not intended for use over long periods of time. Using it incorrectly may cause the game to run out of memory or crash."),
						translate("Time warp mode"));
				Engine.EnableTimeWarpRecording(checked ? 10 : 0);
			},
		},
		{
			"label": translate("Promote selected units"),
			"onPress": checked => {
				Engine.PostNetworkCommand({
					"type": "promote",
					"entities": g_Selection.toList()
				});
			},
		},
		{
			"label": translate("Hierarchical pathfinder overlay"),
			"onPress": checked => {
				Engine.GuiInterfaceCall("SetPathfinderHierDebugOverlay", checked);
			},
		},
		{
			"label": translate("Enable culling"),
			"onPress": checked => {
				Engine.GameView_SetCullingEnabled(checked);
			},
			"checked": () => Engine.GameView_GetCullingEnabled(),
		},
		{
			"label": translate("Lock cull camera"),
			"onPress": checked => {
				Engine.GameView_SetLockCullCameraEnabled(checked);
			},
			"checked": () => Engine.GameView_GetLockCullCameraEnabled(),
		},
		{
			"label": translate("Display camera frustum"),
			"onPress": checked => {
				Engine.Renderer_SetDisplayFrustumEnabled(checked);
			},
			"checked": () => Engine.Renderer_GetDisplayFrustumEnabled(),
		},
	];
};

DeveloperOverlay.prototype.layout = function()
{
	for (let body of Engine.GetGUIObjectByName("dev_commands").children)
		body.hidden = true;

	let overlayHeight = 0;
	let commands = this.getCommands();
	for (let i = 0; i < commands.length; ++i)
	{
		let command = commands[i];

		let body = Engine.GetGUIObjectByName("dev_command[" + i + "]");
		let bodySize = body.size;
		bodySize.top = i * this.commandHeight;
		bodySize.bottom = bodySize.top + this.commandHeight;
		body.size = bodySize;
		body.hidden = false;

		let label = Engine.GetGUIObjectByName("dev_command_label[" + i + "]");
		label.caption = command.label;

		let checkbox = Engine.GetGUIObjectByName("dev_command_checkbox[" + i + "]");
		checkbox.onPress = function() {
			command.onPress(checkbox.checked);
			if (command.checked)
				checkbox.checked = command.checked();
		};

		overlayHeight += this.commandHeight;
	}

	let overlay = Engine.GetGUIObjectByName("devCommandsOverlay");
	let overlaySize = overlay.size;
	overlaySize.bottom = overlaySize.top + overlayHeight;
	overlay.size = overlaySize;

	this.updateValues();
};

DeveloperOverlay.prototype.updateValues = function()
{
	let commands = this.getCommands();
	for (let i = 0; i < commands.length; ++i)
	{
		let command = commands[i];

		let body = Engine.GetGUIObjectByName("dev_command[" + i + "]");
		if (body.hidden)
			continue;

		let checkbox = Engine.GetGUIObjectByName("dev_command_checkbox[" + i + "]");
		if (command.checked)
			checkbox.checked = command.checked();
	}
};

DeveloperOverlay.prototype.toggle = function()
{
	if (!g_GameAttributes.settings.CheatsEnabled && !g_IsReplay)
		return;

	let overlay = Engine.GetGUIObjectByName("devCommandsOverlay");
	overlay.hidden = !overlay.hidden;

	let message = overlay.hidden ?
		markForTranslation("The Developer Overlay was closed.") :
		markForTranslation("The Developer Overlay was opened.");

	// Only players can send the simulation chat command
	if (Engine.GetPlayerID() == -1)
		submitChatDirectly(message);
	else
		Engine.PostNetworkCommand({
			"type": "aichat",
			"message": message,
			"translateMessage": true,
			"translateParameters": [],
			"parameters": {}
		});
};

DeveloperOverlay.prototype.update = function()
{
	this.updateValues();

	let debug = Engine.GetGUIObjectByName("debugEntityState");

	if (!this.displayState)
	{
		debug.hidden = true;
		return;
	}

	debug.hidden = false;

	let conciseSimState = clone(GetSimState());
	conciseSimState.players = "<<<omitted>>>";
	let text = "simulation: " + uneval(conciseSimState);

	let selection = g_Selection.toList();
	if (selection.length)
	{
		let entState = GetEntityState(selection[0]);
		if (entState)
		{
			let template = GetTemplateData(entState.template);
			text += "\n\nentity: {\n";
			for (let k in entState)
				text += "  " + k + ":" + uneval(entState[k]) + "\n";
			text += "}\n\ntemplate: " + uneval(template);
		}
	}

	debug.caption = text.replace(/\[/g, "\\[");
};

DeveloperOverlay.prototype.isTimeWarpEnabled = function() {
	return this.timeWarp;
};

DeveloperOverlay.prototype.isChangePerspective = function() {
	return this.changePerspective;
};

DeveloperOverlay.prototype.setChangePerspective = function(value) {
	this.changePerspective = value;
	selectViewPlayer(g_ViewedPlayer);
};

DeveloperOverlay.prototype.isControlAll = function() {
	return this.controlAll;
};

DeveloperOverlay.prototype.setControlAll = function(value) {
	this.controlAll = value;
};
