/**
 * This class manages the developer overlay which displays the state of the first selected entity.
 */
class DeveloperOverlayEntityState
{
	constructor(selection)
	{
		this.developerOverlayEntityState = Engine.GetGUIObjectByName("developerOverlayEntityState");
		this.selection = selection;
		this.updater = this.update.bind(this);
	}

	setEnabled(enabled)
	{
		this.developerOverlayEntityState.hidden = !enabled;

		if (enabled)
		{
			registerSimulationUpdateHandler(this.updater);
			registerEntitySelectionChangeHandler(this.updater);
		}
		else
		{
			unregisterSimulationUpdateHandler(this.updater);
			unregisterEntitySelectionChangeHandler(this.updater);
		}
	}

	update()
	{
		let simState = clone(g_SimState);
		simState.players = "<<<omitted>>>";
		let text = "simulation: " + uneval(simState);

		let selection = this.selection.toList();
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

		this.developerOverlayEntityState.caption = text.replace(/\[/g, "\\[");
	}
}
