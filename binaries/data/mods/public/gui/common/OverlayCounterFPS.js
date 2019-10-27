/**
 * This counter displays the current framerate in the screen corner.
 */
OverlayCounterTypes.prototype.FPS = class extends OverlayCounter
{
	constructor(overlayCounterManager)
	{
		super(overlayCounterManager);

		// Tiny performance improvement
		this.caption = translate(this.Caption);

		// Minimize object construction
		this.fpsObject = {};
	}

	/**
	 * This function is called frequently and thus minimized.
	 */
	get()
	{
		this.fpsObject.fps = Engine.GetFPS();
		return sprintf(this.caption, this.fpsObject);
	}
};

// dennis-ignore: *
OverlayCounterTypes.prototype.FPS.prototype.Caption = markForTranslation("FPS: %(fps)4s");

OverlayCounterTypes.prototype.FPS.prototype.Config = "overlay.fps";

OverlayCounterTypes.prototype.FPS.prototype.Hotkey = "fps.toggle";
