#pragma once

#include "geode_includes.h"
#include "constants.h"
#include "globals.h"
#include "music_player.h"
#include <numbers>

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
        FMOD::DSP* m_fftDSP = nullptr;
		float beats_amplitude[BEATS_AMOUNT];
		float beats_offset = 0.;
    };

	bool init();
	void update_beats(float dt);
};