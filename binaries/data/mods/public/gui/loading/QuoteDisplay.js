/**
 * This class is concerned with chosing and displaying quotes from historical figures.
 */
class QuoteDisplay
{
	constructor()
	{
		Engine.GetGUIObjectByName("quoteText").caption =
			translate(pickRandom(
				Engine.ReadFileLines(this.File).filter(line => line)));
	}
}

QuoteDisplay.prototype.File = "gui/text/quotes.txt";
