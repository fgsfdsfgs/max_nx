/* openal.c -- OpenAL hooks and patches
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

#include "../util.h"
#include "../so_util.h"

ALCcontext *alcCreateContextHook(ALCdevice *dev, const ALCint *unused) {
  // override 22050hz with 44100hz in case someone wants high quality sounds
  const ALCint attr[] = { ALC_FREQUENCY, 44100, 0 };
  return alcCreateContext(dev, attr);
}

void patch_openal(void) {
  // used for openal
  hook_arm64(so_find_addr("InitializeCriticalSection"), (uintptr_t)ret0);
  // openal API
  hook_arm64(so_find_addr("alAuxiliaryEffectSlotf"), (uintptr_t)alAuxiliaryEffectSlotf);
  hook_arm64(so_find_addr("alAuxiliaryEffectSlotfv"), (uintptr_t)alAuxiliaryEffectSlotfv);
  hook_arm64(so_find_addr("alAuxiliaryEffectSloti"), (uintptr_t)alAuxiliaryEffectSloti);
  hook_arm64(so_find_addr("alAuxiliaryEffectSlotiv"), (uintptr_t)alAuxiliaryEffectSlotiv);
  hook_arm64(so_find_addr("alBuffer3f"), (uintptr_t)alBuffer3f);
  hook_arm64(so_find_addr("alBuffer3i"), (uintptr_t)alBuffer3i);
  hook_arm64(so_find_addr("alBufferData"), (uintptr_t)alBufferData);
  hook_arm64(so_find_addr("alBufferf"), (uintptr_t)alBufferf);
  hook_arm64(so_find_addr("alBufferfv"), (uintptr_t)alBufferfv);
  hook_arm64(so_find_addr("alBufferi"), (uintptr_t)alBufferi);
  hook_arm64(so_find_addr("alBufferiv"), (uintptr_t)alBufferiv);
  hook_arm64(so_find_addr("alDeleteAuxiliaryEffectSlots"), (uintptr_t)alDeleteAuxiliaryEffectSlots);
  hook_arm64(so_find_addr("alDeleteBuffers"), (uintptr_t)alDeleteBuffers);
  hook_arm64(so_find_addr("alDeleteEffects"), (uintptr_t)alDeleteEffects);
  hook_arm64(so_find_addr("alDeleteFilters"), (uintptr_t)alDeleteFilters);
  hook_arm64(so_find_addr("alDeleteSources"), (uintptr_t)alDeleteSources);
  hook_arm64(so_find_addr("alDisable"), (uintptr_t)alDisable);
  hook_arm64(so_find_addr("alDistanceModel"), (uintptr_t)alDistanceModel);
  hook_arm64(so_find_addr("alDopplerFactor"), (uintptr_t)alDopplerFactor);
  hook_arm64(so_find_addr("alDopplerVelocity"), (uintptr_t)alDopplerVelocity);
  hook_arm64(so_find_addr("alEffectf"), (uintptr_t)alEffectf);
  hook_arm64(so_find_addr("alEffectfv"), (uintptr_t)alEffectfv);
  hook_arm64(so_find_addr("alEffecti"), (uintptr_t)alEffecti);
  hook_arm64(so_find_addr("alEffectiv"), (uintptr_t)alEffectiv);
  hook_arm64(so_find_addr("alEnable"), (uintptr_t)alEnable);
  hook_arm64(so_find_addr("alFilterf"), (uintptr_t)alFilterf);
  hook_arm64(so_find_addr("alFilterfv"), (uintptr_t)alFilterfv);
  hook_arm64(so_find_addr("alFilteri"), (uintptr_t)alFilteri);
  hook_arm64(so_find_addr("alFilteriv"), (uintptr_t)alFilteriv);
  hook_arm64(so_find_addr("alGenAuxiliaryEffectSlots"), (uintptr_t)alGenAuxiliaryEffectSlots);
  hook_arm64(so_find_addr("alGenBuffers"), (uintptr_t)alGenBuffers);
  hook_arm64(so_find_addr("alGenEffects"), (uintptr_t)alGenEffects);
  hook_arm64(so_find_addr("alGenFilters"), (uintptr_t)alGenFilters);
  hook_arm64(so_find_addr("alGenSources"), (uintptr_t)alGenSources);
  hook_arm64(so_find_addr("alGetAuxiliaryEffectSlotf"), (uintptr_t)alGetAuxiliaryEffectSlotf);
  hook_arm64(so_find_addr("alGetAuxiliaryEffectSlotfv"), (uintptr_t)alGetAuxiliaryEffectSlotfv);
  hook_arm64(so_find_addr("alGetAuxiliaryEffectSloti"), (uintptr_t)alGetAuxiliaryEffectSloti);
  hook_arm64(so_find_addr("alGetAuxiliaryEffectSlotiv"), (uintptr_t)alGetAuxiliaryEffectSlotiv);
  hook_arm64(so_find_addr("alGetBoolean"), (uintptr_t)alGetBoolean);
  hook_arm64(so_find_addr("alGetBooleanv"), (uintptr_t)alGetBooleanv);
  hook_arm64(so_find_addr("alGetBuffer3f"), (uintptr_t)alGetBuffer3f);
  hook_arm64(so_find_addr("alGetBuffer3i"), (uintptr_t)alGetBuffer3i);
  hook_arm64(so_find_addr("alGetBufferf"), (uintptr_t)alGetBufferf);
  hook_arm64(so_find_addr("alGetBufferfv"), (uintptr_t)alGetBufferfv);
  hook_arm64(so_find_addr("alGetBufferi"), (uintptr_t)alGetBufferi);
  hook_arm64(so_find_addr("alGetBufferiv"), (uintptr_t)alGetBufferiv);
  hook_arm64(so_find_addr("alGetDouble"), (uintptr_t)alGetDouble);
  hook_arm64(so_find_addr("alGetDoublev"), (uintptr_t)alGetDoublev);
  hook_arm64(so_find_addr("alGetEffectf"), (uintptr_t)alGetEffectf);
  hook_arm64(so_find_addr("alGetEffectfv"), (uintptr_t)alGetEffectfv);
  hook_arm64(so_find_addr("alGetEffecti"), (uintptr_t)alGetEffecti);
  hook_arm64(so_find_addr("alGetEffectiv"), (uintptr_t)alGetEffectiv);
  hook_arm64(so_find_addr("alGetEnumValue"), (uintptr_t)alGetEnumValue);
  hook_arm64(so_find_addr("alGetError"), (uintptr_t)alGetError);
  hook_arm64(so_find_addr("alGetFilterf"), (uintptr_t)alGetFilterf);
  hook_arm64(so_find_addr("alGetFilterfv"), (uintptr_t)alGetFilterfv);
  hook_arm64(so_find_addr("alGetFilteri"), (uintptr_t)alGetFilteri);
  hook_arm64(so_find_addr("alGetFilteriv"), (uintptr_t)alGetFilteriv);
  hook_arm64(so_find_addr("alGetFloat"), (uintptr_t)alGetFloat);
  hook_arm64(so_find_addr("alGetFloatv"), (uintptr_t)alGetFloatv);
  hook_arm64(so_find_addr("alGetInteger"), (uintptr_t)alGetInteger);
  hook_arm64(so_find_addr("alGetIntegerv"), (uintptr_t)alGetIntegerv);
  hook_arm64(so_find_addr("alGetListener3f"), (uintptr_t)alGetListener3f);
  hook_arm64(so_find_addr("alGetListener3i"), (uintptr_t)alGetListener3i);
  hook_arm64(so_find_addr("alGetListenerf"), (uintptr_t)alGetListenerf);
  hook_arm64(so_find_addr("alGetListenerfv"), (uintptr_t)alGetListenerfv);
  hook_arm64(so_find_addr("alGetListeneri"), (uintptr_t)alGetListeneri);
  hook_arm64(so_find_addr("alGetListeneriv"), (uintptr_t)alGetListeneriv);
  hook_arm64(so_find_addr("alGetProcAddress"), (uintptr_t)alGetProcAddress);
  hook_arm64(so_find_addr("alGetSource3f"), (uintptr_t)alGetSource3f);
  hook_arm64(so_find_addr("alGetSource3i"), (uintptr_t)alGetSource3i);
  hook_arm64(so_find_addr("alGetSourcef"), (uintptr_t)alGetSourcef);
  hook_arm64(so_find_addr("alGetSourcefv"), (uintptr_t)alGetSourcefv);
  hook_arm64(so_find_addr("alGetSourcei"), (uintptr_t)alGetSourcei);
  hook_arm64(so_find_addr("alGetSourceiv"), (uintptr_t)alGetSourceiv);
  hook_arm64(so_find_addr("alGetString"), (uintptr_t)alGetString);
  hook_arm64(so_find_addr("alIsAuxiliaryEffectSlot"), (uintptr_t)alIsAuxiliaryEffectSlot);
  hook_arm64(so_find_addr("alIsBuffer"), (uintptr_t)alIsBuffer);
  hook_arm64(so_find_addr("alIsEffect"), (uintptr_t)alIsEffect);
  hook_arm64(so_find_addr("alIsEnabled"), (uintptr_t)alIsEnabled);
  hook_arm64(so_find_addr("alIsExtensionPresent"), (uintptr_t)alIsExtensionPresent);
  hook_arm64(so_find_addr("alIsFilter"), (uintptr_t)alIsFilter);
  hook_arm64(so_find_addr("alIsSource"), (uintptr_t)alIsSource);
  hook_arm64(so_find_addr("alListener3f"), (uintptr_t)alListener3f);
  hook_arm64(so_find_addr("alListener3i"), (uintptr_t)alListener3i);
  hook_arm64(so_find_addr("alListenerf"), (uintptr_t)alListenerf);
  hook_arm64(so_find_addr("alListenerfv"), (uintptr_t)alListenerfv);
  hook_arm64(so_find_addr("alListeneri"), (uintptr_t)alListeneri);
  hook_arm64(so_find_addr("alListeneriv"), (uintptr_t)alListeneriv);
  hook_arm64(so_find_addr("alSource3f"), (uintptr_t)alSource3f);
  hook_arm64(so_find_addr("alSource3i"), (uintptr_t)alSource3i);
  hook_arm64(so_find_addr("alSourcePause"), (uintptr_t)alSourcePause);
  hook_arm64(so_find_addr("alSourcePausev"), (uintptr_t)alSourcePausev);
  hook_arm64(so_find_addr("alSourcePlay"), (uintptr_t)alSourcePlay);
  hook_arm64(so_find_addr("alSourcePlayv"), (uintptr_t)alSourcePlayv);
  hook_arm64(so_find_addr("alSourceQueueBuffers"), (uintptr_t)alSourceQueueBuffers);
  hook_arm64(so_find_addr("alSourceRewind"), (uintptr_t)alSourceRewind);
  hook_arm64(so_find_addr("alSourceRewindv"), (uintptr_t)alSourceRewindv);
  hook_arm64(so_find_addr("alSourceStop"), (uintptr_t)alSourceStop);
  hook_arm64(so_find_addr("alSourceStopv"), (uintptr_t)alSourceStopv);
  hook_arm64(so_find_addr("alSourceUnqueueBuffers"), (uintptr_t)alSourceUnqueueBuffers);
  hook_arm64(so_find_addr("alSourcef"), (uintptr_t)alSourcef);
  hook_arm64(so_find_addr("alSourcefv"), (uintptr_t)alSourcefv);
  hook_arm64(so_find_addr("alSourcei"), (uintptr_t)alSourcei);
  hook_arm64(so_find_addr("alSourceiv"), (uintptr_t)alSourceiv);
  hook_arm64(so_find_addr("alSpeedOfSound"), (uintptr_t)alSpeedOfSound);
  hook_arm64(so_find_addr("al_print"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("alcCaptureCloseDevice"), (uintptr_t)alcCaptureCloseDevice);
  hook_arm64(so_find_addr("alcCaptureOpenDevice"), (uintptr_t)alcCaptureOpenDevice);
  hook_arm64(so_find_addr("alcCaptureSamples"), (uintptr_t)alcCaptureSamples);
  hook_arm64(so_find_addr("alcCaptureStart"), (uintptr_t)alcCaptureStart);
  hook_arm64(so_find_addr("alcCaptureStop"), (uintptr_t)alcCaptureStop);
  hook_arm64(so_find_addr("alcCloseDevice"), (uintptr_t)alcCloseDevice);
  hook_arm64(so_find_addr("alcCreateContext"), (uintptr_t)alcCreateContextHook);
  hook_arm64(so_find_addr("alcDestroyContext"), (uintptr_t)alcDestroyContext);
  hook_arm64(so_find_addr("alcGetContextsDevice"), (uintptr_t)alcGetContextsDevice);
  hook_arm64(so_find_addr("alcGetCurrentContext"), (uintptr_t)alcGetCurrentContext);
  hook_arm64(so_find_addr("alcGetEnumValue"), (uintptr_t)alcGetEnumValue);
  hook_arm64(so_find_addr("alcGetError"), (uintptr_t)alcGetError);
  hook_arm64(so_find_addr("alcGetIntegerv"), (uintptr_t)alcGetIntegerv);
  hook_arm64(so_find_addr("alcGetProcAddress"), (uintptr_t)alcGetProcAddress);
  hook_arm64(so_find_addr("alcGetString"), (uintptr_t)alcGetString);
  hook_arm64(so_find_addr("alcGetThreadContext"), (uintptr_t)alcGetThreadContext);
  hook_arm64(so_find_addr("alcIsExtensionPresent"), (uintptr_t)alcIsExtensionPresent);
  hook_arm64(so_find_addr("alcMakeContextCurrent"), (uintptr_t)alcMakeContextCurrent);
  hook_arm64(so_find_addr("alcOpenDevice"), (uintptr_t)alcOpenDevice);
  hook_arm64(so_find_addr("alcProcessContext"), (uintptr_t)alcProcessContext);
  hook_arm64(so_find_addr("alcSetThreadContext"), (uintptr_t)alcSetThreadContext);
  hook_arm64(so_find_addr("alcSuspendContext"), (uintptr_t)alcSuspendContext);
}
