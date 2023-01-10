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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_DEVICE
#define INCLUDED_RENDERER_BACKEND_VULKAN_DEVICE

#include "renderer/backend/IDevice.h"
#include "renderer/backend/vulkan/DeviceForward.h"
#include "renderer/backend/vulkan/DeviceSelection.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/VMA.h"
#include "scriptinterface/ScriptForward.h"

#include <glad/vulkan.h>
#include <memory>
#include <limits>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef struct SDL_Window SDL_Window;

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

static constexpr size_t NUMBER_OF_FRAMES_IN_FLIGHT = 3;

class CBuffer;
class CDescriptorManager;
class CFramebuffer;
class CRenderPassManager;
class CRingCommandContext;
class CSamplerManager;
class CSubmitScheduler;
class CSwapChain;

class CDevice final : public IDevice
{
public:
	/**
	 * Creates the Vulkan device.
	 */
	static std::unique_ptr<CDevice> Create(SDL_Window* window);

	~CDevice() override;

	Backend GetBackend() const override { return Backend::VULKAN; }

	const std::string& GetName() const override { return m_Name; }
	const std::string& GetVersion() const override { return m_Version; }
	const std::string& GetDriverInformation() const override { return m_DriverInformation; }
	const std::vector<std::string>& GetExtensions() const override { return m_Extensions; }

	void Report(const ScriptRequest& rq, JS::HandleValue settings) override;

	std::unique_ptr<IDeviceCommandContext> CreateCommandContext() override;

	std::unique_ptr<IGraphicsPipelineState> CreateGraphicsPipelineState(
		const SGraphicsPipelineStateDesc& pipelineStateDesc) override;

	std::unique_ptr<IVertexInputLayout> CreateVertexInputLayout(
		const PS::span<const SVertexAttributeFormat> attributes) override;

	std::unique_ptr<ITexture> CreateTexture(
		const char* name, const ITexture::Type type, const uint32_t usage,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount) override;

	std::unique_ptr<ITexture> CreateTexture2D(
		const char* name, const uint32_t usage,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount = 1, const uint32_t sampleCount = 1) override;

	std::unique_ptr<IFramebuffer> CreateFramebuffer(
		const char* name, SColorAttachment* colorAttachment,
		SDepthStencilAttachment* depthStencilAttachment) override;

	std::unique_ptr<IBuffer> CreateBuffer(
		const char* name, const IBuffer::Type type, const uint32_t size, const bool dynamic) override;

	std::unique_ptr<CBuffer> CreateCBuffer(
		const char* name, const IBuffer::Type type, const uint32_t size, const bool dynamic);

	std::unique_ptr<IShaderProgram> CreateShaderProgram(
		const CStr& name, const CShaderDefines& defines) override;

	bool AcquireNextBackbuffer() override;

	IFramebuffer* GetCurrentBackbuffer(
		const AttachmentLoadOp colorAttachmentLoadOp,
		const AttachmentStoreOp colorAttachmentStoreOp,
		const AttachmentLoadOp depthStencilAttachmentLoadOp,
		const AttachmentStoreOp depthStencilAttachmentStoreOp) override;

	void Present() override;

	void OnWindowResize(const uint32_t width, const uint32_t height) override;

	bool IsTextureFormatSupported(const Format format) const override;

	bool IsFramebufferFormatSupported(const Format format) const override;

	Format GetPreferredDepthStencilFormat(
		const uint32_t usage, const bool depth, const bool stencil) const override;

	const Capabilities& GetCapabilities() const override { return m_Capabilities; }

	VkDevice GetVkDevice() { return m_Device; }

	VmaAllocator GetVMAAllocator() { return m_VMAAllocator; }

	void ScheduleObjectToDestroy(
		VkObjectType type, const void* handle, const VmaAllocation allocation)
	{
		ScheduleObjectToDestroy(type, reinterpret_cast<uint64_t>(handle), allocation);
	}

	void ScheduleObjectToDestroy(
		VkObjectType type, const uint64_t handle, const VmaAllocation allocation);

	void ScheduleTextureToDestroy(const CTexture::UID uid);

	void SetObjectName(VkObjectType type, const void* handle, const char* name)
	{
		SetObjectName(type, reinterpret_cast<uint64_t>(handle), name);
	}

	void SetObjectName(VkObjectType type, const uint64_t handle, const char* name);

	std::unique_ptr<CRingCommandContext> CreateRingCommandContext(const size_t size);

	const SAvailablePhysicalDevice& GetChoosenPhysicalDevice() const { return m_ChoosenDevice; }

	CRenderPassManager& GetRenderPassManager() { return *m_RenderPassManager; }

	CSamplerManager& GetSamplerManager() { return *m_SamplerManager; }

	CDescriptorManager& GetDescriptorManager() { return *m_DescriptorManager; }

private:
	CDevice();

	void RecreateSwapChain();
	bool IsSwapChainValid();
	void ProcessObjectToDestroyQueue(const bool ignoreFrameID = false);
	void ProcessTextureToDestroyQueue(const bool ignoreFrameID = false);

	std::string m_Name;
	std::string m_Version;
	std::string m_VendorID;
	std::string m_DriverInformation;
	std::vector<std::string> m_Extensions;
	std::vector<std::string> m_InstanceExtensions;
	std::vector<std::string> m_ValidationLayers;

	SAvailablePhysicalDevice m_ChoosenDevice{};
	std::vector<SAvailablePhysicalDevice> m_AvailablePhysicalDevices;

	Capabilities m_Capabilities{};

	VkInstance m_Instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

	SDL_Window* m_Window = nullptr;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;
	VmaAllocator m_VMAAllocator = VK_NULL_HANDLE;
	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	uint32_t m_GraphicsQueueFamilyIndex = std::numeric_limits<uint32_t>::max();

	std::unique_ptr<CSwapChain> m_SwapChain;

	uint32_t m_FrameID = 0;

	struct ObjectToDestroy
	{
		uint32_t frameID;
		VkObjectType type;
		uint64_t handle;
		VmaAllocation allocation;
	};
	std::queue<ObjectToDestroy> m_ObjectToDestroyQueue;
	std::queue<std::pair<uint32_t, CTexture::UID>> m_TextureToDestroyQueue;

	std::unique_ptr<CRenderPassManager> m_RenderPassManager;
	std::unique_ptr<CSamplerManager> m_SamplerManager;
	std::unique_ptr<CDescriptorManager> m_DescriptorManager;
	std::unique_ptr<CSubmitScheduler> m_SubmitScheduler;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_DEVICE
