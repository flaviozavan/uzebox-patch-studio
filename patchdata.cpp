#include <wx/treectrl.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include "patchdata.h"
#include "waves.h"
#include "step_table.h"

PatchData::PatchData() : wave(nullptr), channel(-1) {
};

PatchData::PatchData(const PatchData *p) :
  data(p->data),
  wave(nullptr),
  channel(-1) {
}

PatchData::~PatchData() {
  stop();
  free_chunk();
}

void PatchData::stop() {
  if (channel != -1) {
    Mix_HaltChannel(channel);
    channel = -1;
  }
}

bool PatchData::play(bool loop) {
  stop();
  free_chunk();

  if (generate_wave(wave_data)
      && (wave = Mix_QuickLoad_WAV(&(wave_data[0])))) {
    if (loop) {
      channel = Mix_PlayChannel(-1, wave, -1);
    }
    else {
      Mix_PlayChannel(-1, wave, 0);
    }
  }
  else {
    return false;
  }

  return true;
}

void PatchData::retrigger() {
  if (channel != -1) {
    Mix_PlayChannel(channel, wave, -1);
  }
}

void PatchData::free_chunk() {
  if (wave != nullptr) {
    Mix_FreeChunk(wave);
    wave = nullptr;
  }
}

void PatchData::add_headers(wxVector<uint8_t> &out_data) {
  size_t data_size = out_data.size() - WAVE_HEADER_LEN;
  const uint32_t subchunk2_size = data_size & 1? data_size+1 : data_size;
  const uint32_t chunk_size = subchunk2_size + 36;
  const uint32_t sample_rate = SAMPLE_RATE;
  int pos = 0;

  /* ChunkID */
  out_data[pos++] = 'R';
  out_data[pos++] = 'I';
  out_data[pos++] = 'F';
  out_data[pos++] = 'F';
  /* ChunkSize */
  out_data[pos++] = chunk_size & 0xff;
  out_data[pos++] = (chunk_size>>8) & 0xff;
  out_data[pos++] = (chunk_size>>16) & 0xff;
  out_data[pos++] = (chunk_size>>24) & 0xff;
  /* Format */
  out_data[pos++] = 'W';
  out_data[pos++] = 'A';
  out_data[pos++] = 'V';
  out_data[pos++] = 'E';
  /* Subchunk1ID */
  out_data[pos++] = 'f';
  out_data[pos++] = 'm';
  out_data[pos++] = 't';
  out_data[pos++] = ' ';
  /* Subchunk1Size*/
  out_data[pos++] = 16;
  out_data[pos++] = 0;
  out_data[pos++] = 0;
  out_data[pos++] = 0;
  /* AudioFormat */
  out_data[pos++] = 1;
  out_data[pos++] = 0;
  /* NumChannels */
  out_data[pos++] = 1;
  out_data[pos++] = 0;
  /* SampleRate */
  out_data[pos++] = sample_rate & 0xff;
  out_data[pos++] = (sample_rate>>8) & 0xff;
  out_data[pos++] = (sample_rate>>16) & 0xff;
  out_data[pos++] = (sample_rate>>24) & 0xff;
  /* ByteRate */
  out_data[pos++] = sample_rate & 0xff;
  out_data[pos++] = (sample_rate>>8) & 0xff;
  out_data[pos++] = (sample_rate>>16) & 0xff;
  out_data[pos++] = (sample_rate>>24) & 0xff;
  /* BlockAlign */
  out_data[pos++] = 1;
  out_data[pos++] = 0;
  /* BitsPerSample */
  out_data[pos++] = 8;
  out_data[pos++] = 0;
  /* Subchunk2ID */
  out_data[pos++] = 'd';
  out_data[pos++] = 'a';
  out_data[pos++] = 't';
  out_data[pos++] = 'a';
  /* Subchunk2Size */
  out_data[pos++] = subchunk2_size & 0xff;
  out_data[pos++] = (subchunk2_size>>8) & 0xff;
  out_data[pos++] = (subchunk2_size>>16) & 0xff;
  out_data[pos++] = (subchunk2_size>>24) & 0xff;

  /* Padding */
  if (out_data.size() & 1)
    out_data.push_back(0);
}

