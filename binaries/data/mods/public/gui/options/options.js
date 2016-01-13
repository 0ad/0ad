var g_hasCallback = false;
var g_controls;
/**
 * This array holds the data to populate the general section with.
 * Data is in the form [Title, Tooltip, {ActionType:Action}, InputType].
 */
var options = {
	"generalSetting":
	[
		[translate("Windowed Mode"), translate("Start 0 A.D. in a window"), {"config":"windowed"}, "boolean"],
		[translate("Background Pause"), translate("Pause single player games when window loses focus"), {"config":"pauseonfocusloss"}, "boolean"],
		[translate("Disable Welcome Screen"), translate("If you disable this screen completely, you may miss important announcements.\nYou can still launch it using the main menu."), {"config":"splashscreendisable"}, "boolean"],
		[translate("Detailed Tooltips"), translate("Show detailed tooltips for trainable units in unit-producing buildings."), {"config":"showdetailedtooltips"}, "boolean"],
		[translate("FPS Overlay"), translate("Show frames per second in top right corner."), {"config":"overlay.fps"}, "boolean"],
		[translate("Realtime Overlay"), translate("Show current system time in top right corner."), {"config":"overlay.realtime"}, "boolean"],
		[translate("Gametime Overlay"), translate("Show current simulation time in top right corner."), {"config":"gui.session.timeelapsedcounter"}, "boolean"],
		[translate("Ceasefire Time Overlay"), translate("Always show the remaining ceasefire time."), {"config":"gui.session.ceasefirecounter"}, "boolean"],
		[translate("Persist Match Settings"), translate("Save and restore match settings for quick reuse when hosting another game"), {"config":"persistmatchsettings"}, "boolean"],
	],
	"graphicsSetting":
	[
		[translate("Prefer GLSL"), translate("Use OpenGL 2.0 shaders (recommended)"), {"renderer":"PreferGLSL", "config":"preferglsl"}, "boolean"],
		[translate("Post Processing"), translate("Use screen-space postprocessing filters (HDR, Bloom, DOF, etc)"), {"renderer":"Postproc", "config":"postproc"}, "boolean"],
		[translate("Shadows"), translate("Enable shadows"), {"renderer":"Shadows", "config":"shadows"}, "boolean"],
		[translate("Particles"), translate("Enable particles"), {"renderer":"Particles", "config":"particles"}, "boolean"],
		[translate("Show Sky"), translate("Render Sky"), {"renderer":"ShowSky", "config":"showsky"}, "boolean"],
		[translate("Smooth LOS"), translate("Lift darkness and fog-of-war smoothly"), {"renderer":"SmoothLOS", "config":"smoothlos"}, "boolean"],
		[translate("Unit Silhouettes"), translate("Show outlines of units behind buildings"), {"renderer":"Silhouettes", "config":"silhouettes"}, "boolean"],
		[translate("Shadow Filtering"), translate("Smooth shadows"), {"renderer":"ShadowPCF", "config":"shadowpcf"}, "boolean"],
		[translate("Fast & Ugly Water"), translate("Use the lowest settings possible to render water. This makes other settings irrelevant."), {"renderer":"WaterUgly", "config":"waterugly"}, "boolean"],
		[translate("HQ Water Effects"), translate("Use higher-quality effects for water, rendering coastal waves, shore foam, and ships trails."), {"renderer":"WaterFancyEffects", "config":"waterfancyeffects"}, "boolean"],
		[translate("Real Water Depth"), translate("Use actual water depth in rendering calculations"), {"renderer":"WaterRealDepth", "config":"waterrealdepth"}, "boolean"],
		[translate("Water Reflections"), translate("Allow water to reflect a mirror image"), {"renderer":"WaterReflection", "config":"waterreflection"}, "boolean"],
		[translate("Water Refraction"), translate("Use a real water refraction map and not transparency"), {"renderer":"WaterRefraction", "config":"waterrefraction"}, "boolean"],
		[translate("Shadows on Water"), translate("Cast shadows on water"), {"renderer":"WaterShadows", "config":"watershadows"}, "boolean"],
		[translate("VSync"), translate("Run vertical sync to fix screen tearing. REQUIRES GAME RESTART"), {"config":"vsync"}, "boolean"],
		[translate("Limit FPS in Menus"), translate("Limit FPS to 50 in all menus, to save power."), {"config":"gui.menu.limitfps"}, "boolean"],
		[translate("Graphics quality"), translate("Graphics quality. REQUIRES GAME RESTART"), {"config":"materialmgr.quality", "min": "0", "max": "10"}, "number"],
	],
	"soundSetting":
	[
		[translate("Master Gain"), translate("Master audio gain"), {"config":"sound.mastergain", "function":"Engine.SetMasterGain(Number(this.caption));", "min": "0"}, "number"],
		[translate("Music Gain"), translate("In game music gain"), {"config":"sound.musicgain", "function":"Engine.SetMusicGain(Number(this.caption));", "min": "0"}, "number"],
		[translate("Ambient Gain"), translate("In game ambient sound gain"), {"config":"sound.ambientgain", "function":"Engine.SetAmbientGain(Number(this.caption));", "min": "0"}, "number"],
		[translate("Action Gain"), translate("In game unit action sound gain"), {"config":"sound.actiongain", "function":"Engine.SetActionGain(Number(this.caption));", "min": "0"}, "number"],
		[translate("UI Gain"), translate("UI sound gain"), {"config":"sound.uigain", "function":"Engine.SetUIGain(Number(this.caption));", "min": "0"}, "number"],
	],
	"lobbySetting":
	[
		[translate("Chat Backlog"), translate("Number of backlogged messages to load when joining the lobby"), {"config":"lobby.history", "min": "0"}, "number"],
		[translate("Chat Timestamp"), translate("Show time that messages are posted in the lobby chat"), {"config":"lobby.chattimestamp"}, "boolean"],
	],
};

