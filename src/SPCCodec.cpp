/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SPCCodec.h"

#include <kodi/Filesystem.h>

//------------------------------------------------------------------------------

#define SPC_CHANNELS 2
#define SPC_SAMPLERATE 32000
#define SPC_BITSPERSAMPLE 16

CSPCCodec::CSPCCodec(KODI_HANDLE instance, const std::string& version)
  : CInstanceAudioDecoder(instance, version)
{
}

CSPCCodec::~CSPCCodec()
{
  delete[] ctx.data;
  if (ctx.song)
    spc_delete(ctx.song);
}

bool CSPCCodec::Init(const std::string& filename,
                     unsigned int filecache,
                     int& channels,
                     int& samplerate,
                     int& bitspersample,
                     int64_t& totaltime,
                     int& bitrate,
                     AudioEngineDataFormat& format,
                     std::vector<AudioEngineChannel>& channellist)
{
  kodi::vfs::CFile file;
  if (!file.OpenFile(filename, 0))
    return false;

  ctx.song = spc_new();
  ctx.len = file.GetLength();
  ctx.data = new uint8_t[ctx.len];
  file.Read(ctx.data, ctx.len);
  file.Close();

  ctx.pos = 0;

  spc_load_spc(ctx.song, ctx.data, ctx.len);

  if (id666_parse(&ctx.tag, ctx.data, ctx.len) != 0)
  {
    kodi::Log(ADDON_LOG_WARNING,
              "Failed to parse tag information to get play length on '%s', using 4 minutes",
              filename.c_str());
    ctx.tag.play_len = 4 * 60;
  }

  channels = SPC_CHANNELS;
  samplerate = SPC_SAMPLERATE;
  bitspersample = SPC_BITSPERSAMPLE;
  totaltime = ctx.tag.play_len * 1000 / SPC_SAMPLERATE / SPC_CHANNELS;
  format = AUDIOENGINE_FMT_S16NE;
  bitrate = 0;
  channellist = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR};

  return true;
}

int CSPCCodec::ReadPCM(uint8_t* buffer, int size, int& actualsize)
{
  if (ctx.pos >= ctx.tag.play_len)
    return -1;

  spc_play(ctx.song, size / SPC_CHANNELS, (short*)buffer);
  actualsize = size;
  ctx.pos += actualsize / SPC_CHANNELS;

  if (actualsize)
    return 0;

  return 1;
}

int64_t CSPCCodec::Seek(int64_t time)
{
  int64_t pos = time * SPC_SAMPLERATE * SPC_CHANNELS / 1000;
  if (ctx.pos > pos)
  {
    spc_load_spc(ctx.song, ctx.data, ctx.len);
    ctx.pos = 0;
  }

  spc_skip(ctx.song, pos - ctx.pos);
  ctx.pos = pos;
  return time;
}

bool CSPCCodec::ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag)
{
  kodi::vfs::CFile file;
  if (!file.OpenFile(filename, 0))
    return false;

  int len = file.GetLength();
  uint8_t* data = new uint8_t[len];
  if (!data)
    return false;

  file.Read(data, len);
  file.Close();

  id666 spcTag{0};
  if (id666_parse(&spcTag, data, len) != 0)
    return false;

  tag.SetArtist(spcTag.artist);
  tag.SetAlbumArtist(spcTag.publisher);
  tag.SetTitle(spcTag.song);
  tag.SetAlbum(spcTag.game);
  tag.SetDuration(spcTag.play_len / SPC_SAMPLERATE / SPC_CHANNELS);
  tag.SetDisc(spcTag.ost_disc);
  tag.SetTrack(spcTag.ost_track);
  tag.SetReleaseDate(spcTag.year > 0 ? std::to_string(spcTag.year) : "");
  tag.SetComment(spcTag.comment);
  tag.SetSamplerate(SPC_SAMPLERATE);
  tag.SetChannels(SPC_CHANNELS);

  delete[] data;

  return true;
}

//------------------------------------------------------------------------------

class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(int instanceType,
                              const std::string& instanceID,
                              KODI_HANDLE instance,
                              const std::string& version,
                              KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CSPCCodec(instance, version);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon() = default;
};

ADDONCREATOR(CMyAddon)
