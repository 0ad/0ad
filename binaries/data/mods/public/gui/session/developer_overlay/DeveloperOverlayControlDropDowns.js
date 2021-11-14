/**
 * This class stores the handlers for the individual dropdowns available in the developer overlay.
 * Such a class must have onSelectionChange function.
 * If the class has a selected property, then that will be called every simulation update to
 * synchronize the state of the dropdown (only if the developer overaly is opened).
 */
class DeveloperOverlayControlDrowDowns
{
}

DeveloperOverlayControlDrowDowns.prototype.RenderDebugMode = class
{
	constructor()
	{
		this.selectedIndex = this.values().map(e => e.value).indexOf(
			Engine.Renderer_GetRenderDebugMode());
	}

	values()
	{
		return [
			{ "value": "RENDER_DEBUG_MODE_NONE", "label": translate("Render Debug Mode Disabled") },
			{ "value": "RENDER_DEBUG_MODE_AO", "label": translate("Render Debug Mode AO") },
			{ "value": "RENDER_DEBUG_MODE_ALPHA", "label": translate("Render Debug Mode Alpha") },
			{ "value": "RENDER_DEBUG_MODE_CUSTOM", "label": translate("Render Debug Mode Custom") }
		];
	}

	onSelectionChange(selectedIndex)
	{
		this.selectedIndex = selectedIndex;
		Engine.Renderer_SetRenderDebugMode(this.values()[this.selectedIndex].value);
	}

	selected()
	{
		return this.selectedIndex;
	}
};
