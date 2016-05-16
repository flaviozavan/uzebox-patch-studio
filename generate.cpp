#include <wx/vector.h>
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include "generate.h"
#include "waves.h"
#include "step_table.h"

static void add_headers(wxVector<uint8_t> &data) {
  size_t data_size = data.size() - WAVE_HEADER_LEN;
  const uint32_t subchunk2_size = data_size & 1? data_size+1 : data_size;
  const uint32_t chunk_size = subchunk2_size + 36;
  const uint32_t sample_rate = SAMPLE_RATE;
  int pos = 0;

  /* ChunkID */
  data[pos++] = 'R';
  data[pos++] = 'I';
  data[pos++] = 'F';
  data[pos++] = 'F';
  /* ChunkSize */
  data[pos++] = chunk_size & 0xff;
  data[pos++] = (chunk_size>>8) & 0xff;
  data[pos++] = (chunk_size>>16) & 0xff;
  data[pos++] = (chunk_size>>24) & 0xff;
  /* Format */
  data[pos++] = 'W';
  data[pos++] = 'A';
  data[pos++] = 'V';
  data[pos++] = 'E';
  /* Subchunk1ID */
  data[pos++] = 'f';
  data[pos++] = 'm';
  data[pos++] = 't';
  data[pos++] = ' ';
  /* Subchunk1Size*/
  data[pos++] = 16;
  data[pos++] = 0;
  data[pos++] = 0;
  data[pos++] = 0;
  /* AudioFormat */
  data[pos++] = 1;
  data[pos++] = 0;
  /* NumChannels */
  data[pos++] = 1;
  data[pos++] = 0;
  /* SampleRate */
  data[pos++] = sample_rate & 0xff;
  data[pos++] = (sample_rate>>8) & 0xff;
  data[pos++] = (sample_rate>>16) & 0xff;
  data[pos++] = (sample_rate>>24) & 0xff;
  /* ByteRate */
  data[pos++] = sample_rate & 0xff;
  data[pos++] = (sample_rate>>8) & 0xff;
  data[pos++] = (sample_rate>>16) & 0xff;
  data[pos++] = (sample_rate>>24) & 0xff;
  /* BlockAlign */
  data[pos++] = 1;
  data[pos++] = 0;
  /* BitsPerSample */
  data[pos++] = 8;
  data[pos++] = 0;
  /* Subchunk2ID */
  data[pos++] = 'd';
  data[pos++] = 'a';
  data[pos++] = 't';
  data[pos++] = 'a';
  /* Subchunk2Size */
  data[pos++] = subchunk2_size & 0xff;
  data[pos++] = (subchunk2_size>>8) & 0xff;
  data[pos++] = (subchunk2_size>>16) & 0xff;
  data[pos++] = (subchunk2_size>>24) & 0xff;

  /* Padding */
  if (data.size() & 1)
    data.push_back(0);
}

bool generate_wave(const wxVector<long> &patch, wxVector<uint8_t> &data) {
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
  data.resize(WAVE_HEADER_LEN);

  for (size_t i = 0; i < patch.size(); i += 3) {
    for (int delay = patch[i]; delay; delay--) {
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
        data.push_back((int) v8 + 128);
      }
    }

    if (patch[i+1] == PATCH_END || patch[i+1] == PC_NOTE_CUT)
      break;

    int current;
    int target;
    switch (patch[i+1]) {
      case PC_ENV_SPEED:
        envelope_step = patch[i+2];
        break;

      /* TODO */
      case PC_NOISE_PARAMS:
        break;

      case PC_WAVE:
        wave = patch[i+2];
	if (wave >= NUM_WAVES) {
	  return false;
	}
        break;

      case PC_NOTE_UP:
        note += patch[i+2];
	if (note > 126 || note < 0) {
	  return false;
	}
        track_step = step_table[(int) note];
        break;

      case PC_NOTE_DOWN:
        note -= patch[i+2];
	if (note > 126 || note < 0) {
	  return false;
	}
        track_step = step_table[(int) note];
        break;

      /* TODO */
      case PC_NOTE_HOLD:
        break;

      case PC_ENV_VOL:
        envelope_volume = patch[i+2];
        break;

      case PC_PITCH:
        note = patch[i+2];
	if (note > 126 || note < 0) {
	  return false;
	}
        track_step = step_table[(int) note];
        sliding = false;
        break;

      case PC_TREMOLO_LEVEL:
        tremolo_level = patch[i+2];
        break;

      case PC_TREMOLO_RATE:
        tremolo_rate = patch[i+2];
        break;

      case PC_SLIDE:
        current = step_table[(int) note];
        slide_note = note + patch[i+2];
	if (slide_note > 126 || slide_note < 0) {
	  return false;
	}
        target = step_table[(int) slide_note];
        slide_step = std::max(1, (target-current)/slide_speed);
        track_step += slide_step;
        break;

      case PC_SLIDE_SPEED:
        slide_speed = patch[i+2];
        break;

      case PC_LOOP_END:
        if (loop_count <= 0)
          break;
        else if (patch[i+2] > 0) {
          i -= 3*(i+1);
        }
        else {
          do {
            i -= 3;
          } while(i >= 4 && patch[i+1] != PC_LOOP_START);
        }
        break;

      case PC_LOOP_START:
        loop_count = patch[i+2];

      default:
        break;
    }
  }

  add_headers(data);

  return true;
}