bool PatchData::generate_wave(wxVector<uint8_t> &out_data) {
  int8_t note = 80;
  uint16_t next_sample = 0;
  uint8_t note_volume = DEFAULT_VOLUME;
  uint8_t envelope_volume = 0xff;
  int8_t envelope_step = 0;
  int wave = 0;
  uint8_t tremolo_level = 0;
  uint8_t tremolo_rate = 24;
  uint8_t tremolo_pos = 0;
  uint8_t loop_count = 0;
  uint8_t slide_speed = 0x10;
  int16_t slide_step = 0;
  int8_t slide_note = 0;
  bool sliding = false;
  uint16_t track_step = 0;
  uint16_t noise_barrel = 0x0101;
  uint8_t noise_params = 1;
  int8_t noise_divider = 0;
  int extra_time = 0;
  bool is_noise = is_noise_patch();
  out_data.resize(WAVE_HEADER_LEN);


  for (size_t i = 0; extra_time || i < data.size(); i += 3) {
    for (int delay = extra_time? extra_time : data[i]; delay; delay--) {
      int16_t e_vol = envelope_volume + envelope_step;
      e_vol = std::max((int16_t) 0, std::min((int16_t) 0xff, e_vol));
      envelope_volume = e_vol;

      if (sliding) {
        track_step += slide_step;
        uint16_t t_step = step_table[(int) slide_note];

        if ((slide_step > 0 && track_step >= t_step)
            || (slide_step < 0 && track_step <= t_step)) {
          track_step = t_step;
          sliding = false;
        }
      }

      uint16_t vol = note_volume;
      if (note_volume && envelope_volume) {
        vol = ((vol*envelope_volume)+0x100) >> 8;

        /* Assumes the master volume is 0xff, no calculation needed */

        if (tremolo_level > 0) {
          uint8_t t = ((uint8_t *) waves[0])[tremolo_pos];
          t -= 128;
          uint16_t t_vol = (tremolo_level*t)+0x100;
          t_vol >>= 8;
          vol = ((vol*(0xff-t_vol)) + 0x100) >> 8;
        }
      }
      else {
        vol = 0;
      }

      tremolo_pos += tremolo_rate;

      for (int j = 0; j < SAMPLES_PER_FRAME; j++) {
        int8_t sample;
        if (is_noise) {
          if (--noise_divider < 0) {
            noise_divider = noise_params >> 1;
            uint8_t r_xor = (noise_barrel ^ (noise_barrel >> 1)) & 1;
            noise_barrel = (noise_barrel >> 1)
              | (r_xor << (noise_params & 1? 14 : 6));
          }
          sample = noise_barrel & 1? 127 : -128;
        }
        else {
          sample = waves[wave][next_sample>>8];
          next_sample += track_step;
        }
        int16_t v16 = (int16_t) sample * vol;
        /* Signed extention */
        int8_t v8 = v16 / 256;
        out_data.push_back((int) v8 + 128);
      }
    }

    if (extra_time || data[i+1] == PATCH_END) {
      if (!envelope_volume) {
        break;
      }
      if (envelope_step < 0) {
        extra_time = 1;
      }
      else if (!extra_time) {
        extra_time = EXTRA_TIME;
      }
      else {
        break;
      }

      continue;
    }
    else if (data[i+1] == PC_NOTE_CUT) {
      break;
    }

    int current;
    int target;
    switch (data[i+1]) {
      case PC_ENV_SPEED:
        envelope_step = data[i+2];
        if (data[i+2] < -128 || data[i+2] > 127) {
          last_error = wxString::Format(
              _("Command %lu: Invalid envelope speed"), i/3+1);
          return false;
        }
        break;

      case PC_NOISE_PARAMS:
        noise_barrel = 0x0101;
        noise_params = data[i+2];
        if (data[i+2] < 0 || data[i+2] > 255) {
          last_error = wxString::Format(
              _("Command %lu: Invalid noise parameter"), i/3+1);
          return false;
        }
        break;

      case PC_WAVE:
        wave = data[i+2];
        if (wave < 0 || wave >= NUM_WAVES) {
          last_error = wxString::Format(_("Command %lu: Invalid wave"), i/3+1);
          return false;
        }
        break;

      case PC_NOTE_UP:
        note += data[i+2];
        if (note > 126 || note < 0) {
          last_error = wxString::Format(
              _("Command %lu: Invalid note reached"), i/3+1);
          return false;
        }
        track_step = step_table[(int) note];
        break;

      case PC_NOTE_DOWN:
        note -= data[i+2];
        if (note > 126 || note < 0) {
          last_error = wxString::Format(
              _("Command %lu: Invalid note reached"), i/3+1);
          return false;
        }
        track_step = step_table[(int) note];
        break;

      /* TODO */
      case PC_NOTE_HOLD:
        break;

      case PC_ENV_VOL:
        envelope_volume = data[i+2];
        if (data[i+2] < 0 || data[i+2] > 255) {
          last_error = wxString::Format(
              _("Command %lu: Invalid envelope volume"), i/3+1);
          return false;
        }
        break;

      case PC_PITCH:
        note = data[i+2];
        if (note > 126 || note < 0) {
          last_error = wxString::Format(
              _("Command %lu: Invalid note"), i/3+1);
          return false;
        }
        track_step = step_table[(int) note];
        sliding = false;
        break;

      case PC_TREMOLO_LEVEL:
        tremolo_level = data[i+2];
        if (data[i+2] < 0 || data[i+2] > 255) {
          last_error = wxString::Format(
              _("Command %lu: Invalid tremolo level"), i/3+1);
          return false;
        }
        break;

      case PC_TREMOLO_RATE:
        tremolo_rate = data[i+2];
        if (data[i+2] < 0 || data[i+2] > 255) {
          last_error = wxString::Format(
              _("Command %lu: Invalid tremolo rate"), i/3+1);
          return false;
        }
        break;

      case PC_SLIDE:
        current = step_table[(int) note];
        slide_note = note + data[i+2];
        if (slide_note > 126 || slide_note < 0) {
          last_error = wxString::Format(
              _("Command %lu: Invalid slide note"), i/3+1);
          return false;
        }
        target = step_table[(int) slide_note];
        slide_step = std::max(1, (target-current)/slide_speed);
        track_step += slide_step;
        break;

      case PC_SLIDE_SPEED:
        slide_speed = data[i+2];
        if (data[i+2] < 0 || data[i+2] > 255) {
          last_error = wxString::Format(
              _("Command %lu: Invalid slide speed"), i/3+1);
          return false;
        }
        break;

      case PC_LOOP_END:
        if (data[i+2] < 0 || data[i+2] > 255) {
          last_error = wxString::Format(
              _("Command %lu: Invalid loop end jump"), i/3+1);
          return false;
        }
        else if (data[i+2] > (long) i/3) {
          last_error = wxString::Format(
              _("Command %lu: Loop end jump to negative command"), i/3+1);
          return false;
        }
        if (!loop_count) {
          break;
        }
        else {
          size_t old_i = i;
          loop_count--;
          if (data[i+2] > 0) {
            for (long to_return = data[i+2]+1; to_return--; i -= 3) {
              if (data[i+1] == PC_LOOP_START) {
                last_error = wxString::Format(_("Command %lu: Loop end jump "
                      "to before a loop start causes infinite loop"),
                    old_i/3+1);
                return false;
              }
            }
          }
          else {
            do {
              i -= 3;
            } while(i >= 3 && data[i+1] != PC_LOOP_START);
            if (data[i+1] != PC_LOOP_START) {
              last_error = wxString::Format(
                  _("Command %lu: No previous loop start"), old_i/3+1);
              return false;
            }
          }
        }
        break;

      case PC_LOOP_START:
        loop_count = data[i+2];
        if (data[i+2] < 0 || data[i+2] > 255) {
          last_error = wxString::Format(
              _("Command %lu: Invalid loop count"), i/3+1);
          return false;
        }
        break;

      default:
        break;
    }
  }

  add_headers(out_data);

  return true;
}

bool PatchData::is_noise_patch() {
  for (size_t i = 0; i < data.size(); i += 3) {
    if (data[i+1] == PC_NOISE_PARAMS) {
      return true;
    }
  }

  return false;
}
