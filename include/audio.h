/* -*- c++ -*- */
#ifndef MARCAI_AUDIO_H
#define MARCAI_AUDIO_H

#define PICO_AUDIO_I2S_CLOCK_PIN_BASE 26
#define PICO_AUDIO_I2S_DATA_PIN       28

#include "pico/audio.h"
#include "mp3dec.h"

#include <cstdint>

class Audio {
public:
  Audio();
  ~Audio();

  /** \brief Initialise I2S audio
   */
  bool init_i2s();

  /** \brief Decode one frame of mp3 and play it
   * \param data buffer with mp3 data
   * \param size count of available buffer data
   * \return Size of consumed bytes or -1 on error
   */
  int stream_decode(uint8_t *data, int size);

  void play();
  void pause();
private:
  enum {
    kMaxFrameSize = 1152 * 2, // Size of one stereo frame
  };

  HMP3Decoder decoder_;
  audio_buffer_pool_t *buffer_pool_;
  audio_buffer_format_t producer_format_;
  audio_format_t audio_format_;
};


#endif /* MARCAI_AUDIO_H */
