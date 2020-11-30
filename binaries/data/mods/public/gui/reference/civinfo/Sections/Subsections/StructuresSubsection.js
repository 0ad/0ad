class StructuresSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivStructures = Engine.GetGUIObjectByName("civStructures");
	}

	update(civInfo)
	{
		let structures = civInfo.Structures.map(bonus => this.page.formatEntry(bonus));
		structures.unshift(
			this.page.formatHeading(
				this.HeadingCaption(structures.length),
				this.page.SubsectionHeaderSize
			)
		);

		this.CivStructures.caption = structures.join("\n");
	}
}

StructuresSubsection.prototype.HeadingCaption =
	count => translatePlural("Special Structure", "Special Structures", count);
