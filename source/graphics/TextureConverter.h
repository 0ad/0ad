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

#ifndef INCLUDED_TEXTURECONVERTER
#define INCLUDED_TEXTURECONVERTER

#include "lib/file/vfs/vfs.h"

#include "TextureManager.h"

#include <condition_variable>
#include <mutex>
#include <thread>

class MD5;

/**
 * Texture conversion helper class.
 * Provides an asynchronous API to convert input image files into compressed DDS,
 * given various conversion settings.
 * (The (potentially very slow) compression is a performed in a background thread,
 * so the game can remain responsive).
 * Also provides an API to load conversion settings from XML files.
 *
 * XML files are of the form:
 * @code
 * <Textures>
 *   <File pattern="*" format="dxt5" mipmap="false" alpha="transparency"/>
 *   <File pattern="button_wood.*" format="rgba"/>
 * </Entity>
 * @endcode
 *
 * 'pattern' is a wildcard expression, matching on filenames.
 * All other attributes are optional. Later elements override attributes from
 * earlier elements.
 *
 * 'format' is 'dxt1', 'dxt3', 'dxt5' or 'rgba'.
 *
 * 'mipmap' is 'true' or 'false'.
 *
 * 'normal' is 'true' or 'false'.
 *
 * 'alpha' is 'transparency' or 'player' (it determines whether the color value of
 * 0-alpha pixels is significant or not).
 *
 * 'filter' is 'box', 'triangle' or 'kaiser'.
 *
 * 'kaiserwidth', 'kaiseralpha', 'kaiserstretch' are floats
 * (see http://code.google.com/p/nvidia-texture-tools/wiki/ApiDocumentation#Mipmap_Generation)
 */
class CTextureConverter
{
public:
	enum EFormat
	{
		FMT_UNSPECIFIED,
		FMT_DXT1,
		FMT_DXT3,
		FMT_DXT5,
		FMT_RGBA,
		FMT_ALPHA
	};

	enum EMipmap
	{
		MIP_UNSPECIFIED,
		MIP_TRUE,
		MIP_FALSE
	};

	enum ENormalMap
	{
		NORMAL_UNSPECIFIED,
		NORMAL_TRUE,
		NORMAL_FALSE
	};

	enum EAlpha
	{
		ALPHA_UNSPECIFIED,
		ALPHA_NONE,
		ALPHA_PLAYER,
		ALPHA_TRANSPARENCY
	};

	enum EFilter
	{
		FILTER_UNSPECIFIED,
		FILTER_BOX,
		FILTER_TRIANGLE,
		FILTER_KAISER
	};

	/**
	 * Texture conversion settings.
	 */
	struct Settings
	{
		Settings() :
			format(FMT_UNSPECIFIED), mipmap(MIP_UNSPECIFIED), normal(NORMAL_UNSPECIFIED),
			alpha(ALPHA_UNSPECIFIED), filter(FILTER_UNSPECIFIED),
			kaiserWidth(-1.f), kaiserAlpha(-1.f), kaiserStretch(-1.f)
		{
		}

		/**
		 * Append this object's state to the given hash.
		 */
		void Hash(MD5& hash);

		EFormat format;
		EMipmap mipmap;
		ENormalMap normal;
		EAlpha alpha;
		EFilter filter;
		float kaiserWidth;
		float kaiserAlpha;
		float kaiserStretch;
	};

	/**
	 * Representation of \<File\> line from settings XML file.
	 */
	struct Match
	{
		std::wstring pattern;
		Settings settings;
	};

	/**
	 * Representation of settings XML file.
	 */
	struct SettingsFile
	{
		std::vector<Match> patterns;
	};

	/**
	 * Construct texture converter, for use with files in the given vfs.
	 */
	CTextureConverter(PIVFS vfs, bool highQuality);

	/**
	 * Destroy texture converter and wait to shut down worker thread.
	 * This might take a long time (maybe seconds) if the worker is busy
	 * processing a texture.
	 */
	~CTextureConverter();

	/**
	 * Load a texture conversion settings XML file.
	 * Returns NULL on failure.
	 */
	SettingsFile* LoadSettings(const VfsPath& path) const;

	/**
	 * Match a sequence of settings files against a given texture filename,
	 * and return the resulting settings.
	 * Later entries in settingsFiles override earlier entries.
	 */
	Settings ComputeSettings(const std::wstring& filename, const std::vector<SettingsFile*>& settingsFiles) const;

	/**
	 * Begin converting a texture, using the given settings.
	 * This will load src and return false on failure.
	 * Otherwise it will return true and start an asynchronous conversion request,
	 * whose result will be returned from Poll() (with the texture and dest passed
	 * into this function).
	 */
	bool ConvertTexture(const CTexturePtr& texture, const VfsPath& src, const VfsPath& dest, const Settings& settings);

	/**
	 * Returns the result of a successful ConvertTexture call.
	 * If no result is available yet, returns false.
	 * Otherwise, if the conversion succeeded, it sets ok to true and sets
	 * texture and dest to the corresponding values passed into ConvertTexture(),
	 * then returns true.
	 * If the conversion failed, it sets ok to false and doesn't touch the other arguments,
	 * then returns true.
	 */
	bool Poll(CTexturePtr& texture, VfsPath& dest, bool& ok);

	/**
	 * Returns whether there is currently a queued request from ConvertTexture().
	 * (Note this may return false while the worker thread is still converting the last texture.)
	 */
	bool IsBusy();

private:
	static void RunThread(CTextureConverter* data);

	PIVFS m_VFS;
	bool m_HighQuality;

	std::thread m_WorkerThread;
	std::mutex m_WorkerMutex;
	std::condition_variable m_WorkerCV;

	struct ConversionRequest;
	struct ConversionResult;

	std::deque<shared_ptr<ConversionRequest> > m_RequestQueue; // protected by m_WorkerMutex
	std::deque<shared_ptr<ConversionResult> > m_ResultQueue; // protected by m_WorkerMutex
	bool m_Shutdown; // protected by m_WorkerMutex
};

#endif // INCLUDED_TEXTURECONVERTER
