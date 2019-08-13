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

#include "TextureConverter.h"

#include "lib/regex.h"
#include "lib/timer.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/tex/tex.h"
#include "maths/MD5.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Profiler2.h"
#include "ps/XML/Xeromyces.h"

#if CONFIG2_NVTT

#include "nvtt/nvtt.h"

/**
 * Output handler to collect NVTT's output into a simplistic buffer.
 * WARNING: Used in the worker thread - must be thread-safe.
 */
struct BufferOutputHandler : public nvtt::OutputHandler
{
	std::vector<u8> buffer;

	virtual void beginImage(int UNUSED(size), int UNUSED(width), int UNUSED(height), int UNUSED(depth), int UNUSED(face), int UNUSED(miplevel))
	{
	}

	virtual bool writeData(const void* data, int size)
	{
		size_t off = buffer.size();
		buffer.resize(off + size);
		memcpy(&buffer[off], data, size);
		return true;
	}
};

/**
 * Request for worker thread to process.
 */
struct CTextureConverter::ConversionRequest
{
	VfsPath dest;
	CTexturePtr texture;
	nvtt::InputOptions inputOptions;
	nvtt::CompressionOptions compressionOptions;
	nvtt::OutputOptions outputOptions;
	bool isDXT1a; // see comment in RunThread
	bool is8bpp;
};

/**
 * Result from worker thread.
 */
struct CTextureConverter::ConversionResult
{
	VfsPath dest;
	CTexturePtr texture;
	BufferOutputHandler output;
	bool ret; // true if the conversion succeeded
};

#endif // CONFIG2_NVTT

void CTextureConverter::Settings::Hash(MD5& hash)
{
	hash.Update((const u8*)&format, sizeof(format));
	hash.Update((const u8*)&mipmap, sizeof(mipmap));
	hash.Update((const u8*)&normal, sizeof(normal));
	hash.Update((const u8*)&alpha, sizeof(alpha));
	hash.Update((const u8*)&filter, sizeof(filter));
	hash.Update((const u8*)&kaiserWidth, sizeof(kaiserWidth));
	hash.Update((const u8*)&kaiserAlpha, sizeof(kaiserAlpha));
	hash.Update((const u8*)&kaiserStretch, sizeof(kaiserStretch));
}

CTextureConverter::SettingsFile* CTextureConverter::LoadSettings(const VfsPath& path) const
{
	CXeromyces XeroFile;
	if (XeroFile.Load(m_VFS, path, "texture") != PSRETURN_OK)
		return NULL;

	// Define all the elements used in the XML file
	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(textures);
	EL(file);
	AT(pattern);
	AT(format);
	AT(mipmap);
	AT(normal);
	AT(alpha);
	AT(filter);
	AT(kaiserwidth);
	AT(kaiseralpha);
	AT(kaiserstretch);
	#undef AT
	#undef EL

	XMBElement root = XeroFile.GetRoot();

	if (root.GetNodeName() != el_textures)
	{
		LOGERROR("Invalid texture settings file \"%s\" (unrecognised root element)", path.string8());
		return NULL;
	}

	std::unique_ptr<SettingsFile> settings(new SettingsFile());

	XERO_ITER_EL(root, child)
	{
		if (child.GetNodeName() == el_file)
		{
			Match p;

			XERO_ITER_ATTR(child, attr)
			{
				if (attr.Name == at_pattern)
				{
					p.pattern = attr.Value.FromUTF8();
				}
				else if (attr.Name == at_format)
				{
					CStr v(attr.Value);
					if (v == "dxt1")
						p.settings.format = FMT_DXT1;
					else if (v == "dxt3")
						p.settings.format = FMT_DXT3;
					else if (v == "dxt5")
						p.settings.format = FMT_DXT5;
					else if (v == "rgba")
						p.settings.format = FMT_RGBA;
					else if (v == "alpha")
						p.settings.format = FMT_ALPHA;
					else
						LOGERROR("Invalid attribute value <file format='%s'>", v.c_str());
				}
				else if (attr.Name == at_mipmap)
				{
					CStr v(attr.Value);
					if (v == "true")
						p.settings.mipmap = MIP_TRUE;
					else if (v == "false")
						p.settings.mipmap = MIP_FALSE;
					else
						LOGERROR("Invalid attribute value <file mipmap='%s'>", v.c_str());
				}
				else if (attr.Name == at_normal)
				{
					CStr v(attr.Value);
					if (v == "true")
						p.settings.normal = NORMAL_TRUE;
					else if (v == "false")
						p.settings.normal = NORMAL_FALSE;
					else
						LOGERROR("Invalid attribute value <file normal='%s'>", v.c_str());
				}
				else if (attr.Name == at_alpha)
				{
					CStr v(attr.Value);
					if (v == "none")
						p.settings.alpha = ALPHA_NONE;
					else if (v == "player")
						p.settings.alpha = ALPHA_PLAYER;
					else if (v == "transparency")
						p.settings.alpha = ALPHA_TRANSPARENCY;
					else
						LOGERROR("Invalid attribute value <file alpha='%s'>", v.c_str());
				}
				else if (attr.Name == at_filter)
				{
					CStr v(attr.Value);
					if (v == "box")
						p.settings.filter = FILTER_BOX;
					else if (v == "triangle")
						p.settings.filter = FILTER_TRIANGLE;
					else if (v == "kaiser")
						p.settings.filter = FILTER_KAISER;
					else
						LOGERROR("Invalid attribute value <file filter='%s'>", v.c_str());
				}
				else if (attr.Name == at_kaiserwidth)
				{
					p.settings.kaiserWidth = CStr(attr.Value).ToFloat();
				}
				else if (attr.Name == at_kaiseralpha)
				{
					p.settings.kaiserAlpha = CStr(attr.Value).ToFloat();
				}
				else if (attr.Name == at_kaiserstretch)
				{
					p.settings.kaiserStretch = CStr(attr.Value).ToFloat();
				}
				else
				{
					LOGERROR("Invalid attribute name <file %s='...'>", XeroFile.GetAttributeString(attr.Name).c_str());
				}
			}

			settings->patterns.push_back(p);
		}
	}

	return settings.release();
}