function init(data)
{
	if (data && data.callback)
		g_hasCallback = true;
	let reload = data && data.reload;
	g_controls = [];

	// WARNING: We assume a strict formatting of the XML and do minimal checking.
	for (let prefix of Object.keys(options))
	{
		let lastSize;
		for (let i = 0; i < options[prefix].length; i++)
		{
			let body = Engine.GetGUIObjectByName(prefix + "[" + i + "]");
			let label = Engine.GetGUIObjectByName(prefix + "Label[" + i + "]");
			// Setup control.
			setupControl(options[prefix][i], i, prefix, reload);
			// Setup label.
			label.caption = options[prefix][i][0];
			label.tooltip = options[prefix][i][1];
			// Move each element to the correct place.
			if (i > 0)
			{
				let newSize = new GUISize();
				newSize.left = lastSize.left;
				newSize.rright = lastSize.rright;
				newSize.top = lastSize.bottom;
				newSize.bottom = newSize.top + 25;
				body.size = newSize;
				lastSize = newSize;
			}
			else
				lastSize = body.size;
			// Show element.
			body.hidden = false;
		}
	}

	if (!reload)
		updateStatus();
}

/**
 * Setup the apropriate control for a given option.
 *
 * @param option Structure containing the data to setup an option.
 * @param prefix Prefix to use when accessing control, for example "generalSetting" when the tickbox name is generalSettingTickbox[i].
 */
