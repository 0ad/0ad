/**
 * The MultiPainter applies several painters to the given area.
 */
function MultiPainter(painters)
{
	if (painters instanceof Array)
		this.painters = painters;
	else if (!painters)
		this.painters = [];
	else
		this.painters = [painters];
}

MultiPainter.prototype.paint = function(area)
{
	for (let painter of this.painters)
		painter.paint(area);
};
