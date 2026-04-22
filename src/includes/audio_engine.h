#pragma once

#include "geode_includes.h"
#include "globals.h"

class $modify(MyAudioEngine, FMODAudioEngine)
{
	struct Fields
	{
		std::vector<int> songs;
		int current_music = 0;
	};

    public:
        bool create_playlist();
        void update(float dt);
        void play_next_music_if_finished();
        void play_previous_music();
        void skip_music();
};