CTextureConverter::Settings CTextureConverter::ComputeSettings(const std::wstring& filename, const std::vector<SettingsFile*>& settingsFiles) const
{
	// Set sensible defaults
	Settings settings;
	settings.format = FMT_DXT1;
	settings.mipmap = MIP_TRUE;
	settings.normal = NORMAL_FALSE;
	settings.alpha = ALPHA_NONE;
	settings.filter = FILTER_BOX;
	settings.kaiserWidth = 3.f;
	settings.kaiserAlpha = 4.f;
	settings.kaiserStretch = 1.f;

	for (size_t i = 0; i < settingsFiles.size(); ++i)
	{
		for (size_t j = 0; j < settingsFiles[i]->patterns.size(); ++j)
		{
			Match p = settingsFiles[i]->patterns[j];

			// Check that the pattern matches the texture file
			if (!match_wildcard(filename.c_str(), p.pattern.c_str()))
				continue;

			if (p.settings.format != FMT_UNSPECIFIED)
				settings.format = p.settings.format;

			if (p.settings.mipmap != MIP_UNSPECIFIED)
				settings.mipmap = p.settings.mipmap;

			if (p.settings.normal != NORMAL_UNSPECIFIED)
				settings.normal = p.settings.normal;

			if (p.settings.alpha != ALPHA_UNSPECIFIED)
				settings.alpha = p.settings.alpha;

			if (p.settings.filter != FILTER_UNSPECIFIED)
				settings.filter = p.settings.filter;

			if (p.settings.kaiserWidth != -1.f)
				settings.kaiserWidth = p.settings.kaiserWidth;

			if (p.settings.kaiserAlpha != -1.f)
				settings.kaiserAlpha = p.settings.kaiserAlpha;

			if (p.settings.kaiserStretch != -1.f)
				settings.kaiserStretch = p.settings.kaiserStretch;
		}
	}

	return settings;
}

CTextureConverter::CTextureConverter(PIVFS vfs, bool highQuality) :
	m_VFS(vfs), m_HighQuality(highQuality), m_Shutdown(false)
{
	// Verify that we are running with at least the version we were compiled with,
	// to avoid bugs caused by ABI changes
#if CONFIG2_NVTT
	ENSURE(nvtt::version() >= NVTT_VERSION);
#endif

	// Set up the worker thread:

	// Use SDL semaphores since OS X doesn't implement sem_init
	m_WorkerSem = SDL_CreateSemaphore(0);
	ENSURE(m_WorkerSem);

	m_WorkerThread = std::thread(RunThread, this);

	// Maybe we should share some centralised pool of worker threads?
	// For now we'll just stick with a single thread for this specific use.
}

CTextureConverter::~CTextureConverter()
{
	// Tell the thread to shut down
	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		m_Shutdown = true;
	}

	// Wake it up so it sees the notification
	SDL_SemPost(m_WorkerSem);

	// Wait for it to shut down cleanly
	m_WorkerThread.join();

	// Clean up resources
	SDL_DestroySemaphore(m_WorkerSem);
}

