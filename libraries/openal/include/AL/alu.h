#ifndef __alu_h_
#define __alu_h_

#ifdef _WIN32
#define ALUAPI
#define ALUAPIENTRY __cdecl

#define BUFFERSIZE 48000
#define FRACTIONBITS 14
#define FRACTIONMASK ((1L<<FRACTIONBITS)-1)
#define OUTPUTCHANNELS 2
#else  /* _WIN32 */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export on
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#define ALAPI
#define ALAPIENTRY
#define AL_CALLBACK
#endif /* _WIN32 */

#if defined(__MACH__) && defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alutypes.h>
#else
#include <AL/al.h>
#include <AL/alutypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
ALUAPI ALint	ALUAPIENTRY aluF2L(ALfloat value);
ALUAPI ALshort	ALUAPIENTRY aluF2S(ALfloat value);
ALUAPI ALvoid	ALUAPIENTRY aluCrossproduct(ALfloat *inVector1,ALfloat *inVector2,ALfloat *outVector);
ALUAPI ALfloat	ALUAPIENTRY aluDotproduct(ALfloat *inVector1,ALfloat *inVector2);
ALUAPI ALvoid	ALUAPIENTRY aluNormalize(ALfloat *inVector);
ALUAPI ALvoid	ALUAPIENTRY aluMatrixVector(ALfloat *vector,ALfloat matrix[3][3]);
ALUAPI ALvoid	ALUAPIENTRY aluCalculateSourceParameters(ALuint source,ALuint channels,ALfloat *drysend,ALfloat *wetsend,ALfloat *pitch);
ALUAPI ALvoid	ALUAPIENTRY aluMixData(ALvoid *context,ALvoid *buffer,ALsizei size,ALenum format);
ALUAPI ALvoid	ALUAPIENTRY aluSetReverb(ALvoid *Reverb,ALuint Environment);
ALUAPI ALvoid	ALUAPIENTRY aluReverb(ALvoid *Reverb,ALfloat Buffer[][2],ALsizei BufferSize);
#endif

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifdef __cplusplus
}
#endif

#endif /* __alu_h_ */

