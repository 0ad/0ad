/**
 * This class is concerned with playing an ambient track, birds chirping etc.
 */
class Ambient
{
	constructor()
	{
		Engine.PlayAmbientSound(pickRandom(this.Tracks), true);
	}
}

/**
 * TODO: Let the map decide the tracks.
 */
Ambient.prototype.Tracks = [
	"audio/ambient/dayscape/day_temperate_gen_03.ogg"
];
