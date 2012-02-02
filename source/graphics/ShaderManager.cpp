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

#include "precompiled.h"

#include "ShaderManager.h"

#include "graphics/ShaderTechnique.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "ps/XML/XMLWriter.h"
#include "renderer/Renderer.h"

TIMER_ADD_CLIENT(tc_ShaderValidation);

struct revcompare2nd
{
	template<typename S, typename T> bool operator()(const std::pair<S, T>& a, const std::pair<S, T>& b) const
	{
		return b.second < a.second;
	}
};

CShaderManager::CShaderManager()
{
#if USE_SHADER_XML_VALIDATION
	{
		TIMER_ACCRUE(tc_ShaderValidation);
		CVFSFile grammar;
		if (grammar.Load(g_VFS, L"shaders/program.rng") != PSRETURN_OK)
			LOGERROR(L"Failed to read grammar shaders/program.rng");
		else
		{
			if (!m_Validator.LoadGrammar(grammar.GetAsString()))
				LOGERROR(L"Failed to load grammar shaders/program.rng");
		}
	}
#endif

	// Allow hotloading of textures
	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

CShaderManager::~CShaderManager()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}

CShaderProgramPtr CShaderManager::LoadProgram(const char* name, const std::map<CStr, CStr>& defines)
{
	CacheKey key = { name, defines };
	std::map<CacheKey, CShaderProgramPtr>::iterator it = m_ProgramCache.find(key);
	if (it != m_ProgramCache.end())
		return it->second;

	CShaderProgramPtr program;
	if (!NewProgram(name, defines, program))
	{
		LOGERROR(L"Failed to load shader '%hs'", name);
		program = CShaderProgramPtr();
	}

	m_ProgramCache[key] = program;
	return program;
}

bool CShaderManager::NewProgram(const char* name, const std::map<CStr, CStr>& baseDefines, CShaderProgramPtr& program)
{
	PROFILE2("loading shader");
	PROFILE2_ATTR("name: %s", name);

	if (strncmp(name, "fixed:", 6) == 0)
	{
		program = CShaderProgramPtr(CShaderProgram::ConstructFFP(name+6, baseDefines));
		if (!program)
			return false;
		program->Reload();
		return true;
	}

	VfsPath xmlFilename = L"shaders/" + wstring_from_utf8(name) + L".xml";

	CXeromyces XeroFile;
	PSRETURN ret = XeroFile.Load(g_VFS, xmlFilename);
	if (ret != PSRETURN_OK)
		return false;

#if USE_SHADER_XML_VALIDATION
	{
		TIMER_ACCRUE(tc_ShaderValidation);

		// Serialize the XMB data and pass it to the validator
		XML_Start();
		XML_SetPrettyPrint(false);
		XML_WriteXMB(XeroFile);
		bool ok = m_Validator.ValidateEncoded(wstring_from_utf8(name), XML_GetOutput());
		if (!ok)
			return false;
	}
#endif

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(vertex);
	EL(fragment);
	EL(define);
	EL(uniform);
	EL(attrib);
	EL(stream);
	AT(type);
	AT(file);
	AT(name);
	AT(value);
	AT(loc);
#undef AT
#undef EL

	XMBElement Root = XeroFile.GetRoot();

	bool isGLSL = (Root.GetAttributes().GetNamedItem(at_type) == "glsl");
	VfsPath vertexFile;
	VfsPath fragmentFile;
	std::map<CStr, CStr> defines = baseDefines;
	std::map<CStr, int> vertexUniforms;
	std::map<CStr, int> fragmentUniforms;
	int streamFlags = 0;

	XERO_ITER_EL(Root, Child)
	{
		if (Child.GetNodeName() == el_define)
		{
			defines[Child.GetAttributes().GetNamedItem(at_name)] = Child.GetAttributes().GetNamedItem(at_value);
		}
		else if (Child.GetNodeName() == el_vertex)
		{
			vertexFile = L"shaders/" + Child.GetAttributes().GetNamedItem(at_file).FromUTF8();

			XERO_ITER_EL(Child, Param)
			{
				if (Param.GetNodeName() == el_uniform)
				{
					vertexUniforms[Param.GetAttributes().GetNamedItem(at_name)] = Param.GetAttributes().GetNamedItem(at_loc).ToInt();
				}
				else if (Param.GetNodeName() == el_stream)
				{
					CStr StreamName = Param.GetAttributes().GetNamedItem(at_name);
					if (StreamName == "pos")
						streamFlags |= STREAM_POS;
					else if (StreamName == "normal")
						streamFlags |= STREAM_NORMAL;
					else if (StreamName == "color")
						streamFlags |= STREAM_COLOR;
					else if (StreamName == "uv0")
						streamFlags |= STREAM_UV0;
					else if (StreamName == "uv1")
						streamFlags |= STREAM_UV1;
					else if (StreamName == "uv2")
						streamFlags |= STREAM_UV2;
					else if (StreamName == "uv3")
						streamFlags |= STREAM_UV3;
				}
				else if (Param.GetNodeName() == el_attrib)
				{
					// TODO: add support for vertex attributes
				}
			}
		}
		else if (Child.GetNodeName() == el_fragment)
		{
			fragmentFile = L"shaders/" + Child.GetAttributes().GetNamedItem(at_file).FromUTF8();

			XERO_ITER_EL(Child, Param)
			{
				if (Param.GetNodeName() == el_uniform)
				{
					fragmentUniforms[Param.GetAttributes().GetNamedItem(at_name)] = Param.GetAttributes().GetNamedItem(at_loc).ToInt();
				}
			}
		}
	}

	if (isGLSL)
		program = CShaderProgramPtr(CShaderProgram::ConstructGLSL(vertexFile, fragmentFile, defines, streamFlags));
	else
		program = CShaderProgramPtr(CShaderProgram::ConstructARB(vertexFile, fragmentFile, defines, vertexUniforms, fragmentUniforms, streamFlags));

	program->Reload();

//	m_HotloadFiles[xmlFilename].insert(program); // TODO: should reload somehow when the XML changes
	m_HotloadFiles[vertexFile].insert(program);
	m_HotloadFiles[fragmentFile].insert(program);

	return true;
}

