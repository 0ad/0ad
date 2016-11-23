/* Copyright (C) 2015 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SHADERMANAGER
#define INCLUDED_SHADERMANAGER

#define USE_SHADER_XML_VALIDATION 1

#include <boost/unordered_map.hpp>
#include <memory>
#include <set>

#include "graphics/ShaderDefines.h"
#include "graphics/ShaderProgram.h"
#include "graphics/ShaderTechnique.h"

#if USE_SHADER_XML_VALIDATION
# include "ps/XML/RelaxNG.h"
#endif


/**
 * Shader manager: loads and caches shader programs.
 *
 * For a high-level overview of shaders and materials, see
 * http://trac.wildfiregames.com/wiki/MaterialSystem
 */
class CShaderManager
{
public:
	CShaderManager();
	~CShaderManager();

	/**
	 * Load a shader program.
	 * @param name name of shader XML specification (file is loaded from shaders/${name}.xml)
	 * @param defines key/value set of preprocessor definitions
	 * @return loaded program, or null pointer on error
	 */
	CShaderProgramPtr LoadProgram(const char* name, const CShaderDefines& defines);

	/**
	 * Load a shader effect.
	 * Effects can be implemented via many techniques; this returns the best usable technique.
	 * @param name name of effect XML specification (file is loaded from shaders/effects/${name}.xml)
	 * @param defines1,defines2 key/value set of preprocessor definitions; defines2 has higher precedence
	 * @return loaded technique, or empty technique on error
	 */
	CShaderTechniquePtr LoadEffect(CStrIntern name, const CShaderDefines& defines1, const CShaderDefines& defines2);

	/**
	 * Load a shader effect, with default system defines (from CRenderer::GetSystemShaderDefines).
	 */
	CShaderTechniquePtr LoadEffect(CStrIntern name);

	/**
	 * Returns the number of shader effects that are currently loaded.
	 */
	size_t GetNumEffectsLoaded();

private:

	struct CacheKey
	{
		std::string name;
		CShaderDefines defines;

		bool operator<(const CacheKey& k) const
		{
			if (name < k.name) return true;
			if (k.name < name) return false;
			return defines < k.defines;
		}
	};

	// A CShaderProgram contains expensive GL state, so we ought to cache it.
	// The compiled state depends solely on the filename and list of defines,
	// so we store that in CacheKey.
	// TODO: is this cache useful when we already have an effect cache?
	std::map<CacheKey, CShaderProgramPtr> m_ProgramCache;

	/**
	 * Key for effect cache lookups.
	 * This stores two separate CShaderDefines because the renderer typically
	 * has one set from the rendering context and one set from the material;
	 * by handling both separately here, we avoid the cost of having to merge
	 * the two sets into a single one before doing the cache lookup.
	 */
	struct EffectCacheKey
	{
		CStrIntern name;
		CShaderDefines defines1;
		CShaderDefines defines2;

		bool operator==(const EffectCacheKey& b) const;
	};

	struct EffectCacheKeyHash
	{
		size_t operator()(const EffectCacheKey& key) const;
	};

	typedef boost::unordered_map<EffectCacheKey, CShaderTechniquePtr, EffectCacheKeyHash> EffectCacheMap;
	EffectCacheMap m_EffectCache;

	// Store the set of shaders that need to be reloaded when the given file is modified
	typedef boost::unordered_map<VfsPath, std::set<std::weak_ptr<CShaderProgram>, std::owner_less<std::weak_ptr<CShaderProgram>>>> HotloadFilesMap;
	HotloadFilesMap m_HotloadFiles;

	bool NewProgram(const char* name, const CShaderDefines& defines, CShaderProgramPtr& program);
	bool NewEffect(const char* name, const CShaderDefines& defines, CShaderTechniquePtr& tech);

	static Status ReloadChangedFileCB(void* param, const VfsPath& path);
	Status ReloadChangedFile(const VfsPath& path);
};

#endif // INCLUDED_SHADERMANAGER
