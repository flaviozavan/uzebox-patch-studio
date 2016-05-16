#define SAMPLE_RATE 15734
#define SAMPLES_PER_FRAME ((SAMPLE_RATE)/60)
#define DEFAULT_VOLUME 0xff

#define WAVE_HEADER_LEN 44

#define PC_ENV_SPEED 0
#define PC_NOISE_PARAMS 1
#define PC_WAVE 2
#define PC_NOTE_UP 3
#define PC_NOTE_DOWN 4
#define PC_NOTE_CUT 5
#define PC_NOTE_HOLD 6
#define PC_ENV_VOL 7
#define PC_PITCH 8
#define PC_TREMOLO_LEVEL 9
#define PC_TREMOLO_RATE 10
#define PC_SLIDE 11
#define PC_SLIDE_SPEED 12
#define PC_LOOP_START 13
#define PC_LOOP_END 14
#define PATCH_END 15

bool generate_wave(const wxVector<long> &patch, wxVector<uint8_t> &data);
