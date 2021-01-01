class GameplaySection
{
	constructor(page)
	{
		this.page = page;
		this.CivGameplayHeading = Engine.GetGUIObjectByName('civGameplayHeading');

		this.BonusesSubsection = new BonusesSubsection(this.page);
		this.HeroesSubsection = new HeroesSubsection(this.page);
		this.StructuresSubsection = new StructuresSubsection(this.page);
		this.TechnologiesSubsection = new TechnologiesSubsection(this.page);
	}

	update(civCode, civInfo)
	{
		this.CivGameplayHeading.caption =
			this.page.formatHeading(
				sprintf(this.headingCaption, { "civilization": civInfo.Name }),
				this.page.SectionHeaderSize
			);

		this.BonusesSubsection.update(civCode, civInfo);
		this.TechnologiesSubsection.update(civCode);
		this.StructuresSubsection.update(civCode);
		this.HeroesSubsection.update(civCode);
	}
}

GameplaySection.prototype.headingCaption =
	translate("%(civilization)s Gameplay");
