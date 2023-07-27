/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "MouseEventMask.h"

#include "gui/CGUISetting.h"
#include "lib/tex/tex.h"
#include "maths/Rect.h"
#include "maths/Vector2D.h"
#include "ps/Filesystem.h"
#include "ps/CacheLoader.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "scriptinterface/ScriptConversions.h"

#include <string_view>

class IGUIObject;
class IGUISetting;

namespace
{
	const CStr MOUSE_EVENT_MASK = "mouse_event_mask";
}

class CGUIMouseEventMask::Impl
{
public:
	virtual ~Impl() = default;
	virtual bool IsMouseOver(const CVector2D& mousePos, const CRect& objectSize) const = 0;
};

CGUIMouseEventMask::CGUIMouseEventMask(IGUIObject* owner) : IGUISetting(MOUSE_EVENT_MASK, owner)
{
}

CGUIMouseEventMask::~CGUIMouseEventMask()
{
}

void CGUIMouseEventMask::ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue Value)
{
	Script::ToJSVal(rq, Value, m_Spec);
}

bool CGUIMouseEventMask::DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value)
{
	CStrW spec;
	if (!Script::FromJSVal(rq, value, spec))
		return false;
	return DoFromString(spec);
}

bool CGUIMouseEventMask::IsMouseOver(const CVector2D& mousePos, const CRect& objectSize) const
{
	if (m_Impl)
		return m_Impl->IsMouseOver(mousePos, objectSize);
	return false;
}

class CGUIMouseEventMaskTexture final : public CGUIMouseEventMask::Impl
{
public:
	static constexpr std::string_view identifier = "texture:";
	static constexpr size_t specOffset = identifier.size();

	static std::unique_ptr<CGUIMouseEventMaskTexture> Create(const std::string_view spec)
	{
		std::shared_ptr<u8> shapeFile;
		CCacheLoader loader(g_VFS, L".dds");
		VfsPath sourcePath = VfsPath("art") / L"textures" / L"ui" /
			std::string{spec.substr(specOffset)};
		VfsPath archivePath = loader.ArchiveCachePath(sourcePath);
		Status status;
		size_t size;
		if (loader.CanUseArchiveCache(sourcePath, archivePath))
			status = g_VFS->LoadFile(archivePath, shapeFile, size);
		else
			status = g_VFS->LoadFile(sourcePath, shapeFile, size);
		if (status != INFO::OK)
		{
			LOGWARNING("Mouse event mask texture not found ('%s')", spec);
			return nullptr;
		}
		Tex tex;
		if (tex.decode(shapeFile, size) != INFO::OK)
		{
			LOGERROR("Could not decode mouse event mask texture '%s'", spec);
			return nullptr;
		}
		if (tex.transform(TEX_DXT | TEX_MIPMAPS) != INFO::OK)
		{
			LOGERROR("Could not transform texture '%s'", spec);
			return nullptr;
		}

		// TODO > would be nice to downscale, maybe.
		if (tex.m_Width == 0 || tex.m_Height == 0)
		{
			LOGERROR("Mouse event mask texture must have a non-null size ('%s')", spec);
			return nullptr;
		}

		auto mask = std::make_unique<CGUIMouseEventMaskTexture>();
		mask->m_Width = static_cast<u16>(tex.m_Width);
		mask->m_Height = static_cast<u16>(tex.m_Height);
		mask->m_Data.reserve(mask->m_Width * mask->m_Height);
		for (u8* ptr = tex.get_data(); ptr < tex.get_data() + tex.m_DataSize; ptr += tex.m_Bpp/8)
		{
			if (tex.m_Bpp == 32)
				mask->m_Data.push_back(*(ptr + 3) > 0);
			else
				mask->m_Data.push_back(*ptr > 0);
		}

		return mask;
	}

	virtual bool IsMouseOver(const CVector2D& mousePos, const CRect& objectSize) const override
	{
		if (m_Data.empty())
			return false;

		CVector2D delta = mousePos - objectSize.TopLeft();
		int x = floor(delta.X * m_Width / objectSize.GetWidth());
		int y = floor(delta.Y * m_Height / objectSize.GetHeight());
		if (x < 0 || y < 0 || x >= m_Width || y >= m_Height)
			return false;

		return m_Data[x + y * m_Width];
	}

private:
	// This uses the bool specialization on purpose for the 'compression' effect.
	std::vector<bool> m_Data;
	u16 m_Width;
	u16 m_Height;
};

bool CGUIMouseEventMask::DoFromString(const CStrW& Value)
{
	std::string spec = Value.ToUTF8();
	if (spec == m_Spec)
		return true;

	// Empty spec - reset the mask and return.
	if (spec.empty())
	{
		m_Impl.reset();
		return true;
	}

	if (spec.find(CGUIMouseEventMaskTexture::identifier) != std::string::npos)
	{
		std::unique_ptr<CGUIMouseEventMask::Impl> newImpl = CGUIMouseEventMaskTexture::Create(spec);
		if (newImpl)
		{
			m_Impl = std::move(newImpl);
			m_Spec = spec;
			return true;
		}
		else
			LOGERROR("Could not create shape for: %s", spec);
	}
	else
		LOGWARNING("Unknown clickable shape: %s", spec);
	return false;
}
