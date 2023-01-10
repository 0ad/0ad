/* Copyright (C) 2023 Wildfire Games.
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

#include "graphics/PreprocessorWrapper.h"
#include "graphics/ShaderTechnique.h"
#include "lib/config2.h"
#include "lib/hash.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/CStrIntern.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "ps/VideoMode.h"
#include "renderer/backend/IDevice.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"

#define USE_SHADER_XML_VALIDATION 1

#if USE_SHADER_XML_VALIDATION
#include "ps/XML/RelaxNG.h"
#include "ps/XML/XMLWriter.h"
#endif

#include <optional>
#include <vector>

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

CShaderProgramPtr CShaderManager::LoadProgram(const CStr& name, const CShaderDefines& defines)
{
	CacheKey key = { name, defines };
	std::map<CacheKey, CShaderProgramPtr>::iterator it = m_ProgramCache.find(key);
	if (it != m_ProgramCache.end())
		return it->second;

	CShaderProgramPtr program = CShaderProgram::Create(name, defines);
	if (program)
	{
		for (const VfsPath& path : program->GetFileDependencies())
			AddProgramFileDependency(program, path);
	}
	else
	{
		LOGERROR("Failed to load shader '%s'", name);
	}

	m_ProgramCache[key] = program;
	return program;
}

size_t CShaderManager::EffectCacheKeyHash::operator()(const EffectCacheKey& key) const
{
	size_t hash = 0;
	hash_combine(hash, key.name.GetHash());
	hash_combine(hash, key.defines.GetHash());
	return hash;
}

bool CShaderManager::EffectCacheKey::operator==(const EffectCacheKey& b) const
{
	return name == b.name && defines == b.defines;
}

CShaderTechniquePtr CShaderManager::LoadEffect(CStrIntern name)
{
	return LoadEffect(name, CShaderDefines());
}

CShaderTechniquePtr CShaderManager::LoadEffect(CStrIntern name, const CShaderDefines& defines)
{
	// Return the cached effect, if there is one
	EffectCacheKey key = { name, defines };
	EffectCacheMap::iterator it = m_EffectCache.find(key);
	if (it != m_EffectCache.end())
		return it->second;

	// First time we've seen this key, so construct a new effect:
	const VfsPath xmlFilename = L"shaders/effects/" + wstring_from_utf8(name.string()) + L".xml";
	CShaderTechniquePtr tech = std::make_shared<CShaderTechnique>(
		xmlFilename, defines, PipelineStateDescCallback{});
	if (!LoadTechnique(tech))
	{
		LOGERROR("Failed to load effect '%s'", name.c_str());
		tech = CShaderTechniquePtr();
	}

	m_EffectCache[key] = tech;
	return tech;
}

CShaderTechniquePtr CShaderManager::LoadEffect(
	CStrIntern name, const CShaderDefines& defines, const PipelineStateDescCallback& callback)
{
	// We don't cache techniques with callbacks.
	const VfsPath xmlFilename = L"shaders/effects/" + wstring_from_utf8(name.string()) + L".xml";
	CShaderTechniquePtr technique = std::make_shared<CShaderTechnique>(xmlFilename, defines, callback);
	if (!LoadTechnique(technique))
	{
		LOGERROR("Failed to load effect '%s'", name.c_str());
		return {};
	}
	return technique;
}

bool CShaderManager::LoadTechnique(CShaderTechniquePtr& tech)
{
	PROFILE2("loading technique");
	PROFILE2_ATTR("name: %s", tech->GetPath().string8().c_str());

	AddTechniqueFileDependency(tech, tech->GetPath());

	CXeromyces XeroFile;
	PSRETURN ret = XeroFile.Load(g_VFS, tech->GetPath());
	if (ret != PSRETURN_OK)
		return false;

	Renderer::Backend::IDevice* device = g_VideoMode.GetBackendDevice();

	// By default we assume that we have techinques for every dummy shader.
	if (device->GetBackend() == Renderer::Backend::Backend::DUMMY)
	{
		CShaderProgramPtr shaderProgram = LoadProgram("dummy", tech->GetShaderDefines());
		std::vector<CShaderPass> techPasses;
		Renderer::Backend::SGraphicsPipelineStateDesc passPipelineStateDesc =
			Renderer::Backend::MakeDefaultGraphicsPipelineStateDesc();
		passPipelineStateDesc.shaderProgram = shaderProgram->GetBackendShaderProgram();
		techPasses.emplace_back(
			device->CreateGraphicsPipelineState(passPipelineStateDesc), shaderProgram);
		tech->SetPasses(std::move(techPasses));
		return true;
	}

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(blend);
	EL(color);
	EL(cull);
	EL(define);
	EL(depth);
	EL(pass);
	EL(polygon);
	EL(require);
	EL(sort_by_distance);
	EL(stencil);
	AT(compare);
	AT(constant);
	AT(context);
	AT(depth_fail);
	AT(dst);
	AT(fail);
	AT(front_face);
	AT(func);
	AT(mask);
	AT(mask_read);
	AT(mask_red);
	AT(mask_green);
	AT(mask_blue);
	AT(mask_alpha);
	AT(mode);
	AT(name);
	AT(op);
	AT(pass);
	AT(reference);
	AT(shader);
	AT(shaders);
	AT(src);
	AT(test);
	AT(value);
#undef AT
#undef EL

	// Prepare the preprocessor for conditional tests
	CPreprocessorWrapper preprocessor;
	preprocessor.AddDefines(tech->GetShaderDefines());

	XMBElement root = XeroFile.GetRoot();

	// Find all the techniques that we can use, and their preference

	std::optional<XMBElement> usableTech;
	XERO_ITER_EL(root, technique)
	{
		bool isUsable = true;
		XERO_ITER_EL(technique, child)
		{
			XMBAttributeList attrs = child.GetAttributes();

			// TODO: require should be an attribute of the tech and not its child.
			if (child.GetNodeName() == el_require)
			{
				if (attrs.GetNamedItem(at_shaders) == "arb")
				{
					if (device->GetBackend() != Renderer::Backend::Backend::GL_ARB ||
						!device->GetCapabilities().ARBShaders)
					{
						isUsable = false;
					}
				}
				else if (attrs.GetNamedItem(at_shaders) == "glsl")
				{
					if (device->GetBackend() != Renderer::Backend::Backend::GL)
						isUsable = false;
				}
				else if (attrs.GetNamedItem(at_shaders) == "spirv")
				{
					if (device->GetBackend() != Renderer::Backend::Backend::VULKAN)
						isUsable = false;
				}
				else if (!attrs.GetNamedItem(at_context).empty())
				{
					CStr cond = attrs.GetNamedItem(at_context);
					if (!preprocessor.TestConditional(cond))
						isUsable = false;
				}
			}
		}

		if (isUsable)
		{
			usableTech.emplace(technique);
			break;
		}
	}

	if (!usableTech.has_value())
	{
		debug_warn(L"Can't find a usable technique");
		return false;
	}

	tech->SetSortByDistance(false);

	CShaderDefines techDefines = tech->GetShaderDefines();
	XERO_ITER_EL((*usableTech), Child)
	{
		if (Child.GetNodeName() == el_define)
		{
			techDefines.Add(CStrIntern(Child.GetAttributes().GetNamedItem(at_name)), CStrIntern(Child.GetAttributes().GetNamedItem(at_value)));
		}
		else if (Child.GetNodeName() == el_sort_by_distance)
		{
			tech->SetSortByDistance(true);
		}
	}
	// We don't want to have a shader context depending on the order of define and
	// pass tags.
	// TODO: we might want to implement that in a proper way via splitting passes
	// and tags in different groups in XML.
	std::vector<CShaderPass> techPasses;
	XERO_ITER_EL((*usableTech), Child)
	{
		if (Child.GetNodeName() == el_pass)
		{
			CShaderDefines passDefines = techDefines;

			Renderer::Backend::SGraphicsPipelineStateDesc passPipelineStateDesc =
				Renderer::Backend::MakeDefaultGraphicsPipelineStateDesc();

			XERO_ITER_EL(Child, Element)
			{
				if (Element.GetNodeName() == el_define)
				{
					passDefines.Add(CStrIntern(Element.GetAttributes().GetNamedItem(at_name)), CStrIntern(Element.GetAttributes().GetNamedItem(at_value)));
				}
				else if (Element.GetNodeName() == el_blend)
				{
					passPipelineStateDesc.blendState.enabled = true;
					passPipelineStateDesc.blendState.srcColorBlendFactor = passPipelineStateDesc.blendState.srcAlphaBlendFactor =
						Renderer::Backend::ParseBlendFactor(Element.GetAttributes().GetNamedItem(at_src));
					passPipelineStateDesc.blendState.dstColorBlendFactor = passPipelineStateDesc.blendState.dstAlphaBlendFactor =
						Renderer::Backend::ParseBlendFactor(Element.GetAttributes().GetNamedItem(at_dst));
					if (!Element.GetAttributes().GetNamedItem(at_op).empty())
					{
						passPipelineStateDesc.blendState.colorBlendOp = passPipelineStateDesc.blendState.alphaBlendOp =
							Renderer::Backend::ParseBlendOp(Element.GetAttributes().GetNamedItem(at_op));
					}
					if (!Element.GetAttributes().GetNamedItem(at_constant).empty())
					{
						if (!passPipelineStateDesc.blendState.constant.ParseString(
								Element.GetAttributes().GetNamedItem(at_constant)))
						{
							LOGERROR("Failed to parse blend constant: %s",
								Element.GetAttributes().GetNamedItem(at_constant).c_str());
						}
					}
				}
				else if (Element.GetNodeName() == el_color)
				{
					passPipelineStateDesc.blendState.colorWriteMask = 0;
				#define MASK_CHANNEL(ATTRIBUTE, VALUE) \
					if (Element.GetAttributes().GetNamedItem(ATTRIBUTE) == "TRUE") \
						passPipelineStateDesc.blendState.colorWriteMask |= Renderer::Backend::ColorWriteMask::VALUE

					MASK_CHANNEL(at_mask_red, RED);
					MASK_CHANNEL(at_mask_green, GREEN);
					MASK_CHANNEL(at_mask_blue, BLUE);
					MASK_CHANNEL(at_mask_alpha, ALPHA);
				#undef MASK_CHANNEL
				}
				else if (Element.GetNodeName() == el_cull)
				{
					if (!Element.GetAttributes().GetNamedItem(at_mode).empty())
					{
						passPipelineStateDesc.rasterizationState.cullMode =
							Renderer::Backend::ParseCullMode(Element.GetAttributes().GetNamedItem(at_mode));
					}
					if (!Element.GetAttributes().GetNamedItem(at_front_face).empty())
					{
						passPipelineStateDesc.rasterizationState.frontFace =
							Renderer::Backend::ParseFrontFace(Element.GetAttributes().GetNamedItem(at_front_face));
					}
				}
				else if (Element.GetNodeName() == el_depth)
				{
					if (!Element.GetAttributes().GetNamedItem(at_test).empty())
					{
						passPipelineStateDesc.depthStencilState.depthTestEnabled =
							Element.GetAttributes().GetNamedItem(at_test) == "TRUE";
					}

					if (!Element.GetAttributes().GetNamedItem(at_func).empty())
					{
						passPipelineStateDesc.depthStencilState.depthCompareOp =
							Renderer::Backend::ParseCompareOp(Element.GetAttributes().GetNamedItem(at_func));
					}

					if (!Element.GetAttributes().GetNamedItem(at_mask).empty())
					{
						passPipelineStateDesc.depthStencilState.depthWriteEnabled =
							Element.GetAttributes().GetNamedItem(at_mask) == "true";
					}
				}
				else if (Element.GetNodeName() == el_polygon)
				{
					if (!Element.GetAttributes().GetNamedItem(at_mode).empty())
					{
						passPipelineStateDesc.rasterizationState.polygonMode =
							Renderer::Backend::ParsePolygonMode(Element.GetAttributes().GetNamedItem(at_mode));
					}
				}
				else if (Element.GetNodeName() == el_stencil)
				{
					if (!Element.GetAttributes().GetNamedItem(at_test).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilTestEnabled =
							Element.GetAttributes().GetNamedItem(at_test) == "TRUE";
					}

					if (!Element.GetAttributes().GetNamedItem(at_reference).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilReference =
							Element.GetAttributes().GetNamedItem(at_reference).ToULong();
					}
					if (!Element.GetAttributes().GetNamedItem(at_mask_read).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilReadMask =
							Element.GetAttributes().GetNamedItem(at_mask_read).ToULong();
					}
					if (!Element.GetAttributes().GetNamedItem(at_mask).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilWriteMask =
							Element.GetAttributes().GetNamedItem(at_mask).ToULong();
					}

					if (!Element.GetAttributes().GetNamedItem(at_compare).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilFrontFace.compareOp =
							passPipelineStateDesc.depthStencilState.stencilBackFace.compareOp =
								Renderer::Backend::ParseCompareOp(Element.GetAttributes().GetNamedItem(at_compare));
					}
					if (!Element.GetAttributes().GetNamedItem(at_fail).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilFrontFace.failOp =
							passPipelineStateDesc.depthStencilState.stencilBackFace.failOp =
								Renderer::Backend::ParseStencilOp(Element.GetAttributes().GetNamedItem(at_fail));
					}
					if (!Element.GetAttributes().GetNamedItem(at_pass).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilFrontFace.passOp =
							passPipelineStateDesc.depthStencilState.stencilBackFace.passOp =
							Renderer::Backend::ParseStencilOp(Element.GetAttributes().GetNamedItem(at_pass));
					}
					if (!Element.GetAttributes().GetNamedItem(at_depth_fail).empty())
					{
						passPipelineStateDesc.depthStencilState.stencilFrontFace.depthFailOp =
							passPipelineStateDesc.depthStencilState.stencilBackFace.depthFailOp =
							Renderer::Backend::ParseStencilOp(Element.GetAttributes().GetNamedItem(at_depth_fail));
					}
				}
			}

			// Load the shader program after we've read all the possibly-relevant <define>s.
			CShaderProgramPtr shaderProgram =
				LoadProgram(Child.GetAttributes().GetNamedItem(at_shader).c_str(), passDefines);
			if (shaderProgram)
			{
				for (const VfsPath& shaderProgramPath : shaderProgram->GetFileDependencies())
					AddTechniqueFileDependency(tech, shaderProgramPath);
				if (tech->GetPipelineStateDescCallback())
					tech->GetPipelineStateDescCallback()(passPipelineStateDesc);
				passPipelineStateDesc.shaderProgram = shaderProgram->GetBackendShaderProgram();
				techPasses.emplace_back(
					device->CreateGraphicsPipelineState(passPipelineStateDesc), shaderProgram);
			}
		}
	}

	tech->SetPasses(std::move(techPasses));

	return true;
}

size_t CShaderManager::GetNumEffectsLoaded() const
{
	return m_EffectCache.size();
}

/*static*/ Status CShaderManager::ReloadChangedFileCB(void* param, const VfsPath& path)
{
	return static_cast<CShaderManager*>(param)->ReloadChangedFile(path);
}

