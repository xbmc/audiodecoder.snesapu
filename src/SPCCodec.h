/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "id666.h"
#include "spc.h"

#include <kodi/addon-instance/AudioDecoder.h>

struct SPCContext
{
  id666 tag{0};
  snes_spc_t* song = nullptr;
  int64_t pos;
  int64_t len;
  uint8_t* data = nullptr;
};

class ATTRIBUTE_HIDDEN CSPCCodec : public kodi::addon::CInstanceAudioDecoder
{
public:
  CSPCCodec(KODI_HANDLE instance, const std::string& version);
  virtual ~CSPCCodec();

  bool Init(const std::string& filename,
            unsigned int filecache,
            int& channels,
            int& samplerate,
            int& bitspersample,
            int64_t& totaltime,
            int& bitrate,
            AudioEngineDataFormat& format,
            std::vector<AudioEngineChannel>& channellist) override;
  int ReadPCM(uint8_t* buffer, int size, int& actualsize) override;
  int64_t Seek(int64_t time) override;
  int TrackCount(const std::string& filename) override;
  bool ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag) override;

private:
  int GetTrackNumber(std::string& toLoad);
  std::string URLEncode(const std::string& strURLData);
  std::string GetGenre(char idChar);

  SPCContext ctx;
};
