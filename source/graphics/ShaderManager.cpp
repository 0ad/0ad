/* Copyright (C) 2011 Wildfire Games.
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

#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "ps/XML/XMLWriter.h"
#include "renderer/Renderer.h"

TIMER_ADD_CLIENT(tc_ShaderValidation);

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
	std::map<CacheKey, CShaderProgramPtr>::iterator it = m_Cache.find(key);
	if (it != m_Cache.end())
		return it->second;

	CShaderProgramPtr program;
	if (NewProgram(name, defines, program) != PSRETURN_OK)
	{
		LOGERROR(L"Failed to load shader '%hs'", name);
		program = CShaderProgramPtr();
	}

	m_Cache[key] = program;
	return program;
}

bool CShaderManager::NewProgram(const char* name, const std::map<CStr, CStr>& baseDefines, CShaderProgramPtr& program)
{
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
					fragmentUniforms[Param.GetAttributes().GetNamedItem(at_name)] = Param.GetAttributes().GetNamedItem(at_loc).ToInt();
			}
		}
	}

	// TODO: add GLSL support
	ENSURE(!isGLSL);

	program = CShaderProgramPtr(CShaderProgram::ConstructARB(vertexFile, fragmentFile, defines, vertexUniforms, fragmentUniforms, streamFlags));
	program->Reload();

//	m_HotloadFiles[xmlFilename].insert(program); // TODO: should reload somehow when the XML changes
	m_HotloadFiles[vertexFile].insert(program);
	m_HotloadFiles[fragmentFile].insert(program);

	return PSRETURN_OK;
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

	return INFO::OK;
}
