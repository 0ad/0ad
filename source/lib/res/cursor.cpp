#include "precompiled.h"

#ifdef _WIN32

// From win.cpp (so we don't need all the Windows headers in here)
void cursor_set(const char* name);

void cursor_draw(const char* name)
{
	cursor_set(name);
}

#else // #ifdef _WIN32

#include "lib/res/tex.h"
#include "lib/res/ogl_tex.h"
#include "lib/res/vfs.h"
#include "lib/ogl.h"

extern int mouse_x, mouse_y;

struct Cursor {
	Handle tex;
	int hotspotx, hotspoty;
	int w, h;
	void draw(int x, int y)
	{
		tex_bind(tex);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex2i( x-hotspotx,   y+hotspoty   );
			glTexCoord2i(1, 0); glVertex2i( x-hotspotx+w, y+hotspoty   );
			glTexCoord2i(1, 1); glVertex2i( x-hotspotx+w, y+hotspoty-h );
			glTexCoord2i(0, 1); glVertex2i( x-hotspotx,   y+hotspoty-h );
		glEnd();
	}
};

extern int g_yres;

// TODO: Remove duplication with Windows cursor code, handle errors, rewrite.
void cursor_draw(const char* name)
{
	static struct Cursor* last_cursor = NULL;
	static char* last_name = NULL;

	if (name == NULL)
	{
		// Clean up

		if (last_cursor)
		{
			tex_free(last_cursor->tex);
			delete last_cursor;
		}
		last_cursor = NULL;

		delete[] last_name;
		last_name = NULL;

		return;
	}

	// Don't do much if it's the same as last time
	if (last_name && !strcmp(name, last_name))
	{
		last_cursor->draw(mouse_x, g_yres-mouse_y);
		return;
	}

	// Store the name, so that the code can tell when it's
	// being called several times with the same cursor
	delete[] last_name;
	last_name = new char[strlen(name)+1];
	strcpy(last_name, name);

	char filename[VFS_MAX_PATH];

	// Load the .txt file containing the pixel offset of
	// the cursor's hotspot (the bit of it that's
	// drawn at (mouse_x,mouse_y) )
	sprintf(filename, "art/textures/cursors/%s.txt", name);

	int hotspotx, hotspoty;
	{
		void* data;
		size_t size;
		Handle h = vfs_load(filename, data, size);
		if (h <= 0)
		{
			// TODO: Handle errors
			assert(! "Error loading cursor hotspot .txt file");
			return;
		}
		if (sscanf((const char*)data, "%d %d", &hotspotx, &hotspoty) != 2)
		{
			// TODO: Handle errors
			assert(! "Invalid contents of cursor hotspot .txt file (should be like \"123 456\")");
			return;
		}

		// TODO: Unload the file
	}

	sprintf(filename, "art/textures/cursors/%s.png", name);

	Handle tex = tex_load(filename);
	if (tex <= 0)
	{
		// TODO: Handle errors
		assert(! "Error loading cursor texture");
		return;
	}

	int err = tex_upload(tex);
	if (err < 0)
	{
		// TODO: Handle errors
		assert(! "Error uploading cursor texture");
		return;
	}

	// Create the cursor object:

	int w, h;
	tex_info(tex, &w, &h, NULL, NULL, NULL);

	struct Cursor* cursor = new struct Cursor;
	cursor->tex = tex;
	cursor->hotspotx = hotspotx;
	cursor->hotspoty = hotspoty;
	cursor->w = w;
	cursor->h = h;

	if (last_cursor)
	{
		tex_free(last_cursor->tex);
		delete last_cursor;
	}
	cursor->draw(mouse_x, mouse_y);
	last_cursor = cursor;
}

#endif // #ifdef _WIN32 / #else