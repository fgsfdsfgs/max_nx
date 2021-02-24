/* config.h -- global configuration and config file handling
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

// should be enough for pretend purposes
#define MEMORY_MB 512

#define SO_NAME "libMaxPayne.so"
#define CONFIG_NAME "config.txt"
#define LOG_NAME "debug.log"

#define SCREEN_W 1280
#define SCREEN_H 720

// #define DEBUG_LOG 1

typedef struct {
  int use_bloom;
  int trilinear_filter;
  int msaa;
  int disable_mipmaps;
  int language;
  int crouch_toggle;
  int character_shadows;
  int drop_highest_lod;
  float decal_limit;
  float debris_limit;
  char mod_file[0x100];
} Config;

extern Config config;

int read_config(const char *file);
int write_config(const char *file);

#endif
