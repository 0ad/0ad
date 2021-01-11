class StructuresSubsection extends Subsection
{
	constructor(page)
	{
		super(page);
		this.CivStructures = Engine.GetGUIObjectByName("civStructures");
	}

	update(civCode)
	{
		let structures = this.getEntityCaptions(
			this.page.TemplateLister.getTemplateLists(civCode).structures.keys(),
			this.IdentifyingClassList,
			civCode
		);

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
	count => translatePlural("Specific Structure", "Specific Structures", count);

StructuresSubsection.prototype.IdentifyingClassList =
	["CivSpecific Structure"];