bool CTextureConverter::ConvertTexture(const CTexturePtr& texture, const VfsPath& src, const VfsPath& dest, const Settings& settings)
{
	shared_ptr<u8> file;
	size_t fileSize;
	if (m_VFS->LoadFile(src, file, fileSize) < 0)
	{
		LOGERROR("Failed to load texture \"%s\"", src.string8());
		return false;
	}

	Tex tex;
	if (tex.decode(file, fileSize) < 0)
	{
		LOGERROR("Failed to decode texture \"%s\"", src.string8());
		return false;
	}

	// Check whether there's any alpha channel
	bool hasAlpha = ((tex.m_Flags & TEX_ALPHA) != 0);

	if (settings.format == FMT_ALPHA)
	{
		// Convert to uncompressed 8-bit with no mipmaps
		if (tex.transform_to((tex.m_Flags | TEX_GREY) & ~(TEX_DXT | TEX_MIPMAPS | TEX_ALPHA)) < 0)
		{
			LOGERROR("Failed to transform texture \"%s\"", src.string8());
			return false;
		}
	}
	else
	{
		// Convert to uncompressed BGRA with no mipmaps
		if (tex.transform_to((tex.m_Flags | TEX_BGR | TEX_ALPHA) & ~(TEX_DXT | TEX_MIPMAPS)) < 0)
		{
			LOGERROR("Failed to transform texture \"%s\"", src.string8());
			return false;
		}
	}

	// Check if the texture has all alpha=255, so we can automatically
	// switch from DXT3/DXT5 to DXT1 with no loss
	if (hasAlpha)
	{
		hasAlpha = false;
		u8* data = tex.get_data();
		for (size_t i = 0; i < tex.m_Width * tex.m_Height; ++i)
		{
			if (data[i*4+3] != 0xFF)
			{
				hasAlpha = true;
				break;
			}
		}
	}

#if CONFIG2_NVTT

	shared_ptr<ConversionRequest> request(new ConversionRequest);
	request->dest = dest;
	request->texture = texture;

	// Apply the chosen settings:

	request->inputOptions.setMipmapGeneration(settings.mipmap == MIP_TRUE);

	if (settings.alpha == ALPHA_TRANSPARENCY)
		request->inputOptions.setAlphaMode(nvtt::AlphaMode_Transparency);
	else
		request->inputOptions.setAlphaMode(nvtt::AlphaMode_None);

	request->isDXT1a = false;
	request->is8bpp = false;

	if (settings.format == FMT_RGBA)
	{
		request->compressionOptions.setFormat(nvtt::Format_RGBA);
		// Change the default component order (see tex_dds.cpp decode_pf)
		request->compressionOptions.setPixelFormat(32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000u);
	}
	else if (settings.format == FMT_ALPHA)
	{
		request->compressionOptions.setFormat(nvtt::Format_RGBA);
		request->compressionOptions.setPixelFormat(8, 0x00, 0x00, 0x00, 0xFF);
		request->is8bpp = true;
	}
	else if (!hasAlpha)
	{
		// if no alpha channel then there's no point using DXT3 or DXT5
		request->compressionOptions.setFormat(nvtt::Format_DXT1);
	}
	else if (settings.format == FMT_DXT1)
	{
		request->compressionOptions.setFormat(nvtt::Format_DXT1a);
		request->isDXT1a = true;
	}
	else if (settings.format == FMT_DXT3)
	{
		request->compressionOptions.setFormat(nvtt::Format_DXT3);
	}
	else if (settings.format == FMT_DXT5)
	{
		request->compressionOptions.setFormat(nvtt::Format_DXT5);
	}

	if (settings.filter == FILTER_BOX)
		request->inputOptions.setMipmapFilter(nvtt::MipmapFilter_Box);
	else if (settings.filter == FILTER_TRIANGLE)
		request->inputOptions.setMipmapFilter(nvtt::MipmapFilter_Triangle);
	else if (settings.filter == FILTER_KAISER)
		request->inputOptions.setMipmapFilter(nvtt::MipmapFilter_Kaiser);

	if (settings.normal == NORMAL_TRUE)
		request->inputOptions.setNormalMap(true);

	request->inputOptions.setKaiserParameters(settings.kaiserWidth, settings.kaiserAlpha, settings.kaiserStretch);

	request->inputOptions.setWrapMode(nvtt::WrapMode_Mirror); // TODO: should this be configurable?

	request->compressionOptions.setQuality(m_HighQuality ? nvtt::Quality_Production : nvtt::Quality_Fastest);

	// TODO: normal maps, gamma, etc

	// Load the texture data
	request->inputOptions.setTextureLayout(nvtt::TextureType_2D, tex.m_Width, tex.m_Height);
	if (tex.m_Bpp == 32)
	{
		request->inputOptions.setMipmapData(tex.get_data(), tex.m_Width, tex.m_Height);
	}
	else // bpp == 8
	{
		// NVTT requires 32-bit input data, so convert
		const u8* input = tex.get_data();
		u8* rgba = new u8[tex.m_Width * tex.m_Height * 4];
		u8* p = rgba;
		for (size_t i = 0; i < tex.m_Width * tex.m_Height; i++)
		{
			p[0] = p[1] = p[2] = p[3] = *input++;
			p += 4;
		}
		request->inputOptions.setMipmapData(rgba, tex.m_Width, tex.m_Height);
		delete[] rgba;
	}

	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		m_RequestQueue.push_back(request);
	}

	// Wake up the worker thread
	SDL_SemPost(m_WorkerSem);

	return true;

