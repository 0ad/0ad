/**
 * This is an abstract class instantiated per defined VictoryCondition.
 */
class VictoryConditionCheckbox extends GameSettingControlCheckbox
{
	constructor(victoryCondition, ...args)
	{
		super(...args);

		this.victoryCondition = victoryCondition.Name;
		this.setTitle(victoryCondition.Title);
		this.setTooltip(victoryCondition.Description);

		g_GameSettings.victoryConditions.watch(() => this.render(), ["active"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario" &&
			!g_GameSettings.victoryConditions.disabled.has(this.victoryCondition));
		this.setChecked(g_GameSettings.victoryConditions.active.has(this.victoryCondition));
	}

	onPress(checked)
	{
		g_GameSettings.victoryConditions.setEnabled(this.victoryCondition, checked);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
}
