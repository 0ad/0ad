class TechnologiesSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivTechs = Engine.GetGUIObjectByName("civTechs");
	}

	update(civCode)
	{
		let techs = this.getTechnologyCaptions(
			this.page.TemplateLister.getTemplateLists(civCode).techs.keys(),
			civCode
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
	count => translatePlural("Specific Technology", "Specific Technologies", count);
