/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SPCCodec.h"

#include <cctype>
#include <kodi/Filesystem.h>
#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>
#include <unordered_set>

//------------------------------------------------------------------------------

#define SPC_CHANNELS 2
#define SPC_SAMPLERATE 32000
#define SPC_BITSPERSAMPLE 16

#define MAX_PATH_LENGTH 2048

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
  std::string toLoad(filename);
  int track = GetTrackNumber(toLoad);

  if (kodi::tools::StringUtils::EndsWith(filename, ".spc" KODI_ADDON_AUDIODECODER_TRACK_EXT))
  {
    std::vector<kodi::vfs::CDirEntry> items;
    if (kodi::vfs::GetDirectory("rar://" + URLEncode(toLoad) + "/", ".spc", items))
    {
      toLoad = items[track].Path();
    }
    else
    {
      return false;
    }
  }

  kodi::vfs::CFile file;
  if (!file.OpenFile(toLoad, 0))
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
              toLoad.c_str());
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

int CSPCCodec::ReadPCM(uint8_t* buffer, size_t size, size_t& actualsize)
{
  if (ctx.pos >= ctx.tag.play_len)
    return AUDIODECODER_READ_EOF;

  spc_play(ctx.song, size / SPC_CHANNELS, (short*)buffer);
  actualsize = size;
  ctx.pos += actualsize / SPC_CHANNELS;

  if (actualsize)
    return AUDIODECODER_READ_SUCCESS;

  return AUDIODECODER_READ_ERROR;
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
  std::string toLoad(filename);
  int track = GetTrackNumber(toLoad);
  bool isRSNBaseRead = kodi::tools::StringUtils::EndsWith(filename, ".rsn");
  std::vector<kodi::vfs::CDirEntry> items;

  if (kodi::tools::StringUtils::EndsWith(filename, ".spc" KODI_ADDON_AUDIODECODER_TRACK_EXT) || isRSNBaseRead)
  {
    if (kodi::vfs::GetDirectory("rar://" + URLEncode(toLoad) + "/", ".spc", items))
      toLoad = items[track].Path();
  }

  kodi::vfs::CFile file;
  if (!file.OpenFile(toLoad, 0))
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

  if (!isRSNBaseRead)
  {
    tag.SetArtist(spcTag.artist);
    tag.SetTitle(spcTag.song);
    tag.SetDuration(spcTag.play_len / SPC_SAMPLERATE / SPC_CHANNELS);
    tag.SetDisc(spcTag.ost_disc);
    tag.SetTrack(spcTag.ost_track);
    tag.SetComment(spcTag.comment);
    tag.SetSamplerate(SPC_SAMPLERATE);
    tag.SetChannels(SPC_CHANNELS);

    if (spcTag.ost_track > 0)
    {
      tag.SetTrack(spcTag.ost_track);
    }
    else
    {
      // If track number was not present, check related folder contains only
      // numbered files and no number more as one time inside.
      //
      // Mostly all files where I found for tests, was with a sort number on
      // begin, this adds the bit hacky way to get track number by filename.
      char section = 0;
      char track = 0;
      size_t last_index = std::string::npos;
      bool check_at_end = false;
      std::string filename = kodi::vfs::GetFileName(toLoad);
      if (!isdigit(filename[0]))
      {
        filename.resize(filename.size() - 4); // Remove line ending
        last_index = filename.find_last_not_of("0123456789");
        if (last_index != std::string::npos)
        {
          if (filename.substr(last_index - 1)[0] == '-' && !isdigit(filename.substr(last_index)[0]))
          {
            section = filename.substr(last_index)[0];
            tag.SetGenre(GetGenre(section));
          }
          track = atoi(filename.substr(last_index + 1).c_str());
        }
        check_at_end = true;
      }
      else
      {
        track = atoi(filename.c_str());
      }

      if (track > 0)
      {
        std::vector<kodi::vfs::CDirEntry> items;
        if (kodi::vfs::GetDirectory(kodi::vfs::GetDirectoryName(toLoad), ".spc", items) &&
            items.size() > 1)
        {
          bool sorted_dir = true;
          std::map<char, std::unordered_set<int>> found;
          for (const auto& entry : items)
          {
            if (entry.IsFolder())
              continue;

            std::string label = entry.Label();
            char scanSection = 0;
            char scanTrack = 0;
            if (!check_at_end)
            {
              if (!isdigit(label[0]))
              {
                sorted_dir = false;
                break;
              }

              scanTrack = atoi(label.c_str());
            }
            else
            {
              label.resize(label.size() - 4); // Remove line ending
              size_t last_index = label.find_last_not_of("0123456789");
              if (last_index != std::string::npos)
              {
                if (label.substr(last_index - 1)[0] == '-' && !isdigit(label.substr(last_index)[0]))
                  scanSection = label.substr(last_index)[0];
                scanTrack = atoi(label.substr(last_index + 1).c_str());
                label = label.substr(last_index + 1);
              }
              if (!isdigit(label[0]))
              {
                sorted_dir = false;
                break;
              }
            }

            if (found[scanSection].find(scanTrack) == found[scanSection].end())
              found[scanSection].emplace(scanTrack);
            else
            {
              sorted_dir = false;
              break;
            }
          }

          if (sorted_dir)
          {

            int disc = 1;
            for (const auto& entry : found)
            {
              if (entry.first == section)
                break;
              ++disc;
            }

            tag.SetTrack(track);
            tag.SetDisc(disc);
            tag.SetDiscTotal(found.size());
          }
        }
      }
    }
  }

  tag.SetAlbumArtist(spcTag.publisher);
  tag.SetAlbum(spcTag.game);
  tag.SetReleaseDate(spcTag.year > 0 ? std::to_string(spcTag.year) : "");

  delete[] data;

  return true;
}

int CSPCCodec::TrackCount(const std::string& filename)
{
  if (kodi::tools::StringUtils::EndsWith(filename, ".rsn"))
  {
    std::vector<kodi::vfs::CDirEntry> items;
    if (kodi::vfs::GetDirectory("rar://" + URLEncode(filename), ".spc", items))
      return items.size();
  }

  return 1;
}

int CSPCCodec::GetTrackNumber(std::string& toLoad)
{
  int track = 0;
  toLoad = kodi::addon::CInstanceAudioDecoder::GetTrack("spc", toLoad, track);

  // Correct if packed sound file with several sounds
  if (track > 0)
    --track;

  return track;
}

std::string CSPCCodec::GetGenre(char idChar)
{
  switch (idChar)
  {
    case 'v':
      return kodi::GetLocalizedString(30100);
    default:
      break;
  }

  return "";
}

std::string CSPCCodec::URLEncode(const std::string& strURLData)
{
  std::string strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve(strURLData.length() * 2);

  for (size_t i = 0; i < strURLData.size(); ++i)
  {
    const unsigned char kar = strURLData[i];

    // Don't URL encode "-_.!()" according to RFC1738
    //! @todo Update it to "-_.~" after Gotham according to RFC3986
    if (std::isalnum(kar) || kar == '-' || kar == '.' || kar == '_' || kar == '!' || kar == '(' ||
        kar == ')')
      strResult.push_back(kar);
    else
    {
      char temp[MAX_PATH_LENGTH];
      snprintf(temp, MAX_PATH_LENGTH, "%%%2.2X", (unsigned int)((unsigned char)kar));
      strResult += temp;
    }
  }

  return strResult;
}

//------------------------------------------------------------------------------

class ATTR_DLL_LOCAL CMyAddon : public kodi::addon::CAddonBase
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