Status CShaderManager::ReloadChangedFile(const VfsPath& path)
{
	// Find all shader programs using this file.
	const auto programs = m_HotloadPrograms.find(path);
	if (programs != m_HotloadPrograms.end())
	{
		// Reload all shader programs using this file.
		for (const std::weak_ptr<CShaderProgram>& ptr : programs->second)
			if (std::shared_ptr<CShaderProgram> program = ptr.lock())
				program->Reload();
	}

	// Find all shader techinques using this file. We need to reload them after
	// shader programs.
	const auto techniques = m_HotloadTechniques.find(path);
	if (techniques != m_HotloadTechniques.end())
	{
		// Reload all shader techinques using this file.
		for (const std::weak_ptr<CShaderTechnique>& ptr : techniques->second)
			if (std::shared_ptr<CShaderTechnique> technique = ptr.lock())
			{
				if (!LoadTechnique(technique))
					LOGERROR("Failed to reload technique '%s'", technique->GetPath().string8().c_str());
			}
	}

	return INFO::OK;
}

void CShaderManager::AddTechniqueFileDependency(const CShaderTechniquePtr& technique, const VfsPath& path)
{
	m_HotloadTechniques[path].insert(technique);
}

void CShaderManager::AddProgramFileDependency(const CShaderProgramPtr& program, const VfsPath& path)
{
	m_HotloadPrograms[path].insert(program);
}
