/* Copyright (C) 2012 Wildfire Games.
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
#include <boost/weak_ptr.hpp>

#include "graphics/ShaderProgram.h"
#include "graphics/ShaderTechnique.h"

#if USE_SHADER_XML_VALIDATION
# include "ps/XML/RelaxNG.h"
#endif

#include <set>

/**
 * Shader manager: loads and caches shader programs.
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
	CShaderProgramPtr LoadProgram(const char* name, const std::map<CStr, CStr>& defines = (std::map<CStr, CStr>()));

	/**
	 * Load a shader effect.
	 * Effects can be implemented via many techniques; this returns the best usable technique.
	 * @param name name of effect XML specification (file is loaded from shaders/effects/${name}.xml)
	 * @param defines key/value set of preprocessor definitions
	 * @return loaded technique, or empty technique on error
	 */
	CShaderTechniquePtr LoadEffect(const char* name, const std::map<CStr, CStr>& defines = (std::map<CStr, CStr>()));

private:

	struct CacheKey
	{
		std::string name;
		std::map<CStr, CStr> defines;

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
	std::map<CacheKey, CShaderProgramPtr> m_ProgramCache;

	// An effect isn't too expensive but it may be loaded many times per frame,
	// so we ought to cache it anyway.
	// For each effect we pick a technique at load time, dependent on various
	// settings (e.g. GL shader extensions, or user's chosen graphics quality)
	// which rarely change. We'll store that collection of settings in
	// EffectContext and reload the effect cache whenever it changes.
	struct EffectContext
	{
		bool hasARB;
		bool hasGLSL;
		bool preferGLSL;

		bool operator==(const EffectContext& b) const
		{
			return hasARB == b.hasARB && hasGLSL == b.hasGLSL && preferGLSL == b.preferGLSL;
		}
	};

	EffectContext GetEffectContextAndVerifyCache();

	std::map<CacheKey, CShaderTechniquePtr> m_EffectCache;
	EffectContext m_EffectCacheContext;


	// Store the set of shaders that need to be reloaded when the given file is modified
	typedef boost::unordered_map<VfsPath, std::set<boost::weak_ptr<CShaderProgram> > > HotloadFilesMap;
	HotloadFilesMap m_HotloadFiles;

#if USE_SHADER_XML_VALIDATION
	RelaxNGValidator m_Validator;
#endif

	bool NewProgram(const char* name, const std::map<CStr, CStr>& defines, CShaderProgramPtr& program);
	bool NewEffect(const char* name, const std::map<CStr, CStr>& defines, const EffectContext& cx, CShaderTechniquePtr& tech);

	static Status ReloadChangedFileCB(void* param, const VfsPath& path);
	Status ReloadChangedFile(const VfsPath& path);
};

#endif // INCLUDED_SHADERMANAGER