static GLenum ParseBlendFunc(const CStr& str)
{
	if (str == "zero")
		return GL_ZERO;
	if (str == "one")
		return GL_ONE;
	if (str == "src_color")
		return GL_SRC_COLOR;
	if (str == "one_minus_src_color")
		return GL_ONE_MINUS_SRC_COLOR;
	if (str == "dst_color")
		return GL_DST_COLOR;
	if (str == "one_minus_dst_color")
		return GL_ONE_MINUS_DST_COLOR;
	if (str == "src_alpha")
		return GL_SRC_ALPHA;
	if (str == "one_minus_src_alpha")
		return GL_ONE_MINUS_SRC_ALPHA;
	if (str == "dst_alpha")
		return GL_DST_ALPHA;
	if (str == "one_minus_dst_alpha")
		return GL_ONE_MINUS_DST_ALPHA;
	if (str == "constant_color")
		return GL_CONSTANT_COLOR;
	if (str == "one_minus_constant_color")
		return GL_ONE_MINUS_CONSTANT_COLOR;
	if (str == "constant_alpha")
		return GL_CONSTANT_ALPHA;
	if (str == "one_minus_constant_alpha")
		return GL_ONE_MINUS_CONSTANT_ALPHA;
	if (str == "src_alpha_saturate")
		return GL_SRC_ALPHA_SATURATE;
	return GL_ZERO;
}

CShaderManager::EffectContext CShaderManager::GetEffectContextAndVerifyCache()
{
	EffectContext cx;
	cx.hasARB = g_Renderer.GetCapabilities().m_ARBProgram;
	cx.hasGLSL = (g_Renderer.GetCapabilities().m_VertexShader && g_Renderer.GetCapabilities().m_FragmentShader);
	cx.preferGLSL = g_Renderer.m_Options.m_PreferGLSL;

	// If the context changed since last time, reload every effect
	if (!(cx == m_EffectCacheContext))
	{
		m_EffectCacheContext = cx;
		for (std::map<CacheKey, CShaderTechniquePtr>::iterator it = m_EffectCache.begin(); it != m_EffectCache.end(); ++it)
		{
			it->second->Reset();
			NewEffect(it->first.name.c_str(), it->first.defines, cx, it->second);
		}
	}

	return cx;
}

