#include "precompiled.h"

#include "XMLFix.h"

#include "CommonConvert.h"

#include "FUtils/FUXmlParser.h"

/*

Things that are fixed here:

----

3ds Max "file://" image URLs

Identifier: /COLLADA/asset/contributor/authoring_tool = "FBX COLLADA exporter"

Problem: /COLLADA/library_images/image/init_from = "file://" which crashes some versions of FCollada

Fix: Delete the whole library_images subtree, since we never use it anyway.
Then delete library_effects and library_materials too, to avoid broken references.

----

3ds Max broken material references

Identifier: /COLLADA/asset/contributor/authoring_tool = "FBX COLLADA exporter"

Problem: /COLLADA/library_visual_scenes/.../instance_material/@target sometimes
refers to non-existent material IDs.

Fix: Delete the whole bind_material subtree, since we never use it anyway.

----

*/

static xmlNode* findChildElement(xmlNode* node, const char* name)
{
	for (xmlNode* child = node->children; child; child = child->next)
	{
		if (child->type == XML_ELEMENT_NODE && strcmp((const char*)child->name, name) == 0)
			return child;
	}
	return NULL;
}

static bool applyFBXFixesNode(xmlNode* node)
{
	bool changed = false;
	for (xmlNode* child = node->children; child; child = child->next)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			if (strcmp((const char*)child->name, "node") == 0)
			{
				if (applyFBXFixesNode(child))
					changed = true;
			}
			else if (strcmp((const char*)child->name, "instance_geometry") == 0)
			{
				xmlNode* bind_material = findChildElement(child, "bind_material");
				if (! bind_material) continue;
				Log(LOG_INFO, "Found a bind_material to delete");
				xmlUnlinkNode(bind_material);
				xmlFreeNode(bind_material);

				changed = true;
			}
		}
	}
	return changed;
}

static bool applyFBXFixes(xmlNode* root)
{
	Log(LOG_INFO, "Applying fixes for 3ds Max exporter");

	bool changed = false;

	xmlNode* library_images = findChildElement(root, "library_images");
	if (library_images)
	{
		Log(LOG_INFO, "Found library_images to delete");
		xmlUnlinkNode(library_images);
		xmlFreeNode(library_images);
		changed = true;
	}

	xmlNode* library_materials = findChildElement(root, "library_materials");
	if (library_materials)
	{
		Log(LOG_INFO, "Found library_materials to delete");
		xmlUnlinkNode(library_materials);
		xmlFreeNode(library_materials);
		changed = true;
	}

	xmlNode* library_effects = findChildElement(root, "library_effects");
	if (library_effects)
	{
		Log(LOG_INFO, "Found library_effects to delete");
		xmlUnlinkNode(library_effects);
		xmlFreeNode(library_effects);
		changed = true;
	}

	xmlNode* library_visual_scenes = findChildElement(root, "library_visual_scenes");
	if (library_visual_scenes) // (Assume there's only one of these)
	{
		xmlNode* visual_scene = findChildElement(library_visual_scenes, "visual_scene");
		if (visual_scene) // (Assume there's only one of these)
		{
			for (xmlNode* child = visual_scene->children; child; child = child->next)
			{
				if (child->type == XML_ELEMENT_NODE && strcmp((const char*)child->name, "node") == 0)
					if (applyFBXFixesNode(child))
						changed = true;
			}
		}
	}

	return changed;
}

static bool processDocument(xmlNode* root)
{
	xmlNode* asset = findChildElement(root, "asset");
	if (! asset) return false;
	xmlNode* contributor = findChildElement(asset, "contributor");
	if (! contributor) return false;
	xmlNode* authoring_tool = findChildElement(contributor, "authoring_tool");
	if (! authoring_tool) return false;

	xmlNode* authoring_tool_text = authoring_tool->children;
	if (! authoring_tool_text) return false;
	if (authoring_tool_text->type != XML_TEXT_NODE) return false;
	xmlChar* toolname = authoring_tool_text->content;
	Log(LOG_INFO, "Authoring tool: %s", toolname);
	if (strcmp((const char*)toolname, "FBX COLLADA exporter") == 0)
		return applyFBXFixes(root);
	else
		return false;
}

void FixBrokenXML(const char* text, const char** out, size_t* outSize)
{
	Log(LOG_INFO, "Running FixBrokenXML");

	size_t textSize = strlen(text);
	xmlDocPtr doc = xmlParseMemory(text, textSize);

	xmlNode* root = xmlDocGetRootElement(doc);
	if (root && processDocument(root))
	{
		// Reserialising the document, then parsing it again inside FCollada, is a bit ugly;
		// but it's the only way I can see to make it work through FCollada's public API
		xmlChar* mem = NULL;
		int size = -1;
		xmlDocDumpFormatMemory(doc, &mem, &size, 0);
		*out = (const char*)mem;
		*outSize = size;
	}
	else
	{
		*out = text;
		*outSize = textSize;
	}

	xmlFreeDoc(doc);
}
