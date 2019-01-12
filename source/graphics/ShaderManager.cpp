/* Copyright (C) 2019 Wildfire Games.
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
#include "lib/config2.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/CStrIntern.h"
#include "ps/Filesystem.h"
#include "ps/PreprocessorWrapper.h"
#include "ps/Profile.h"
#if USE_SHADER_XML_VALIDATION
# include "ps/XML/RelaxNG.h"
#endif
#include "ps/XML/Xeromyces.h"
#include "ps/XML/XMLWriter.h"
#include "renderer/Renderer.h"

TIMER_ADD_CLIENT(tc_ShaderValidation);

CShaderManager::CShaderManager()
{
#if USE_SHADER_XML_VALIDATION
	{
		TIMER_ACCRUE(tc_ShaderValidation);

		if (!CXeromyces::AddValidator(g_VFS, "shader", "shaders/program.rng"))
			LOGERROR("CShaderManager: failed to load grammar shaders/program.rng");
	}
#endif

	// Allow hotloading of textures
	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

CShaderManager::~CShaderManager()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}

CShaderProgramPtr CShaderManager::LoadProgram(const char* name, const CShaderDefines& defines)
{
	CacheKey key = { name, defines };
	std::map<CacheKey, CShaderProgramPtr>::iterator it = m_ProgramCache.find(key);
	if (it != m_ProgramCache.end())
		return it->second;

	CShaderProgramPtr program;
	if (!NewProgram(name, defines, program))
	{
		LOGERROR("Failed to load shader '%s'", name);
		program = CShaderProgramPtr();
	}

	m_ProgramCache[key] = program;
	return program;
}

static GLenum ParseAttribSemantics(const CStr& str)
{
	// Map known semantics onto the attribute locations documented by NVIDIA
	if (str == "gl_Vertex") return 0;
	if (str == "gl_Normal") return 2;
	if (str == "gl_Color") return 3;
	if (str == "gl_SecondaryColor") return 4;
	if (str == "gl_FogCoord") return 5;
	if (str == "gl_MultiTexCoord0") return 8;
	if (str == "gl_MultiTexCoord1") return 9;
	if (str == "gl_MultiTexCoord2") return 10;
	if (str == "gl_MultiTexCoord3") return 11;
	if (str == "gl_MultiTexCoord4") return 12;
	if (str == "gl_MultiTexCoord5") return 13;
	if (str == "gl_MultiTexCoord6") return 14;
	if (str == "gl_MultiTexCoord7") return 15;

	// Define some arbitrary names for user-defined attribute locations
	// that won't conflict with any standard semantics
	if (str == "CustomAttribute0") return 1;
	if (str == "CustomAttribute1") return 6;
	if (str == "CustomAttribute2") return 7;

	debug_warn("Invalid attribute semantics");
	return 0;
}

bool CShaderManager::NewProgram(const char* name, const CShaderDefines& baseDefines, CShaderProgramPtr& program)
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
		XMLWriter_File shaderFile;
		shaderFile.SetPrettyPrint(false);
		shaderFile.XMB(XeroFile);
		bool ok = CXeromyces::ValidateEncoded("shader", wstring_from_utf8(name), shaderFile.GetOutput());
		if (!ok)
			return false;
	}
#endif

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(attrib);
	EL(define);
	EL(fragment);
	EL(stream);
	EL(uniform);
	EL(vertex);
	AT(file);
	AT(if);
	AT(loc);
	AT(name);
	AT(semantics);
	AT(type);
	AT(value);
#undef AT
#undef EL

	CPreprocessorWrapper preprocessor;
	preprocessor.AddDefines(baseDefines);

	XMBElement Root = XeroFile.GetRoot();

	bool isGLSL = (Root.GetAttributes().GetNamedItem(at_type) == "glsl");
	VfsPath vertexFile;
	VfsPath fragmentFile;
	CShaderDefines defines = baseDefines;
	std::map<CStrIntern, int> vertexUniforms;
	std::map<CStrIntern, CShaderProgram::frag_index_pair_t> fragmentUniforms;
	std::map<CStrIntern, int> vertexAttribs;
	int streamFlags = 0;

	XERO_ITER_EL(Root, Child)
	{
		if (Child.GetNodeName() == el_define)
		{
			defines.Add(CStrIntern(Child.GetAttributes().GetNamedItem(at_name)), CStrIntern(Child.GetAttributes().GetNamedItem(at_value)));
		}
		else if (Child.GetNodeName() == el_vertex)
		{
			vertexFile = L"shaders/" + Child.GetAttributes().GetNamedItem(at_file).FromUTF8();

			XERO_ITER_EL(Child, Param)
			{
				XMBAttributeList Attrs = Param.GetAttributes();

				CStr cond = Attrs.GetNamedItem(at_if);
				if (!cond.empty() && !preprocessor.TestConditional(cond))
					continue;

				if (Param.GetNodeName() == el_uniform)
				{
					vertexUniforms[CStrIntern(Attrs.GetNamedItem(at_name))] = Attrs.GetNamedItem(at_loc).ToInt();
				}
				else if (Param.GetNodeName() == el_stream)
				{
					CStr StreamName = Attrs.GetNamedItem(at_name);
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
					int attribLoc = ParseAttribSemantics(Attrs.GetNamedItem(at_semantics));
					vertexAttribs[CStrIntern(Attrs.GetNamedItem(at_name))] = attribLoc;
				}
			}
		}
		else if (Child.GetNodeName() == el_fragment)
		{
			fragmentFile = L"shaders/" + Child.GetAttributes().GetNamedItem(at_file).FromUTF8();

			XERO_ITER_EL(Child, Param)
			{
				XMBAttributeList Attrs = Param.GetAttributes();

				CStr cond = Attrs.GetNamedItem(at_if);
				if (!cond.empty() && !preprocessor.TestConditional(cond))
					continue;

				if (Param.GetNodeName() == el_uniform)
				{
					// A somewhat incomplete listing, missing "shadow" and "rect" versions
					// which are interpreted as 2D (NB: our shadowmaps may change
					// type based on user config).
					GLenum type = GL_TEXTURE_2D;
					CStr t = Attrs.GetNamedItem(at_type);
					if (t == "sampler1D")
#if CONFIG2_GLES
						debug_warn(L"sampler1D not implemented on GLES");
#else
						type = GL_TEXTURE_1D;
#endif
					else if (t == "sampler2D")
						type = GL_TEXTURE_2D;
					else if (t == "sampler3D")
#if CONFIG2_GLES
						debug_warn(L"sampler3D not implemented on GLES");
#else
						type = GL_TEXTURE_3D;
#endif
					else if (t == "samplerCube")
						type = GL_TEXTURE_CUBE_MAP;

					fragmentUniforms[CStrIntern(Attrs.GetNamedItem(at_name))] =
						std::make_pair(Attrs.GetNamedItem(at_loc).ToInt(), type);
				}
			}
		}
	}

	if (isGLSL)
		program = CShaderProgramPtr(CShaderProgram::ConstructGLSL(vertexFile, fragmentFile, defines, vertexAttribs, streamFlags));
	else
		program = CShaderProgramPtr(CShaderProgram::ConstructARB(vertexFile, fragmentFile, defines, vertexUniforms, fragmentUniforms, streamFlags));

	program->Reload();

//	m_HotloadFiles[xmlFilename].insert(program); // TODO: should reload somehow when the XML changes
	m_HotloadFiles[vertexFile].insert(program);
	m_HotloadFiles[fragmentFile].insert(program);

	return true;
}

static GLenum ParseComparisonFunc(const CStr& str)
{
	if (str == "never")
		return GL_NEVER;
	if (str == "always")
		return GL_ALWAYS;
	if (str == "less")
		return GL_LESS;
	if (str == "lequal")
		return GL_LEQUAL;
	if (str == "equal")
		return GL_EQUAL;
	if (str == "gequal")
		return GL_GEQUAL;
	if (str == "greater")
		return GL_GREATER;
	if (str == "notequal")
		return GL_NOTEQUAL;
	debug_warn("Invalid comparison func");
	return GL_ALWAYS;
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
	debug_warn("Invalid blend func");
	return GL_ZERO;
}

size_t CShaderManager::EffectCacheKeyHash::operator()(const EffectCacheKey& key) const
{
	size_t hash = 0;
	boost::hash_combine(hash, key.name.GetHash());
	boost::hash_combine(hash, key.defines1.GetHash());
	boost::hash_combine(hash, key.defines2.GetHash());
	return hash;
}

bool CShaderManager::EffectCacheKey::operator==(const EffectCacheKey& b) const
{
	return (name == b.name && defines1 == b.defines1 && defines2 == b.defines2);
}

CShaderTechniquePtr CShaderManager::LoadEffect(CStrIntern name)
{
	return LoadEffect(name, g_Renderer.GetSystemShaderDefines(), CShaderDefines());
}

CShaderTechniquePtr CShaderManager::LoadEffect(CStrIntern name, const CShaderDefines& defines1, const CShaderDefines& defines2)
{
	// Return the cached effect, if there is one
	EffectCacheKey key = { name, defines1, defines2 };
	EffectCacheMap::iterator it = m_EffectCache.find(key);
	if (it != m_EffectCache.end())
		return it->second;

	// First time we've seen this key, so construct a new effect:

	// Merge the two sets of defines, so NewEffect doesn't have to care about the split
	CShaderDefines defines(defines1);
	defines.SetMany(defines2);

	CShaderTechniquePtr tech(new CShaderTechnique());
	if (!NewEffect(name.c_str(), defines, tech))
	{
		LOGERROR("Failed to load effect '%s'", name.c_str());
		tech = CShaderTechniquePtr();
	}

	m_EffectCache[key] = tech;
	return tech;
}

bool CShaderManager::NewEffect(const char* name, const CShaderDefines& baseDefines, CShaderTechniquePtr& tech)
{
	PROFILE2("loading effect");
	PROFILE2_ATTR("name: %s", name);

	// Shortcut syntax for effects that just contain a single shader
	if (strncmp(name, "shader:", 7) == 0)
	{
		CShaderProgramPtr program = LoadProgram(name+7, baseDefines);
		if (!program)
			return false;
		CShaderPass pass;
		pass.SetShader(program);
		tech->AddPass(pass);
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
	EL(alpha);
	EL(blend);
	EL(define);
	EL(depth);
	EL(pass);
	EL(require);
	EL(sort_by_distance);
	AT(context);
	AT(dst);
	AT(func);
	AT(ref);
	AT(shader);
	AT(shaders);
	AT(src);
	AT(mask);
	AT(name);
	AT(value);
#undef AT
#undef EL

	// Read some defines that influence how we pick techniques
	bool hasARB = (baseDefines.GetInt("SYS_HAS_ARB") != 0);
	bool hasGLSL = (baseDefines.GetInt("SYS_HAS_GLSL") != 0);
	bool preferGLSL = (baseDefines.GetInt("SYS_PREFER_GLSL") != 0);

	// Prepare the preprocessor for conditional tests
	CPreprocessorWrapper preprocessor;
	preprocessor.AddDefines(baseDefines);

	XMBElement Root = XeroFile.GetRoot();

	// Find all the techniques that we can use, and their preference

	std::vector<std::pair<XMBElement, int> > usableTechs;

	XERO_ITER_EL(Root, Technique)
	{
		int preference = 0;
		bool isUsable = true;
		XERO_ITER_EL(Technique, Child)
		{
			XMBAttributeList Attrs = Child.GetAttributes();

			if (Child.GetNodeName() == el_require)
			{
				if (Attrs.GetNamedItem(at_shaders) == "fixed")
				{
					// FFP not supported by OpenGL ES
					#if CONFIG2_GLES
						isUsable = false;
					#endif
				}
				else if (Attrs.GetNamedItem(at_shaders) == "arb")
				{
					if (!hasARB)
						isUsable = false;
				}
				else if (Attrs.GetNamedItem(at_shaders) == "glsl")
				{
					if (!hasGLSL)
						isUsable = false;

					if (preferGLSL)
						preference += 100;
					else
						preference -= 100;
				}
				else if (!Attrs.GetNamedItem(at_context).empty())
				{
					CStr cond = Attrs.GetNamedItem(at_context);
					if (!preprocessor.TestConditional(cond))
						isUsable = false;
				}
			}
		}

		if (isUsable)
			usableTechs.emplace_back(Technique, preference);
	}

	if (usableTechs.empty())
	{
		debug_warn(L"Can't find a usable technique");
		return false;
	}

	// Sort by preference, tie-break on order of specification
	std::stable_sort(usableTechs.begin(), usableTechs.end(),
		[](const std::pair<XMBElement, int>& a, const std::pair<XMBElement, int>& b) {
			return b.second < a.second;
		});

	CShaderDefines techDefines = baseDefines;

	XERO_ITER_EL(usableTechs[0].first, Child)
	{
		if (Child.GetNodeName() == el_define)
		{
			techDefines.Add(CStrIntern(Child.GetAttributes().GetNamedItem(at_name)), CStrIntern(Child.GetAttributes().GetNamedItem(at_value)));
		}
		else if (Child.GetNodeName() == el_sort_by_distance)
		{
			tech->SetSortByDistance(true);
		}
		else if (Child.GetNodeName() == el_pass)
		{
			CShaderDefines passDefines = techDefines;

			CShaderPass pass;

			XERO_ITER_EL(Child, Element)
			{
				if (Element.GetNodeName() == el_define)
				{
					passDefines.Add(CStrIntern(Element.GetAttributes().GetNamedItem(at_name)), CStrIntern(Element.GetAttributes().GetNamedItem(at_value)));
				}
				else if (Element.GetNodeName() == el_alpha)
				{
					GLenum func = ParseComparisonFunc(Element.GetAttributes().GetNamedItem(at_func));
					float ref = Element.GetAttributes().GetNamedItem(at_ref).ToFloat();
					pass.AlphaFunc(func, ref);
				}
				else if (Element.GetNodeName() == el_blend)
				{
					GLenum src = ParseBlendFunc(Element.GetAttributes().GetNamedItem(at_src));
					GLenum dst = ParseBlendFunc(Element.GetAttributes().GetNamedItem(at_dst));
					pass.BlendFunc(src, dst);
				}
				else if (Element.GetNodeName() == el_depth)
				{
					if (!Element.GetAttributes().GetNamedItem(at_func).empty())
						pass.DepthFunc(ParseComparisonFunc(Element.GetAttributes().GetNamedItem(at_func)));

					if (!Element.GetAttributes().GetNamedItem(at_mask).empty())
						pass.DepthMask(Element.GetAttributes().GetNamedItem(at_mask) == "true" ? 1 : 0);
				}
			}

			// Load the shader program after we've read all the possibly-relevant <define>s
			pass.SetShader(LoadProgram(Child.GetAttributes().GetNamedItem(at_shader).c_str(), passDefines));

			tech->AddPass(pass);
		}
	}

	return true;
}

size_t CShaderManager::GetNumEffectsLoaded()
{
	return m_EffectCache.size();
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
		for (std::set<std::weak_ptr<CShaderProgram> >::iterator it = files->second.begin(); it != files->second.end(); ++it)
		{
			if (std::shared_ptr<CShaderProgram> program = it->lock())
				program->Reload();
		}
	}

	// TODO: hotloading changes to shader XML files and effect XML files would be nice

	return INFO::OK;
}
