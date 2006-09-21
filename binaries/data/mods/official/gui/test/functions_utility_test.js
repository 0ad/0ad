function updateOrbital()
{
	if( !getGUIObjectByName( 'arena' ).hidden )
	{
		g_ballx += g_balldx;
		g_bally += g_balldy;
		if (g_ballx > 600)
		{
			g_balldx *= -0.9;
			g_ballx = 600-(g_ballx-600);
		}
		else if (g_ballx < 0)
		{
			g_balldx *= -0.9;
			g_ballx = -g_ballx;
		}
		if (g_bally > 400)
		{
			g_balldy *= -0.9;
			g_bally = 400-(g_bally-400);
		}
		else if (g_bally < 0)
		{
			g_balldy *= -0.9;
			g_bally = -g_bally;
		}

		// Gravitate towards the mouse
		var vect_x = g_ballx-g_mousex;
		var vect_y = g_bally-g_mousey;
		var dsquared = vect_x*vect_x + vect_y*vect_y;
		if (dsquared < 1000) dsquared = 1000;
		var force = 10000.0 / dsquared;
		var mag = Math.sqrt(dsquared);
		vect_x /= mag; vect_y /= mag;
		g_balldx -= force * vect_x;
		g_balldy -= force * vect_y;

		var ball = getGUIObjectByName('ball');
		var r=5;
		ball.size = new GUISize(g_ballx-r, g_bally-r, g_ballx+r, g_bally+r);
	}
}

