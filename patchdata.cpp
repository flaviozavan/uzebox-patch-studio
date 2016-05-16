#include <wx/treectrl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "patchdata.h"
#include "waves.h"
#include "step_table.h"

PatchData::PatchData() : wave(nullptr), channel(-1) {
};

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

  if (generate_wave() && (wave = Mix_LoadWAV_RW(
          SDL_RWFromMem(&(wave_data[0]), wave_data.size()), 0))) {
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

void PatchData::free_chunk() {
  if (wave != nullptr) {
    Mix_FreeChunk(wave);
  }
}

void PatchData::add_headers() {
  size_t data_size = wave_data.size() - WAVE_HEADER_LEN;
  const uint32_t subchunk2_size = data_size & 1? data_size+1 : data_size;
  const uint32_t chunk_size = subchunk2_size + 36;
  const uint32_t sample_rate = SAMPLE_RATE;
  int pos = 0;

  /* ChunkID */
  wave_data[pos++] = 'R';
  wave_data[pos++] = 'I';
  wave_data[pos++] = 'F';
  wave_data[pos++] = 'F';
  /* ChunkSize */
  wave_data[pos++] = chunk_size & 0xff;
  wave_data[pos++] = (chunk_size>>8) & 0xff;
  wave_data[pos++] = (chunk_size>>16) & 0xff;
  wave_data[pos++] = (chunk_size>>24) & 0xff;
  /* Format */
  wave_data[pos++] = 'W';
  wave_data[pos++] = 'A';
  wave_data[pos++] = 'V';
  wave_data[pos++] = 'E';
  /* Subchunk1ID */
  wave_data[pos++] = 'f';
  wave_data[pos++] = 'm';
  wave_data[pos++] = 't';
  wave_data[pos++] = ' ';
  /* Subchunk1Size*/
  wave_data[pos++] = 16;
  wave_data[pos++] = 0;
  wave_data[pos++] = 0;
  wave_data[pos++] = 0;
  /* AudioFormat */
  wave_data[pos++] = 1;
  wave_data[pos++] = 0;
  /* NumChannels */
  wave_data[pos++] = 1;
  wave_data[pos++] = 0;
  /* SampleRate */
  wave_data[pos++] = sample_rate & 0xff;
  wave_data[pos++] = (sample_rate>>8) & 0xff;
  wave_data[pos++] = (sample_rate>>16) & 0xff;
  wave_data[pos++] = (sample_rate>>24) & 0xff;
  /* ByteRate */
  wave_data[pos++] = sample_rate & 0xff;
  wave_data[pos++] = (sample_rate>>8) & 0xff;
  wave_data[pos++] = (sample_rate>>16) & 0xff;
  wave_data[pos++] = (sample_rate>>24) & 0xff;
  /* BlockAlign */
  wave_data[pos++] = 1;
  wave_data[pos++] = 0;
  /* BitsPerSample */
  wave_data[pos++] = 8;
  wave_data[pos++] = 0;
  /* Subchunk2ID */
  wave_data[pos++] = 'd';
  wave_data[pos++] = 'a';
  wave_data[pos++] = 't';
  wave_data[pos++] = 'a';
  /* Subchunk2Size */
  wave_data[pos++] = subchunk2_size & 0xff;
  wave_data[pos++] = (subchunk2_size>>8) & 0xff;
  wave_data[pos++] = (subchunk2_size>>16) & 0xff;
  wave_data[pos++] = (subchunk2_size>>24) & 0xff;

  /* Padding */
  if (wave_data.size() & 1)
    wave_data.push_back(0);
}

bool PatchData::generate_wave() {
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
  wave_data.resize(WAVE_HEADER_LEN);

  for (size_t i = 0; i < data.size(); i += 3) {
    for (int delay = data[i]; delay; delay--) {
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
        int8_t sample = waves[wave][next_sample>>8];
        next_sample += track_step;
        int16_t v16 = (int16_t) sample * vol;
        /* Signed extention */
        int8_t v8 = v16 / 256;
        wave_data.push_back((int) v8 + 128);
      }
    }

    if (data[i+1] == PATCH_END || data[i+1] == PC_NOTE_CUT)
      break;

    int current;
    int target;
    switch (data[i+1]) {
      case PC_ENV_SPEED:
        envelope_step = data[i+2];
        break;

      /* TODO */
      case PC_NOISE_PARAMS:
        break;

      case PC_WAVE:
        wave = data[i+2];
        if (wave >= NUM_WAVES) {
          return false;
        }
        break;

      case PC_NOTE_UP:
        note += data[i+2];
        if (note > 126 || note < 0) {
          return false;
        }
        track_step = step_table[(int) note];
        break;

      case PC_NOTE_DOWN:
        note -= data[i+2];
        if (note > 126 || note < 0) {
          return false;
        }
        track_step = step_table[(int) note];
        break;

      /* TODO */
      case PC_NOTE_HOLD:
        break;

      case PC_ENV_VOL:
        envelope_volume = data[i+2];
        break;

      case PC_PITCH:
        note = data[i+2];
        if (note > 126 || note < 0) {
          return false;
        }
        track_step = step_table[(int) note];
        sliding = false;
        break;

      case PC_TREMOLO_LEVEL:
        tremolo_level = data[i+2];
        break;

      case PC_TREMOLO_RATE:
        tremolo_rate = data[i+2];
        break;

      case PC_SLIDE:
        current = step_table[(int) note];
        slide_note = note + data[i+2];
        if (slide_note > 126 || slide_note < 0) {
          return false;
        }
        target = step_table[(int) slide_note];
        slide_step = std::max(1, (target-current)/slide_speed);
        track_step += slide_step;
        break;

      case PC_SLIDE_SPEED:
        slide_speed = data[i+2];
        break;

      case PC_LOOP_END:
        if (loop_count <= 0)
          break;
        else if (data[i+2] > 0) {
          i -= 3*(i+1);
        }
        else {
          do {
            i -= 3;
          } while(i >= 4 && data[i+1] != PC_LOOP_START);
        }
        break;

      case PC_LOOP_START:
        loop_count = data[i+2];

      default:
        break;
    }
  }

  add_headers();

  return true;
}
