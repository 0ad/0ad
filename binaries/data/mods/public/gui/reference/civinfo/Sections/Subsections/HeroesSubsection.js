class HeroesSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivHeroes = Engine.GetGUIObjectByName("civHeroes");
	}

	update(civInfo)
	{
		let heroes = [];
		for (let faction of civInfo.Factions)
			Array.prototype.push.apply(
				heroes,
				faction.Heroes.map(hero => this.page.formatEntry(hero))
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
