/**
 * This class is concerned with chosing and displaying hints about how to play the game.
 * This includes a text and an image.
 */
class TipDisplay
{
	constructor()
	{
		this.tipImage = Engine.GetGUIObjectByName("tipImage");
		this.tipTitle = Engine.GetGUIObjectByName("tipTitle");
		this.tipText = Engine.GetGUIObjectByName("tipText");

		this.tipFiles = listFiles(this.TextPath, ".txt", false);
		this.displayRandomTip();
	}

	displayRandomTip()
	{
		let tipFile = pickRandom(this.tipFiles);
		if (tipFile)
			this.displayTip(tipFile);
		else
			error("Failed to find any matching tips for the loading screen.");
	}

	displayTip(tipFile)
	{
		this.tipImage.sprite =
			"stretched:" + this.ImagePath + tipFile + ".png";

		let tipText = Engine.TranslateLines(Engine.ReadFile(
			this.TextPath + tipFile + ".txt")).split("\n");

		this.tipTitle.caption = tipText.shift();

		this.tipText.caption = tipText.map(text =>
			text && sprintf(this.BulletFormat, { "tiptext": text })).join("\n\n");
	}
}

/**
 * Directory storing txt files containing the gameplay tips.
 */
TipDisplay.prototype.TextPath = "gui/text/tips/";

/**
 * Directory storing the PNG images with filenames corresponding to the tip text files.
 */
TipDisplay.prototype.ImagePath = "loading/tips/";

// Translation: A bullet point used before every item of list of tips displayed on loading screen
TipDisplay.prototype.BulletFormat = translate("• %(tiptext)s");
