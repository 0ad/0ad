/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * OpenAL sound engine. handles sound I/O, buffer suballocation and
 * voice management/prioritization.
 */

#ifndef INCLUDED_SND_MGR
#define INCLUDED_SND_MGR

#include "lib/res/handle.h"
#include "lib/file/vfs/vfs.h"

/**

overview
--------

this module provides a moderately high-level sound interface. basic usage
is opening a sound and requesting it be played; it is closed automatically
when playback has finished (fire and forget).
any number of sound play requests may be issued; the most 'important' ones
are actually played (necessary due to limited hardware mixing capacity).
3d positional sounds (heard to emanate from a given spot) are supported.
active sound instances are referenced by Handles, so changing volume etc.
during playback is possible (useful for fadeout).


sound setup
-----------

OpenAL provides portable access to the underlying sound hardware, and
falls back to software mixing if no acceleration is provided.
we allow the user to specify the device to use (in case the default
has problems) and maximum number of sources (to reduce mixing cost).


performance
-----------
much effort has been invested in efficiency: all sound data is cached,
so every open() after the first is effectively free. large sound files are
streamed from disk to reduce load time and memory usage. hardware mixing
resources are suballocated to avoid delays when starting to play.
therefore, the user can confidently fire off hundreds of sound requests.
finally, lengthy initialization steps are delayed until the sound engine
is actually needed (i.e. upon first open()). perceived startup time is
therefore reduced - the user sees e.g. our main menu earlier.


terminology
-----------

"hardware voice" refers to mixing resources on the DSP. strictly
  speaking, we mean 'OpenAL source', but this term is more clear.
  voice ~= source, unless expensive effects (e.g. EAX) are enabled.
  note: software mixing usually doesn't have a fixed 'source' cap.
"gain" is quantified volume. 1 is unattenuated, 0.5 corresponds to -6 dB,
  and 0 is silence. this can be set per-source as well as globally.
"position" of a sound is within the app's coordinate system,
  the orientation of which is passed to snd_update.
"importance" of a sound derives from the app-assigned priority
  (e.g. voiceover must not be skipped in favor of seagulls) and
  distance from the listener. it's calculated by our prioritizer.
"virtual source" denotes a sound play request issued by the app.
  this is in contrast to an actual AL source, which will be mixed
  into the output channel. the most important VSrc receive an al_src.
"sound instances" store playback parameters (e.g. position), and
  reference the (centrally cached) "sound data" that will be played.

**/


//
// device enumeration
//

/**
 * prepare to enumerate all device names (this resets the list returned by
 * snd_dev_next).
 * may be called each time the device list is needed.
 *
 * @return LibError; fails iff the requisite OpenAL extension isn't available.
 * in that case, a "cannot enum device" message should be displayed, but
 * snd_dev_set need not be called; OpenAL will use its default device.
 **/
extern LibError snd_dev_prepare_enum();

/**
 * get next device name in list.
 *
 * do not call unless snd_dev_prepare_enum succeeded!
 * not thread-safe! (static data from snd_dev_prepare_enum is used)
 *
 * @return device name string, or 0 if all have been returned.
 **/
extern const char* snd_dev_next();


//
// sound system setup
//

/**
 * tell OpenAL to use the specified device in future.
 *
 * @param alc_new_dev_name device name string. if 0, revert to
 * OpenAL's default choice, which will also be used if
 * this routine is never called.
 * the device name is typically taken from a config file at init-time;
 * the snd_dev* enumeration routines above are used to present a list
 * of choices to the user in the options screen.
 *
 * if OpenAL hasn't yet been initialized (i.e. no sounds have been opened),
 *   this just stores the device name for use when init does occur.
 *   note: we can't check now if it's invalid (if so, init will fail).
 * otherwise, we shut OpenAL down (thereby stopping all sounds) and
 * re-initialize with the new device. that's fairly time-consuming,
 * so preferably call this routine before sounds are loaded.
 *
 * @return LibError (the status returned by OpenAL re-init)
 **/
extern LibError snd_dev_set(const char* alc_new_dev_name);

/**
 * set maximum number of voices to play simultaneously;
 * this can be used to reduce mixing cost on low-end systems.
 *
 * @param cap maximum number of voices. ignored if higher than
 * an implementation-defined limit anyway.
 * @return LibError
 **/
extern LibError snd_set_max_voices(size_t cap);

/**
 * set amplitude modifier, which is effectively applied to all sounds.
 * this is akin to a global "volume" control.
 *
 * @param gain amplitude modifier. must be non-negative;
 * 1 -> unattenuated, 0.5 -> -6 dB, 0 -> silence.
 * @return LibError
 **/
extern LibError snd_set_master_gain(float gain);


//
// sound instance
//

/**
 * open and return a handle to a sound instance.
 * this loads the sound data and makes it ready for other snd_* APIs.
 *
 * @param pathname. if a text file (extension ".txt"), it is
 *   assumed to be a definition file containing the sound file name and
 *   its gain (0.0 .. 1.0).
 * otherwise, it is taken to be the sound file name and
 *   gain is set to the default of 1.0 (no attenuation).
 *
 * @param is_stream (default false) forces the sound to be opened as a
 * stream: opening is faster, it won't be kept in memory, but
 * only one instance can be open at a time.
 * @return Handle or LibError
 **/
extern Handle snd_open(const PIVFS& vfs, const VfsPath& name, bool stream = false);

