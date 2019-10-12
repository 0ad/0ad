/**
 * This class updates the ceasefire counter in the diplomacy dialog.
 */
class DiplomacyDialogCeasefireCounter
{
	constructor()
	{
		this.diplomacyCeasefireCounter = Engine.GetGUIObjectByName("diplomacyCeasefireCounter");
	}

	update()
	{
		let active = GetSimState().ceasefireActive;

		this.diplomacyCeasefireCounter.hidden = !active;
		if (!active)
			return;

		this.diplomacyCeasefireCounter.caption =
			sprintf(translateWithContext("ceasefire", this.Caption), {
				"time": timeToString(GetSimState().ceasefireTimeRemaining)
			});
	}
}

DiplomacyDialogCeasefireCounter.prototype.Caption =
	markForTranslationWithContext("ceasefire", "Remaining ceasefire time: %(time)s.");
