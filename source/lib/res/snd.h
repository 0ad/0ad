#include "handle.h"


// prepare to enumerate all device names (this resets the list returned by
// snd_dev_next). return 0 on success, otherwise -1 (only if the requisite
// OpenAL extension isn't available). on failure, a "cannot change device"
// message should be presented to the user, and snd_dev_set need not be
// called; OpenAL will use its default device.
// may be called each time the device list is needed.
extern int snd_dev_prepare_enum();

// return the next device name, or 0 if all have been returned.
// do not call unless snd_dev_prepare_enum succeeded!
// not thread-safe! (static data from snd_dev_prepare_enum is used)
extern const char* snd_dev_next();

// tell OpenAL to use the specified device (0 for default) in future.
// if OpenAL hasn't been initialized yet, we only remember the device
// name, which will be set when oal_init is later called; otherwise,
// OpenAL is reinitialized to use the desired device.
// (this is to speed up the common case of retrieving a device name from
// config files and setting it; OpenAL doesn't have to be loaded until
// sounds are actually played).
// return 0 to indicate success, or the status returned while initializing
// OpenAL.
extern int snd_dev_set(const char* new_alc_dev_name);


extern int snd_set_max_src(int cap);



extern Handle snd_open(const char* fn, bool stream = false);
extern int snd_play(Handle hs);
extern int snd_free(Handle& hs);


extern int snd_set_pos(Handle hs, float x, float y, float z, bool relative = false);

extern int snd_set_gain(Handle hs, float gain);

extern int snd_set_loop(Handle hs, bool loop);






extern int snd_update(float lx, float ly, float lz);