function setupControl(option, i, prefix, reload)
{
	var control;
	var onPress;

	switch (option[3])
	{
	case "boolean":
		// More space for the label
		let text = Engine.GetGUIObjectByName(prefix + "Label[" + i + "]");
		let size = text.size;
		size.rright = 87;
		text.size = size;
		control = Engine.GetGUIObjectByName(prefix + "Tickbox[" + i + "]");
		var checked;
		var keyRenderer;
		var keyConfig;
		var functionBody;

		for (let action of Object.keys(option[2]))
		{
			switch (action)
			{
			case "config":
				keyConfig = option[2].config;
				if (checked === undefined || reload)
					checked = Engine.ConfigDB_GetValue("user", keyConfig) === "true";
				else if ((Engine.ConfigDB_GetValue("user", keyConfig) === "true") !== checked)
					Engine.ConfigDB_CreateValue("user", keyConfig, String(checked));
				break;
			case "renderer":
				keyRenderer = option[2].renderer;
				if (checked === undefined)
					checked = eval("Engine.Renderer_Get" + keyRenderer + "Enabled()");
				else if (eval("Engine.Renderer_Get" + keyRenderer + "Enabled()") !== checked)
					eval("Engine.Renderer_Set" + keyRenderer + "Enabled(" + checked + ")");
				break;
			case "function":
				// This allows for doing low-level actions, like hiding/showing UI elements.
				functionBody = option[2].function;
				break;
			default:
				warn("Unknown option source type '" + action + "'");
			}
		}

		onPress = function(keyRenderer, keyConfig, functionBody)
		{
			return function()
			{
				if (keyRenderer)
					eval("Engine.Renderer_Set" + keyRenderer + "Enabled(" + this.checked + ")");
				if (keyConfig)
					Engine.ConfigDB_CreateValue("user", keyConfig, String(this.checked));
				if (functionBody)
					eval(functionBody);
				updateStatus(true);
			};
		}(keyRenderer, keyConfig, functionBody);

		// Load final data to the control element.
		control.checked = checked;
		control.onPress = onPress;
		break;
	case "number":
		// TODO: Slider
	case "string":
		control = Engine.GetGUIObjectByName(prefix + "Input[" + i + "]");
		var caption;
		var key;
		var functionBody;
		var minval;
		var maxval;

		for (let action of Object.keys(option[2]))
		{
			switch (action)
			{
			case "config":
				key = option[2].config;
				caption = Engine.ConfigDB_GetValue("user", key);
				break;
			case "function":
				// This allows for doing low-level actions, like hiding/showing UI elements.
				functionBody = option[2].function;
				break;
			case "min":
				minval = option[2].min;
				break;
			case "max":
				maxval = option[2].max;
				break;
			default:
				warn("Unknown option source type '" + action + "'");
			}
		}

		// as the enter key is not necessarily pressed after modifying an entry, we will register the input also
		// - when the mouse leave the control (MouseLeave event)
		// - or when saving or closing the window (registerChanges function)
		// so we must ensure that something has indeed been modified
		onPress = function(key, functionBody, minval, maxval)
		{
			return function()
			{
				if (minval && +minval > +this.caption)
					this.caption = minval;
				if (maxval && +maxval < +this.caption)
					this.caption = maxval;
				if (Engine.ConfigDB_GetValue("user", key) === this.caption)
					return;
				Engine.ConfigDB_CreateValue("user", key, this.caption);
				if (functionBody)
					eval(functionBody);
				updateStatus(true);
			};
		}(key, functionBody, minval, maxval);

		control.caption = caption;
		control.onPress = onPress;
		control.onMouseLeave = onPress;
		g_controls.push(control);
		break;
	default:
		warn("Unknown option type '" + option[3] + "', assuming string. Valid types are 'number', 'string', or 'bool'.");
		control = Engine.GetGUIObjectByName(prefix + "Input[" + i + "]");
		break;
	}
	control.hidden = false;
	control.tooltip = option[1];
	return control;
}

function updateStatus(val)
{
	if (typeof val == "boolean")
		Engine.ConfigDB_CreateValue("user", "nosave.haschanges", String(val));
	else
		val = Engine.ConfigDB_GetValue("user", "nosave.haschanges") === "true";

	Engine.GetGUIObjectByName("loadOptions").enabled = val;
	Engine.GetGUIObjectByName("saveOptions").enabled = val;
}

/**
 * Register changes of input (text and number) controls
 */
function registerChanges()
{
	for (let control of g_controls)
		control.onPress();
}

function reloadDefaults()
{
	Engine.ConfigDB_Reload("user");
	updateStatus(false);
	init({ "reload": true });
}

function saveDefaults()
{
	registerChanges();
	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
	updateStatus(false);
}

/**
 * Close GUI page and call callbacks if they exist.
 **/
function closePage()
{
	registerChanges();
	if (Engine.ConfigDB_GetValue("user", "nosave.haschanges") === "true")
	{
		let btCaptions = [translate("No"), translate("Yes")];
		let btCode = [null, function(){ closePageWithoutConfirmation(); }];
		messageBox(500, 200, translate("You have unsaved changes, are you sure you want to quit ?"),
			translate("Warning"), 0, btCaptions, btCode);
	}
	else
		closePageWithoutConfirmation();
}

function closePageWithoutConfirmation()
{
	if (g_hasCallback)
		Engine.PopGuiPageCB();
	else
		Engine.PopGuiPage();
}