/**
 * close the sound instance. if it was playing, it will be stopped.
 *
 * rationale: sounds are already closed automatically when done playing;
 * this API is provided for completeness only.
 *
 * @param hs Handle to sound instance. zeroed afterwards.
 * @return LibError
 **/
extern LibError snd_free(Handle& hs);

/**
 * start playing the sound.
 *
 * Notes:
 * <UL>
 *   <LI> once done playing, the sound is automatically closed (allows
 *        fire-and-forget play code).
 *   <LI> if no hardware voice is available, this sound may not be
 *        played at all, or in the case of looped sounds, start later.
 * </UL>
 *
 * @param priority (min 0 .. max 1, default 0) indicates which sounds are
 * considered more important (i.e. will override others when no hardware
 * voices are available). the static priority is attenuated by
 * distance to the listener; see snd_update.
 *
 * @return LibError
 **/
extern LibError snd_play(Handle hs, float priority = 0.0f);

/**
 * change 3d position of the sound source.
 *
 * may be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * @param relative treat (x,y,z) as relative to the listener;
 * if false (the default), it is the position in world coordinates.
 * @return LibError
 **/
extern LibError snd_set_pos(Handle hs, float x, float y, float z, bool relative = false);

/**
 * change gain (amplitude modifier) of the sound source.
 *
 * should not be called during a fade (see note in implementation);
 * fails with invalid handle return if the sound has already been
 * closed (e.g. it never played).
 *
 * @param gain amplitude modifier. must be non-negative;
 * 1 -> unattenuated, 0.5 -> -6 dB, 0 -> silence.
 * @return LibError
 **/
extern LibError snd_set_gain(Handle hs, float gain);

/**
 * change pitch shift of the sound source.
 *
 * may be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * @param pitch shift: 1.0 means no change; each doubling/halving equals a
 * pitch shift of +/-12 semitones (one octave). zero is invalid.
 * @return LibError
 **/
extern LibError snd_set_pitch(Handle hs, float pitch);

/**
 * enable/disable looping on the sound source.
 * used to implement variable-length sounds (e.g. while building).
 *
 * may be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * Notes:
 * <UL>
 *   <LI> looping sounds are not discarded if they cannot be played for
 *        lack of a hardware voice at the moment play was requested.
 *   <LI> once looping is again disabled and the sound has reached its end,
 *        the sound instance is freed automatically (as if never looped).
 * </UL>
 * @return LibError
 **/
extern LibError snd_set_loop(Handle hs, bool loop);

/// types of fade in/out operations
enum FadeType
{
	FT_NONE,		/// currently no fade in progres
	FT_LINEAR,		/// f(t) = t
	FT_EXPONENTIAL,	/// f(t) = t**3
	FT_S_CURVE,		/// cosine curve

	FT_ABORT		/// abort and mark pending fade as complete
};

/**
 * fade the sound source in or out over time.
 *
 * may be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * gain starts at <initial_gain> (immediately) and is moved toward
 * <final_gain> over <length> seconds.
 * @param type of fade curve: linear, exponential or S-curve.
 * for guidance on which to use, see
 * http://www.transom.org/tools/editing_mixing/200309.stupidfadetricks.html
 * you can also pass FT_ABORT to stop fading (if in progress) and
 * set gain to the current <final_gain> parameter.
 * special cases:
 * - if <initial_gain> < 0 (an otherwise illegal value), the sound's
 *   current gain is used as the start value (useful for fading out).
 * - if <final_gain> is 0, the sound is freed when the fade completes or
 *   is aborted, thus allowing fire-and-forget fadeouts. no cases are
 *   foreseen where this is undesirable, and it is easier to implement
 *   than an extra set-free-after-fade-flag function.
 *
 * note that this function doesn't busy-wait until the fade is complete;
 * any number of fades may be active at a time (allows cross-fading).
 * each snd_update calculates a new gain value for all pending fades.
 * it is safe to start another fade on the same sound source while
 * one is already in progress; the old one will be discarded.
 * @return LibError
 **/
extern LibError snd_fade(Handle hvs, float initial_gain, float final_gain,
	float length, FadeType type);


//
// sound engine
//

/**
 * (temporarily) disable all sound output.
 *
 * because it causes future snd_open calls to immediately abort before they
 * demand-initialize OpenAL, startup is sped up considerably (500..1000ms).
 * therefore, this must be called before the first snd_open to have
 * any effect; otherwise, the cat will already be out of the bag and
 * we raise a warning.
 *
 * rationale: this is a quick'n dirty way of speeding up startup during
 * development without having to change the game's sound code.
 *
 * can later be called to reactivate sound; all settings ever changed
 * will be applied and subsequent sound load / play requests will work.
 * @return LibError
 **/
extern LibError snd_disable(bool disabled);

/**
 * perform housekeeping (e.g. streaming); call once a frame.
 *
 * all parameters are expressed in world coordinates. they can all be NULL
 * to avoid updating the listener data; this is useful when the game world
 * has not been initialized yet.
 * @param pos listener's position
 * @param dir listener view direction
 * @param up listener's local up vector
 * @return LibError
 **/
extern LibError snd_update(const float* pos, const float* dir, const float* up);

/** added by GF
 * find out if a sound is still playing
 *
 * @param hvs - handle to the snd to check
 * @return bool true if playing

**/
extern bool is_playing(Handle hvs);


/**
 * free all resources and shut down the sound system.
 * call before h_mgr_shutdown.
 **/
extern void snd_shutdown();

#endif	// #ifndef INCLUDED_SND_MGR
