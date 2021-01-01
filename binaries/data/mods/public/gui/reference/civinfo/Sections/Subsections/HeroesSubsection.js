class HeroesSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivHeroes = Engine.GetGUIObjectByName("civHeroes");
	}

	update(civCode)
	{
		let heroes = this.getEntityCaptions(
			this.page.TemplateLister.getTemplateLists(civCode).units.keys(),
			this.IdentifyingClassList,
			civCode
		);

		heroes.unshift(
			this.page.formatHeading(
				this.HeadingCaption(heroes.length),
				this.page.SubsectionHeaderSize
			)
		);

		this.CivHeroes.caption = heroes.join("\n");
	}
}

HeroesSubsection.prototype.HeadingCaption =
	count => translatePlural("Hero", "Heroes", count);

HeroesSubsection.prototype.IdentifyingClassList =
	["Hero"];
