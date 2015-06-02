/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "libXBMC_addon.h"
#include "spc.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

extern "C" {
#include "kodi_audiodec_dll.h"
#include "AEChannelData.h"

char soundfont[1024];

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct SPC_ID666
{
  char songname[33];
  char gametitle[33];
  char dumper[17];
  char comments[33];
  char author[33];
  int64_t playtime;
};

// copied from libspc, then modified. thanks :)
SPC_ID666* SPC_get_id666FP (uint8_t* data)
{
  SPC_ID666* id = new SPC_ID666;
  unsigned char playtime_str[4] = { 0, 0, 0, 0 };

  char c;
  c = data[0x23];
  if (c == 27) {
      delete id;
      return NULL;
  }

  memcpy(id->songname, data+0x2E, 32);
  id->songname[32] = '\0';

  memcpy(id->gametitle, data+32+0x2E, 32);
  id->gametitle[32] = '\0';

  memcpy(id->dumper, data+64+0x2E, 16);
  id->dumper[16] = '\0';

  memcpy(id->comments, data+64+16+0x2E, 32);
  id->comments[32] = '\0';

  memcpy(playtime_str, data+0xA9, 3);
  playtime_str[3] = '\0';
  id->playtime = atoi((char*)playtime_str);

  memcpy(id->author, data+0xB0, 32);
  id->author[32] = '\0';

  return id;
}

struct SPCContext
{
  SPC_ID666* tag;
  SNES_SPC* song;
  int64_t pos;
  int64_t len;
  uint8_t* data;
};

#define SET_IF(ptr, value) \
{ \
  if ((ptr)) \
   *(ptr) = (value); \
}

void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  SPCContext* result = new SPCContext;
  if (!result)
    return NULL;

  void* file = XBMC->OpenFile(strFile, 0);
  if (!file)
  {
    delete result;
    return NULL;
  }

  result->song = spc_new();
  result->len = XBMC->GetFileLength(file);
  result->data = new uint8_t[result->len];
  XBMC->ReadFile(file, result->data, result->len);
  XBMC->CloseFile(file);

  result->pos = 0;

  spc_load_spc(result->song, result->data, result->len);

  result->tag = SPC_get_id666FP(result->data);
  if (!result->tag->playtime)
    result->tag->playtime = 4*60;

  SET_IF(channels, 2)
  SET_IF(samplerate, 32000)
  SET_IF(bitspersample, 16)
  SET_IF(totaltime, result->tag->playtime*1000)
  SET_IF(format, AE_FMT_S16NE)
  SET_IF(bitrate, 0)
  static enum AEChannel map[3] = {
    AE_CH_FL, AE_CH_FR, AE_CH_NULL
  };
  SET_IF(channelinfo, map)

  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  if (!context || !actualsize)
    return 1;

  SPCContext* ctx = (SPCContext*)context;

  if (ctx->pos > ctx->tag->playtime*32000*4)
    return -1;

  spc_play(ctx->song, size/2, (short*)pBuffer);
  *actualsize = size;
  ctx->pos += *actualsize;

  if (*actualsize)
    return 0;

  return 1;
}

int64_t Seek(void* context, int64_t time)
{
  if (!context)
    return 0;

  SPCContext* ctx = (SPCContext*)context;

  if (ctx->pos > time/1000*32000*4)
  {
    spc_load_spc(ctx->song, ctx->data, ctx->len);
    ctx->pos = 0;
  }

  spc_skip(ctx->song,time/1000*32000-ctx->pos/2);
  return time;
}

bool DeInit(void* context)
{
  if (!context)
    return true;

  SPCContext* ctx = (SPCContext*)context;

  delete ctx->tag;
  delete[] ctx->data;
  delete ctx;

  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist,
             int* length)
{
  void* file = XBMC->OpenFile(strFile, 0);
  if (!file)
    return false;

  int len = XBMC->GetFileLength(file);
  uint8_t* data = new uint8_t[len];
  if (!data)
  {
    XBMC->CloseFile(file);
    return false;
  }
  XBMC->ReadFile(file, data, len);
  XBMC->CloseFile(file);

  SPC_ID666* tag = SPC_get_id666FP(data);
  strcpy(title, tag->songname);
  strcpy(artist, tag->author);
  *length = tag->playtime;

  delete[] data;
  delete tag;

  return true;
}

int TrackCount(const char* strFile)
{
  return 1;
}
}
