#ifndef AUDIO_H
#define AUDIO_H


#include "../stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif
	bool audio_init(double refreshra, float input_srate, float fps);
	void audio_destroy();
	void audio_mix(const int16_t* samples, size_t sample_count);
#ifdef __cplusplus
}
#endif

#endif AUDIO_H