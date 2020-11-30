class BonusesSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivBonuses = Engine.GetGUIObjectByName("civBonuses");
	}

	update(civInfo)
	{
		let civBonuses = civInfo.CivBonuses.map(bonus => this.page.formatEntry(bonus));
		civBonuses.unshift(
			this.page.formatHeading(
				this.HeadingCivBonusCaption(civBonuses.length),
				this.page.SubsectionHeaderSize
			)
		);

		let teamBonuses = civInfo.TeamBonuses.map(bonus => this.page.formatEntry(bonus));
		teamBonuses.unshift(
			this.page.formatHeading(
				this.HeadingTeamBonusCaption(teamBonuses.length),
				this.page.SubsectionHeaderSize
			)
		);

		this.CivBonuses.caption = civBonuses.join("\n") + "\n\n" + teamBonuses.join("\n");
	}
}

BonusesSubsection.prototype.HeadingCivBonusCaption =
	count => translatePlural("Civilization Bonus", "Civilization Bonuses", count);

BonusesSubsection.prototype.HeadingTeamBonusCaption =
	count => translatePlural("Team Bonus", "Team Bonuses", count);
