var g_hasCallback = false;
var g_hasChanges = false;
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
	],
	"soundSetting":
	[
		[translate("Master Gain"), translate("Master audio gain"), {"config":"sound.mastergain", "function":"Engine.SetMasterGain(Number(this.caption));"}, "number"],
		[translate("Music Gain"), translate("In game music gain"), {"config":"sound.musicgain", "function":"Engine.SetMusicGain(Number(this.caption));"}, "number"],
		[translate("Ambient Gain"), translate("In game ambient sound gain"), {"config":"sound.ambientgain", "function":"Engine.SetMusicGain(Number(this.caption));"}, "number"],
		[translate("Action Gain"), translate("In game unit action sound gain"), {"config":"sound.actiongain", "function":"Engine.SetMusicGain(Number(this.caption));"}, "number"],
		[translate("UI Gain"), translate("UI sound gain"), {"config":"sound.uigain", "function":"Engine.SetMusicGain(Number(this.caption));"}, "number"],
	],
	"lobbySetting":
	[
		[translate("Chat Backlog"), translate("Number of backlogged messages to load when joining the lobby"), {"config":"lobby.history"}, "number"],
		[translate("Chat Timestamp"), translate("Show time that messages are posted in the lobby chat"), {"config":"lobby.chattimestamp"}, "boolean"],
	],
};

function init(data)
{
	if (data && data.callback)
		g_hasCallback = true;
	let reload = data && data.reload;

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
	var onPress = function(){ g_hasChanges = true; };

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
		// Different option action load and save differently, so this switch is needed.
		for (let action of Object.keys(option[2]))
		{
			switch (action)
			{
			case "config":
				// Load initial value if not yet loaded.
				if (!checked || typeof checked != "boolean")
					checked = Engine.ConfigDB_GetValue("user", option[2][action]) === "true";
				// Hacky macro to create the callback.
				var callback = function(key)
				{
					return function()
						Engine.ConfigDB_CreateValue("user", key, String(this.checked));
				}(option[2][action]);
				// Merge the new callback with any existing callbacks.
				onPress = mergeFunctions(callback, onPress);
				break;
			case "renderer":
				// If reloading, config values have priority, otherwise load initial value if not yet loaded
				if (reload && option[2].config)
				{
					checked = Engine.ConfigDB_GetValue("user", option[2].config) === "true";
					let rendererChecked = eval("Engine.Renderer_Get" + option[2].renderer + "Enabled()");
					if (rendererChecked != checked)
						eval("Engine.Renderer_Set" + option[2].renderer + "Enabled(" + checked + ")");
				}
				else if (!checked || typeof checked != "boolean")
					checked = eval("Engine.Renderer_Get" + option[2][action] + "Enabled()");
				// Hacky macro to create the callback (updating also the config value if any).
				var callback = function(key, keyConfig)
				{
					return function()
					{
						eval("Engine.Renderer_Set" + key + "Enabled(" + this.checked + ")");
						if (keyConfig)
							Engine.ConfigDB_CreateValue("user", keyConfig, String(this.checked));
					}
				}(option[2][action], option[2].config);
				// Merge the new callback with any existing callbacks.
				onPress = mergeFunctions(callback, onPress);
				break;
			case "function":
				// This allows for doing low-level actions, like hiding/showing UI elements.
				onPress = mergeFunctions(eval("function(){" + option[2][action] + "}"), onPress);
				break;
			default:
				warn("Unknown option source type '" + action + "'");
			}
		}
		// Load final data to the control element.
		control.checked = checked;
		control.onPress = onPress;
		break;
	case "number":
		// TODO: Slider
	case "string":
		control = Engine.GetGUIObjectByName(prefix + "Input[" + i + "]");
		var caption;
		for (let action of Object.keys(option[2]))
		{
			switch (action)
			{
			case "config":
				onPress = function(){};
				caption = Engine.ConfigDB_GetValue("user", option[2][action]);
				// Hacky macro to create the callback.
				var callback = function(key)
				{
					return function()
					{
						if (Engine.ConfigDB_GetValue("user", key) == this.caption)
							return;
						Engine.ConfigDB_CreateValue("user", key, String(this.caption));
						g_hasChanges = true;
					}
				}(option[2][action]);
				// Merge the new callback with any existing callbacks.
				onPress = mergeFunctions(callback, onPress);
				break;
			case "function":
				// This allows for doing low-level actions, like hiding/showing UI elements.
				onPress = mergeFunctions(function(){eval(option[2][action])}, onPress);
				break;
			default:
				warn("Unknown option source type '" + action + "'");
			}
		}
		control.caption = caption;
		control.onPress = onPress;
		control.onMouseLeave = onPress;
		break;
	default:
		warn("Unknown option type '" + options[3] + "', assuming string. Valid types are 'number', 'string', or 'bool'.");
		control = Engine.GetGUIObjectByName(prefix + "Input[" + i + "]");
		break;
	}
	control.hidden = false;
	control.tooltip = option[1];
	return control;
}

/**
 * Merge two functions which don't expect arguments.
 *
 * @return Merged function.
 */
function mergeFunctions(function1, function2)
{
	return function()
	{
		function1.apply(this);
		function2.apply(this);
	};
}

function reloadDefaults()
{
	Engine.ConfigDB_Reload("user");
	init({ "reload": true });
	g_hasChanges = false;
}

function saveDefaults()
{
	g_hasChanges = false;
	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
}

/**
 * Close GUI page and call callbacks if they exist.
 **/
function closePage()
{
	if (g_hasChanges)
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
