#pragma once

#include "geode_includes.h"
#include "utils.h"
#include "audio_engine.h"

using namespace geode::prelude;

class MusicPlayer : public CCMenu {
	protected:
		int old_music_id = -1;
		bool is_mouse_down = false;

		bool init();
        bool mouse_touches_progress_bar();
        void search_song(int songID, bool isCustom);
        void refresh_cover_image(int music_id);


	public:
		bool ccTouchBegan(CCTouch* touch, CCEvent* event);
        void ccTouchEnded(CCTouch* touch, CCEvent* event);
        void ccTouchCancelled(CCTouch* touch, CCEvent* event);
        void update_menu(float dt);
        static MusicPlayer* create();
        void next_music(CCObject* sender);
        void previous_music(CCObject* sender);
        void search_current_song(CCObject* sender);
        void open_current_song_url(CCObject* sender);
};