class TechnologiesSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivTechs = Engine.GetGUIObjectByName("civTechs");
	}

	update(civInfo)
	{
		let techs = [];
		for (let faction of civInfo.Factions)
			Array.prototype.push.apply(
				techs,
				faction.Technologies.map(tech => this.page.formatEntry(tech))
			);

		techs.unshift(
			this.page.formatHeading(
				this.HeadingCaption(techs.length),
				this.page.SubsectionHeaderSize
			)
		);

		this.CivTechs.caption = techs.join("\n");
	}
}

TechnologiesSubsection.prototype.HeadingCaption =
	count => translatePlural("Special Technology", "Special Technologies", count);