CShaderTechniquePtr CShaderManager::LoadEffect(const char* name, const std::map<CStr, CStr>& defines)
{
	EffectContext cx = GetEffectContextAndVerifyCache();

	CacheKey key = { name, defines };
	std::map<CacheKey, CShaderTechniquePtr>::iterator it = m_EffectCache.find(key);
	if (it != m_EffectCache.end())
		return it->second;

	CShaderTechniquePtr tech(new CShaderTechnique());
	if (!NewEffect(name, defines, cx, tech))
	{
		LOGERROR(L"Failed to load effect '%hs'", name);
		tech = CShaderTechniquePtr();
	}

	m_EffectCache[key] = tech;
	return tech;
}

bool CShaderManager::NewEffect(const char* name, const std::map<CStr, CStr>& baseDefines, const EffectContext& cx, CShaderTechniquePtr& tech)
{
	PROFILE2("loading effect");
	PROFILE2_ATTR("name: %s", name);

	// Shortcut syntax for effects that just contain a single shader
	if (strncmp(name, "shader:", 7) == 0)
	{
		CShaderProgramPtr program = LoadProgram(name+7, baseDefines);
		if (!program)
			return false;
		tech->AddPass(CShaderPass(program));
		return true;
	}

	VfsPath xmlFilename = L"shaders/effects/" + wstring_from_utf8(name) + L".xml";

	CXeromyces XeroFile;
	PSRETURN ret = XeroFile.Load(g_VFS, xmlFilename);
	if (ret != PSRETURN_OK)
		return false;

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(pass);
	EL(require);
	EL(blend);
	AT(shaders);
	AT(shader);
	AT(src);
	AT(dst);
#undef AT
#undef EL

	XMBElement Root = XeroFile.GetRoot();

	// Find all the techniques that we can use, and their preference

	std::vector<std::pair<XMBElement, int> > usableTechs;

	XERO_ITER_EL(Root, Technique)
	{
		int preference = 0;
		bool isUsable = true;
		XERO_ITER_EL(Technique, Child)
		{
			if (Child.GetNodeName() == el_require)
			{
				if (Child.GetAttributes().GetNamedItem(at_shaders) == "fixed")
				{
					// FFP not supported by OpenGL ES
					#if CONFIG2_GLES
						isUsable = false;
					#endif
				}
				else if (Child.GetAttributes().GetNamedItem(at_shaders) == "arb")
				{
					if (!cx.hasARB)
						isUsable = false;
				}
				else if (Child.GetAttributes().GetNamedItem(at_shaders) == "glsl")
				{
					if (!cx.hasGLSL)
						isUsable = false;

					if (cx.preferGLSL)
						preference += 100;
					else
						preference -= 100;
				}
			}
		}

		if (isUsable)
			usableTechs.push_back(std::make_pair(Technique, preference));
	}

	if (usableTechs.empty())
	{
		debug_warn(L"Can't find a usable technique");
		return false;
	}

	// Sort by preference, tie-break on order of specification
	std::stable_sort(usableTechs.begin(), usableTechs.end(), revcompare2nd());

	XERO_ITER_EL(usableTechs[0].first, Child)
	{
		if (Child.GetNodeName() == el_pass)
		{
			CShaderProgramPtr shader = LoadProgram(Child.GetAttributes().GetNamedItem(at_shader).c_str(), baseDefines);
			CShaderPass pass(shader);

			XERO_ITER_EL(Child, Element)
			{
				if (Element.GetNodeName() == el_blend)
				{
					GLenum src = ParseBlendFunc(Element.GetAttributes().GetNamedItem(at_src));
					GLenum dst = ParseBlendFunc(Element.GetAttributes().GetNamedItem(at_dst));
					pass.BlendFunc(src, dst);
				}
			}

			tech->AddPass(pass);
		}
	}

	return true;
}

/*static*/ Status CShaderManager::ReloadChangedFileCB(void* param, const VfsPath& path)
{
	return static_cast<CShaderManager*>(param)->ReloadChangedFile(path);
}

Status CShaderManager::ReloadChangedFile(const VfsPath& path)
{
	// Find all shaders using this file
	HotloadFilesMap::iterator files = m_HotloadFiles.find(path);
	if (files != m_HotloadFiles.end())
	{
		// Reload all shaders using this file
		for (std::set<boost::weak_ptr<CShaderProgram> >::iterator it = files->second.begin(); it != files->second.end(); ++it)
		{
			if (shared_ptr<CShaderProgram> program = it->lock())
				program->Reload();
		}
	}

	// TODO: hotloading changes to shader XML files and effect XML files would be nice

	return INFO::OK;
}
