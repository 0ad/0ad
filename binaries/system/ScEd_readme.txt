0AD Terrain Editor
-------------------

Notes:
------
- Load/Save facility is available for scenarios, but there's no guarantee that scenarios generated in the current version will be loadable in subsequent versions
- Texture browser is under Map->Texture Tools
	- current texture for painting in top box
	- click texture in list to use it for painting
- Heightmap textures (used via "Map -> Load Terrain" must be TGAs; the heightmap is always generated from the red component of the texture
- Other textures must be a power of 2 in both width and height, but don't necessarily need to be square)
- Getting around: 
	- mouse wheel - zoom in/out
	- click and drag on minimap
	- RTS style scroll navigation
- Screenshot hotkey on F9, for anyone that wants : these are saved as consecutive numbered TGAs in a "screenshots" subdirectory of the directory containing ScEd.exe 
	- counter resets to 1 each time the editor is restarted, so copy TGAs somewhere else to preserve them
	- probably more use later on when submitting graphics bugs


Elevation Tools
-----------------
1. Raise/Lower: use left mouse to raise, and right mouse to lower terrain.  "Brush Effect" indicates scale of alteration
2. Smooth: use left mouse to smooth.  "Brush Effect" indicates "scale" of smoothing.  


Unit Tools
-----------------
To create a new object to add to the scenario:
1. Go to Map->Unit Tools
2. Click Add
3. Enter the name of the new unit
4. On the next page, enter the name of the 0adm file, and the name of texture to use on the model.  The "..." button will bring up
a file browser to search for valid files
5. Click "Update" to rebuild the model data
6. Click "<<<" to return to the scenario editor

Notes:
	- if you re-export an SMD file, it is possible to update existing models in the scene using that file:
		- Go to Tools->SMD Conversion; and convert the file, overwriting the existing filename
		- Go to Map->Unit Tools->Add, then click "Update" on the Object Properties Page.  Click "<<<" and everything should
		be redrawn with the new model.
	- bad texture files, or bad model data aren't yet handled correctly - you'll either get a crash, or no model shown
	- there's no trackball yet - the view in the Object Editor is fixed at the minute; if the model is offset from the origin, it's possible
	you won't see it

To apply models to the terrain, just highlight the object name in the list, and left click somewhere in the world view.

	
Known Issues
--------------
	- crash on certain video cards using VBOs; eg Radeons in laptops
	- minimap not always updating when painting textures/altering elevation
	- textures loaded by the engine are locked once opened 
		- ie it's not possible to resave textures from (eg) Photoshop are reload them
		 	- either have to restart editor, or save to different name; appreciate this isn't much fun, will be fixed for next release, hopefully.	 		
	- incorrect normals/colour interpolation along terrain "creases"