#else
	LOGERROR("Failed to convert texture \"%s\" (NVTT not available)", src.string8());
	return false;
#endif
}

bool CTextureConverter::Poll(CTexturePtr& texture, VfsPath& dest, bool& ok)
{
#if CONFIG2_NVTT
	shared_ptr<ConversionResult> result;

	// Grab the first result (if any)
	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		if (!m_ResultQueue.empty())
		{
			result = m_ResultQueue.front();
			m_ResultQueue.pop_front();
		}
	}

	if (!result)
	{
		// no work to do
		return false;
	}

	if (!result->ret)
	{
		// conversion had failed
		ok = false;
		return true;
	}

	// Move output into a correctly-aligned buffer
	size_t size = result->output.buffer.size();
	shared_ptr<u8> file;
	AllocateAligned(file, size, maxSectorSize);
	memcpy(file.get(), &result->output.buffer[0], size);
	if (m_VFS->CreateFile(result->dest, file, size) < 0)
	{
		// error writing file
		ok = false;
		return true;
	}

	// Succeeded in converting texture
	texture = result->texture;
	dest = result->dest;
	ok = true;
	return true;

#else // #if CONFIG2_NVTT
	return false;
#endif
}

bool CTextureConverter::IsBusy()
{
	std::lock_guard<std::mutex> lock(m_WorkerMutex);
	bool busy = !m_RequestQueue.empty();

	return busy;
}

void CTextureConverter::RunThread(CTextureConverter* textureConverter)
{
	debug_SetThreadName("TextureConverter");
	g_Profiler2.RegisterCurrentThread("texconv");

#if CONFIG2_NVTT

	// Wait until the main thread wakes us up
	while (SDL_SemWait(textureConverter->m_WorkerSem) == 0)
	{
		g_Profiler2.RecordSyncMarker();
		PROFILE2_EVENT("wakeup");

		shared_ptr<ConversionRequest> request;

		{
			std::lock_guard<std::mutex> lock(textureConverter->m_WorkerMutex);
			if (textureConverter->m_Shutdown)
				break;
			// If we weren't woken up for shutdown, we must have been woken up for
			// a new request, so grab it from the queue
			request = textureConverter->m_RequestQueue.front();
			textureConverter->m_RequestQueue.pop_front();
		}

		// Set up the result object
		shared_ptr<ConversionResult> result(new ConversionResult());
		result->dest = request->dest;
		result->texture = request->texture;

		request->outputOptions.setOutputHandler(&result->output);

//		TIMER(L"TextureConverter compress");

		{
			PROFILE2("compress");

			// Perform the compression
			nvtt::Compressor compressor;
			result->ret = compressor.process(request->inputOptions, request->compressionOptions, request->outputOptions);
		}

		// Ugly hack: NVTT 2.0 doesn't set DDPF_ALPHAPIXELS for DXT1a, so we can't
		// distinguish it from DXT1. (It's fixed in trunk by
		// http://code.google.com/p/nvidia-texture-tools/source/detail?r=924&path=/trunk).
		// Rather than using a trunk NVTT (unstable, makes packaging harder)
		// or patching our copy (makes packaging harder), we'll just manually
		// set the flag here.
		if (request->isDXT1a && result->ret && result->output.buffer.size() > 80)
			result->output.buffer[80] |= 1; // DDPF_ALPHAPIXELS in DDS_PIXELFORMAT.dwFlags
		// Ugly hack: NVTT always sets DDPF_RGB, even if we're trying to output 8-bit
		// alpha-only DDS with no RGB components. Unset that flag.
		if (request->is8bpp)
			result->output.buffer[80] &= ~0x40; // DDPF_RGB in DDS_PIXELFORMAT.dwFlags

		// Push the result onto the queue
		std::lock_guard<std::mutex> lock(textureConverter->m_WorkerMutex);
		textureConverter->m_ResultQueue.push_back(result);
	}

#endif
}
