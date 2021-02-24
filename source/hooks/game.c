/* game.c -- hooks and patches for everything other than AL and GL
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <threads.h>
#include <switch.h>

#include "../config.h"
#include "../util.h"
#include "../so_util.h"
#include "../hooks.h"

#define APK_PATH "main.obb"

extern uintptr_t __cxa_guard_acquire;
extern uintptr_t __cxa_guard_release;
extern uintptr_t __cxa_throw;

static int *deviceChip;
static int *deviceForm;
static int *definedDevice;

static PadState pad;

static uint8_t fake_tls[0x100];

// control binding array
typedef struct {
  int unk[14];
} MaxPayne_InputControl;
static MaxPayne_InputControl *sm_control = NULL; // [32]

int NvAPKOpen(const char *path) {
  // debugPrintf("NvAPKOpen: %s\n", path);
  return 0;
}

int ProcessEvents(void) {
  return 0; // 1 is exit!
}

int AND_DeviceType(void) {
  // 0x1: phone
  // 0x2: tegra
  // low memory is < 256
  return (MEMORY_MB << 6) | (3 << 2) | 0x2;
}

int AND_DeviceLocale(void) {
  return 0; // english
}

int AND_SystemInitialize(void) {
  // set device information in such a way that bloom isn't enabled
  *deviceForm = 1; // phone
  *deviceChip = 14; // some tegra? tegras are 12, 13, 14
  *definedDevice = 27; // some tegra?
  return 0;
}

int OS_ScreenGetHeight(void) {
  return screen_height;
}

int OS_ScreenGetWidth(void) {
  return screen_width;
}

char *OS_FileGetArchiveName(int mode) {
  char *out = malloc(strlen(APK_PATH) + 1);
  out[0] = '\0';
  if (mode == 1) // main
    strcpy(out, APK_PATH);
  return out;
}

void ExitAndroidGame(int code) {
  // deinit openal
  deinit_openal();
  // deinit EGL
  deinit_opengl();
  // unmap lib
  so_unload();
  // die
  // exit(0); // doesn't actually exit?
  extern void NORETURN __libnx_exit(int rc);
  __libnx_exit(0);
}

// this is supposed to allocate and return a thread handle struct, but the game never uses it
// and never frees it, so we just return a pointer to some static garbage
void *OS_ThreadLaunch(int (* func)(void *), void *arg, int r2, char *name, int r4, int priority) {
  static char buf[0x80];
  thrd_t thrd;
  thrd_create(&thrd, func, arg);
  return buf;
}

int ReadDataFromPrivateStorage(const char *file, void **data, int *size) {
  debugPrintf("ReadDataFromPrivateStorage %s\n", file);

  FILE *f = fopen(file, "rb");
  if (!f) return 0;

  fseek(f, 0, SEEK_END);
  const int sz = ftell(f);
  fseek(f, 0, SEEK_SET);

  int ret = 0;

  if (sz > 0) {
    void *buf = malloc(sz);
    if (buf && fread(buf, sz, 1, f)) {
      ret = 1;
      *size = sz;
      *data = buf;
    } else {
      free(buf);
    }
  }

  fclose(f);

  return ret;
}

int WriteDataToPrivateStorage(const char *file, const void *data, int size) {
  debugPrintf("WriteDataToPrivateStorage %s\n", file);

  FILE *f = fopen(file, "wb");
  if (!f) return 0;

  const int ret = fwrite(data, size, 1, f);
  fclose(f);

  return ret;
}

// 0, 5, 6: XBOX 360
// 4: MogaPocket
// 7: MogaPro
// 8: PS3
// 9: IOSExtended
// 10: IOSSimple
int WarGamepad_GetGamepadType(int padnum) {
  return 0;
}

int WarGamepad_GetGamepadButtons(int padnum) {
  int mask = 0;

  // this is called first, so we call update here
  padUpdate(&pad);
  const u32 kdown = padGetButtons(&pad);

  if (kdown & HidNpadButton_A)
    mask |= 0x1;
  if (kdown & HidNpadButton_B)
    mask |= 0x2;
  if (kdown & HidNpadButton_X)
    mask |= 0x4;
  if (kdown & HidNpadButton_Y)
    mask |= 0x8;
  if (kdown & HidNpadButton_Plus)
    mask |= 0x10;
  if (kdown & HidNpadButton_Minus)
    mask |= 0x20;
  if (kdown & HidNpadButton_L)
    mask |= 0x40;
  if (kdown & HidNpadButton_R)
    mask |= 0x80;
  if (kdown & HidNpadButton_Up)
    mask |= 0x100;
  if (kdown & HidNpadButton_Down)
    mask |= 0x200;
  if (kdown & HidNpadButton_Left)
    mask |= 0x400;
  if (kdown & HidNpadButton_Right)
    mask |= 0x800;
  if (kdown & HidNpadButton_StickL)
    mask |= 0x1000;
  if (kdown & HidNpadButton_StickR)
    mask |= 0x2000;

  return mask;
}

float WarGamepad_GetGamepadAxis(int padnum, int axis) {
  const float scale = 1.f / (float)0x7fff;
  const u32 kdown = padGetButtonsDown(&pad);
  const HidAnalogStickState sticks[2] = {
    padGetStickPos(&pad, 0),
    padGetStickPos(&pad, 1)
  };

  float val = 0.0f;

  switch (axis) {
    case 0:
      val = (float)sticks[0].x * scale;
      break;
    case 1:
      val = (float)sticks[0].y * -scale;
      break;
    case 2:
      val = (float)sticks[1].x * scale;
      break;
    case 3:
      val = (float)sticks[1].y * -scale;
      break;
    case 4: // LT
      val = (kdown & HidNpadButton_ZL) ? 1.0f : 0.0f;
      break;
    case 5: // RT
      val = (kdown & HidNpadButton_ZR) ? 1.0f : 0.0f;
      break;
  }

  if (fabsf(val) > 0.2f)
    return val;

  return 0.0f;
}

static int (* MaxPayne_InputControl_getButton)(MaxPayne_InputControl *, int);

int MaxPayne_ConfiguredInput_readCrouch(void *this) {
  static int prev = 0;
  static int latch = 0;
  // crouch is control #5
  const int new = MaxPayne_InputControl_getButton(&sm_control[5], 0);
  if (prev != new) {
    prev = new;
    if (new) latch = !latch;
  }
  return latch;
}

int GetAndroidCurrentLanguage(void) {
  // this will be loaded from config.txt; cap it
  if (config.language < 0 || config.language > 6)
    config.language = 0; // english
  return config.language;
}

void SetAndroidCurrentLanguage(int lang) {
  if (config.language != lang) {
    // changed; save config
    config.language = lang;
    write_config(CONFIG_NAME);
  }
}

static int (* R_File_loadArchives)(void *this);
static void (* R_File_unloadArchives)(void *this);
static void (* R_File_enablePriorityArchive)(void *this, const char *arc);

int R_File_setFileSystemRoot(void *this, const char *root) {
  // root appears to be unused?
  R_File_unloadArchives(this);
  const int res = R_File_loadArchives(this);
  R_File_enablePriorityArchive(this, config.mod_file);
  return res;
}

int X_DetailLevel_getCharacterShadows(void) {
  return config.character_shadows;
}

int X_DetailLevel_getDropHighestLOD(void) {
  return config.drop_highest_lod;
}

float X_DetailLevel_getDecalLimitMultiplier(void) {
  return config.decal_limit;
}

float X_DetailLevel_getDebrisProjectileLimitMultiplier(void) {
  return config.debris_limit;
}

int64_t UseBloom(void) {
  return config.use_bloom;
}

void patch_game(void) {
  // configure our supported input layout: all players with standard controller styles
  padConfigureInput(8, HidNpadStyleSet_NpadStandard);
  // initialize the gamepad for reading all controllers
  padInitializeAny(&pad);

  // make it crash in an obvious location when it calls JNI methods
  hook_arm64(so_find_addr("_Z24NVThreadGetCurrentJNIEnvv"), (uintptr_t)0x1337);

  hook_arm64(so_find_addr("__cxa_throw"), (uintptr_t)&__cxa_throw);
  hook_arm64(so_find_addr("__cxa_guard_acquire"), (uintptr_t)&__cxa_guard_acquire);
  hook_arm64(so_find_addr("__cxa_guard_release"), (uintptr_t)&__cxa_guard_release);

  hook_arm64(so_find_addr("_Z15OS_ThreadLaunchPFjPvES_jPKci16OSThreadPriority"), (uintptr_t)OS_ThreadLaunch);

  // used to check some flags
  hook_arm64(so_find_addr("_Z20OS_ServiceAppCommandPKcS0_"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z23OS_ServiceAppCommandIntPKci"), (uintptr_t)ret0);
  // this is checked on startup
  hook_arm64(so_find_addr("_Z25OS_ServiceIsWifiAvailablev"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z28OS_ServiceIsNetworkAvailablev"), (uintptr_t)ret0);
  // don't bother opening links
  hook_arm64(so_find_addr("_Z18OS_ServiceOpenLinkPKc"), (uintptr_t)ret0);

  // don't have movie playback yet
  hook_arm64(so_find_addr("_Z12OS_MoviePlayPKcbbf"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z12OS_MovieStopv"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z20OS_MovieSetSkippableb"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z17OS_MovieTextScalei"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z17OS_MovieIsPlayingPi"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z20OS_MoviePlayinWindowPKciiiibbf"), (uintptr_t)ret0);

  hook_arm64(so_find_addr("_Z17OS_ScreenGetWidthv"), (uintptr_t)OS_ScreenGetWidth);
  hook_arm64(so_find_addr("_Z18OS_ScreenGetHeightv"), (uintptr_t)OS_ScreenGetHeight);

  hook_arm64(so_find_addr("_Z9NvAPKOpenPKc"), (uintptr_t)ret0);

  // TODO: implement touch here
  hook_arm64(so_find_addr("_Z13ProcessEventsb"), (uintptr_t)ProcessEvents);

  // both set and get are called, remember the language that it sets
  hook_arm64(so_find_addr("_Z25GetAndroidCurrentLanguagev"), (uintptr_t)GetAndroidCurrentLanguage);
  hook_arm64(so_find_addr("_Z25SetAndroidCurrentLanguagei"), (uintptr_t)SetAndroidCurrentLanguage);

  hook_arm64(so_find_addr("_Z14AND_DeviceTypev"), (uintptr_t)AND_DeviceType);
  hook_arm64(so_find_addr("_Z16AND_DeviceLocalev"), (uintptr_t)AND_DeviceLocale);
  hook_arm64(so_find_addr("_Z20AND_SystemInitializev"), (uintptr_t)AND_SystemInitialize);
  hook_arm64(so_find_addr("_Z21AND_ScreenSetWakeLockb"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z22AND_FileGetArchiveName13OSFileArchive"), (uintptr_t)OS_FileGetArchiveName);

  hook_arm64(so_find_addr("_Z26ReadDataFromPrivateStoragePKcRPcRi"), (uintptr_t)ReadDataFromPrivateStorage);
  hook_arm64(so_find_addr("_Z25WriteDataToPrivateStoragePKcS0_i"), (uintptr_t)WriteDataToPrivateStorage);

  hook_arm64(so_find_addr("_Z25WarGamepad_GetGamepadTypei"), (uintptr_t)WarGamepad_GetGamepadType);
  hook_arm64(so_find_addr("_Z28WarGamepad_GetGamepadButtonsi"), (uintptr_t)WarGamepad_GetGamepadButtons);
  hook_arm64(so_find_addr("_Z25WarGamepad_GetGamepadAxisii"), (uintptr_t)WarGamepad_GetGamepadAxis);

  // no vibration of any kind
  hook_arm64(so_find_addr("_Z12VibratePhonei"), (uintptr_t)ret0);
  hook_arm64(so_find_addr("_Z14Mobile_Vibratei"), (uintptr_t)ret0);

  hook_arm64(so_find_addr("_Z15ExitAndroidGamev"), (uintptr_t)ExitAndroidGame);

  // hook detail level getters to our own settings
  hook_arm64(so_find_addr("_ZN13X_DetailLevel19getCharacterShadowsEv"), (uintptr_t)X_DetailLevel_getCharacterShadows);
  hook_arm64(so_find_addr("_ZN13X_DetailLevel34getDebrisProjectileLimitMultiplierEv"), (uintptr_t)X_DetailLevel_getDebrisProjectileLimitMultiplier);
  hook_arm64(so_find_addr("_ZN13X_DetailLevel23getDecalLimitMultiplierEv"), (uintptr_t)X_DetailLevel_getDecalLimitMultiplier);
  hook_arm64(so_find_addr("_ZN13X_DetailLevel13dropHighesLODEv"), (uintptr_t)X_DetailLevel_getDropHighestLOD);

  // force bloom to our config value
  hook_arm64(so_find_addr("_Z8UseBloomv"), (uintptr_t)UseBloom);

  // dummy out the weapon menu arrow drawer if it's disabled
  if (!config.show_weapon_menu)
    hook_arm64(so_find_addr("_ZN12WeaponSwiper4DrawEv"), (uintptr_t)ret0);

  // crouch toggle
  if (config.crouch_toggle) {
    sm_control = (void *)so_find_addr_rx("_ZN24MaxPayne_ConfiguredInput10sm_controlE");
    MaxPayne_InputControl_getButton = (void *)so_find_addr_rx("_ZNK21MaxPayne_InputControl9getButtonEi");
    hook_arm64(so_find_addr("_ZNK24MaxPayne_ConfiguredInput10readCrouchEv"), (uintptr_t)MaxPayne_ConfiguredInput_readCrouch);
  }

  // if mod file is enabled, hook into R_File::setFileSystemRoot to set the mod as the priority archive
  // before R_File::loadArchives is called
  if (config.mod_file[0]) {
    R_File_unloadArchives = (void *)so_find_addr_rx("_ZN6R_File14unloadArchivesEv");
    R_File_loadArchives = (void *)so_find_addr_rx("_ZN6R_File12loadArchivesEv");
    R_File_enablePriorityArchive = (void *)so_find_addr_rx("_ZN6R_File21enablePriorityArchiveEPKc");
    hook_arm64(so_find_addr("_ZN6R_File17setFileSystemRootEPKc"), (uintptr_t)R_File_setFileSystemRoot);
  }

  // HACK: THIS IS POSSIBLY VERY BAD
  // the game uses some sort of a stack guard mechanism that reads an offset from TPIDR_EL0,
  // reads a cookie value from that offset and then uses it to check stack frame integrity
  // however on the Switch TPIDR_EL0 seems to just return 0 (armGetTls() uses TPIDRRO_EL0)
  // I don't know whether this will cause any issues or not, but we just write a pointer to
  // a static buffer to TPIDR_EL0 and let the game use that
  armSetTlsRw(fake_tls);

  // vars used in AND_SystemInitialize
  deviceChip = (int *)so_find_addr_rx("deviceChip");
  deviceForm = (int *)so_find_addr_rx("deviceForm");
  definedDevice = (int *)so_find_addr_rx("definedDevice");
}
