/**
 * This is an abstract class instantiated per defined VictoryCondition.
 */
class VictoryConditionCheckbox extends GameSettingControlCheckbox
{
	constructor(victoryCondition, ...args)
	{
		super(...args);

		this.victoryCondition = victoryCondition;

		this.setTitle(this.victoryCondition.Title);
		this.setTooltip(this.victoryCondition.Description);
	}

	onMapChange(mapData)
	{
		let mapIndex =
			mapData &&
			mapData.settings &&
			mapData.settings.VictoryConditions &&
			mapData.settings.VictoryConditions.indexOf(this.victoryCondition.Name);

		if (mapIndex === undefined)
			return;

		let index =
			g_GameAttributes.settings &&
			g_GameAttributes.settings.VictoryConditions &&
			g_GameAttributes.settings.VictoryConditions.indexOf(this.victoryCondition.Name);

		if (index !== undefined && (mapIndex == -1) == (index == -1))
			return;

		if (!g_GameAttributes.settings.VictoryConditions)
			g_GameAttributes.settings.VictoryConditions = [];

		if (mapIndex == -1)
		{
			if (index !== undefined)
				g_GameAttributes.settings.VictoryConditions.splice(index, 1);
		}
		else
			g_GameAttributes.settings.VictoryConditions.push(this.victoryCondition.Name);

		this.gameSettingsControl.updateGameAttributes();
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.settings.VictoryConditions)
		{
			g_GameAttributes.settings.VictoryConditions = [];
			for (let victoryCondition of g_VictoryConditions)
				if (victoryCondition.Default)
					g_GameAttributes.settings.VictoryConditions.push(victoryCondition.Name);
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setEnabled(
			g_GameAttributes.mapType != "scenario" &&
			(!this.victoryCondition.DisabledWhenChecked ||
			this.victoryCondition.DisabledWhenChecked.every(name =>
				g_GameAttributes.settings.VictoryConditions.indexOf(name) == -1)));

		this.setChecked(g_GameAttributes.settings.VictoryConditions.indexOf(this.victoryCondition.Name) != -1);
	}

	onPress(checked)
	{
		let victoryCondition = new Set(g_GameAttributes.settings.VictoryConditions);

		if (checked)
		{
			victoryCondition.add(this.victoryCondition.Name);

			if (this.victoryCondition.ChangeOnChecked)
				for (let name in this.victoryCondition.ChangeOnChecked)
					if (this.victoryCondition.ChangeOnChecked[name])
						victoryCondition.add(name);
					else
						victoryCondition.delete(name);
		}
		else
			victoryCondition.delete(this.victoryCondition.Name);

		g_GameAttributes.settings.VictoryConditions = Array.from(victoryCondition);
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onGameAttributesFinalize()
	{
		if (!g_GameAttributes.settings.TriggerScripts)
			g_GameAttributes.settings.TriggerScripts = [];

		if (g_GameAttributes.settings.VictoryConditions.indexOf(this.victoryCondition.Name) != -1)
			for (let script of this.victoryCondition.Scripts)
				if (g_GameAttributes.settings.TriggerScripts.indexOf(script) == -1)
					g_GameAttributes.settings.TriggerScripts.push(script);
	}
}
