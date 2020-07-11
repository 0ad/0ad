class TrainerSection
{
	constructor(page)
	{
		this.page = page;
		this.width = 0;
		this.widthChangedHandlers = new Set();

		this.TrainerSection = Engine.GetGUIObjectByName("trainerSection");
		this.Trainers = Engine.GetGUIObjectByName("trainers");

		this.TrainerSectionHeading = Engine.GetGUIObjectByName("trainerSectionHeading");
		this.TrainerSectionHeading.caption = this.Caption;

		this.trainerBoxes = [];
		for (let boxIdx in this.Trainers.children)
			this.trainerBoxes.push(new TrainerBox(this.page, boxIdx));
	}

	registerWidthChangedHandler(handler)
	{
		this.widthChangedHandlers.add(handler);
	}

	draw(units, civCode)
	{
		let caption = this.TrainerSectionHeading;
		this.width = Engine.GetTextWidth(caption.font, caption.caption) + (caption.size.left + caption.buffer_zone) * 2;
		let count = 0;

		for (let unitCode of units.keys())
		{
			let unitTemplate = this.page.TemplateParser.getEntity(unitCode, civCode);
			if (!unitTemplate.production.units.length && !unitTemplate.production.techs.length && !unitTemplate.upgrades)
				continue;

			if (count > this.trainerBoxes.length)
			{
				error("\"" + this.activeCiv + "\" has more unit trainers than can be supported by the current GUI layout");
				break;
			}

			this.width = Math.max(
				this.width,
				this.trainerBoxes[count].draw(unitCode, civCode)
			);

			++count;
		}
		hideRemaining(this.Trainers.name, count);

		// Update width and visibility of section
		let size = this.TrainerSection.size;
		this.width += EntityBox.prototype.HMargin;
		size.left = -this.width + size.right;
		this.TrainerSection.size = size;
		this.TrainerSection.hidden = count == 0;

		for (let handler of this.widthChangedHandlers)
			handler(this.width, !this.TrainerSection.hidden);
	}

	isVisible()
	{
		return !this.TrainerSection.hidden;
	}
}

TrainerSection.prototype.Caption =
	translate("Trainer Units");
