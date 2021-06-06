class BonusesSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivBonuses = Engine.GetGUIObjectByName("civBonuses");
	}

	update(civCode, civInfo)
	{
		const player = this.page.TemplateParser.getPlayer(civCode);

		// Civilization bonuses can be represented by a single auto-researched technology...
		let civBonuses = this.getTechnologyCaptions(
			this.page.TemplateLoader.autoResearchTechList,
			civCode
		);
		// ...or as a globally applied aura...
		Array.prototype.push.apply(civBonuses, this.getAuraCaptions(player.civbonuses));
		// ...however some fit into neither of the two above categories (e.g. Athenian "Silver
		// Owls", Roman "Testudo Formation"). Thus we also display descriptions of civ bonuses
		// as written in the {civ}.json files.
		for (let bonus of civInfo.CivBonuses)
			civBonuses.push(this.page.formatEntry(
				bonus.Name,
				bonus.History || false,
				bonus.Description || false
			));
		civBonuses.unshift(
			this.page.formatHeading(
				this.HeadingCivBonusCaption(civBonuses.length),
				this.page.SubsectionHeaderSize
			)
		);

		let teamBonuses = this.getAuraCaptions(
			player.teambonuses,
			civCode
		);
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
