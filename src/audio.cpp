#include "audio.h"
#include "mp3dec.h"
#include "pico/audio.h"
#include "pico/audio_i2s.h"
#include <cstdio>

#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN,         "I2S DIN",
                            PICO_AUDIO_I2S_CLOCK_PIN_BASE,   "I2S BCK",
                            PICO_AUDIO_I2S_CLOCK_PIN_BASE+1, "I2S LRCK"));

Audio::Audio() {
  decoder_ = MP3InitDecoder();

  // Set sample rate and set mono or stereo audio
  audio_format_ = {
    .sample_freq = 48000,
    .format = AUDIO_BUFFER_FORMAT_PCM_S16,
    .channel_count = 1,
  };

  // Sets buffer format which is used in the audio buffer
  producer_format_.format = &audio_format_;
  producer_format_.sample_stride = 2;

  // Check for valid channel connection to speaker and starts
  // processing the buffer
  buffer_pool_ = audio_new_producer_pool(&producer_format_, 10, kMaxFrameSize);
}

Audio::~Audio() {
  MP3FreeDecoder(decoder_);
}

bool Audio::init_i2s() {
  printf("Init I2S audio...");

  audio_i2s_config_t config = {
    .data_pin = PICO_AUDIO_I2S_DATA_PIN,
    .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
    .dma_channel = 2, // dma 0 and 1 used by wifi?
    .pio_sm = 0,
  };

  const audio_format_t *output_format;
  output_format = audio_i2s_setup(&audio_format_, &config);
  if (!output_format) {
    panic("PicoAudio: Unable to open audio device.\n");
  }

  bool ok = audio_i2s_connect(buffer_pool_);
  assert(ok);

  audio_i2s_set_enabled(true);
  printf("DONE\n");
  return false;
}

int Audio::stream_decode(uint8_t *data, int size) {
  // Get sample buffer with no blocking
  audio_buffer_t *buffer = take_audio_buffer(buffer_pool_, false);

  // If no available buffers we return error
  if (!buffer)
    return -1;

  int16_t *samples = reinterpret_cast<int16_t*>(buffer->buffer->bytes);
  int ret = MP3Decode(decoder_, &data, &size, samples, 0);
  if (ERR_MP3_NONE != ret) {
    // No samples were decoded
    buffer->sample_count = 0;
    give_audio_buffer(buffer_pool_, buffer);
    return 0;
  }

  MP3FrameInfo info;
  MP3GetLastFrameInfo(decoder_, &info);

  if (info.nChans == 1) {
    // Rearrange samples to match LRLRLR... so if we have mono sound
    // we skip every second sample
    for (int i = info.outputSamps - 1; i > 0; --i) {
      samples[i * 2] = samples[i];
      //samples[i * 2 + 1] = 0;
    }
  }

  // Set decoded sample count and release buffer
  buffer->sample_count = info.outputSamps / info.nChans;
  give_audio_buffer(buffer_pool_, buffer);
  return info.size;
}
