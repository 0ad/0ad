#include "AtlasObjectText.h"
#include "AtlasObjectImpl.h"
#include "AtlasObject.h"

static std::wstring ConvertRecursive(const AtNode::Ptr obj, bool use_brackets = true)
{
	// Convert (1, ()) into "1"
	// Convert (3, (d: (...), e: (...))) into "3 (conv(...), conv(...))"
	// etc
	// resulting in data-loss [because of the key names], and a rather arbitrary
	// [alphabetical by key] ordering of children, but at least it's fairly readable

	if (! obj)
		return L"";

	std::wstring result;

	bool has_value = (obj->value.length() != 0);
	bool has_children = (obj->children.size() != 0);

	if (has_value && has_children)
		result = obj->value + L" ";
	else if (has_value)
		result = obj->value;
	// else no value; result = L""

	if (has_children)
	{
		if (use_brackets)
			result += L"(";

		bool first_child = true; // so we can add ", " in appropriate places

		for (AtNode::child_maptype::const_iterator it = obj->children.begin();
			it != obj->children.end();
			++it)
		{
			if (! first_child)
				result += L", ";
			else
				first_child = false;

			result += ConvertRecursive(it->second);
		}

		if (use_brackets)
			result += L")";
	}

	return result;
}

std::wstring AtlasObject::ConvertToString(const AtObj& obj)
{
	return ConvertRecursive(obj.p, false);
}
