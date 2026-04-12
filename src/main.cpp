#include "Geode/c++stl/string.hpp"
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/base_nodes/CCNode.h"
#include "Geode/cocos/cocoa/CCGeometry.h"
#include "Geode/cocos/layers_scenes_transitions_nodes/CCTransition.h"
#include "Geode/cocos/sprite_nodes/CCSprite.h"
#include "Geode/loader/Log.hpp"
#include "Geode/utils/addresser.hpp"
#include "fmod.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/AppDelegate.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/cocos/draw_nodes/CCDrawingPrimitives.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <Geode/modify/MenuLayer.hpp>
#include <vector>
#include <Geode/modify/AppDelegate.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/CreatorLayer.hpp>

using namespace geode::prelude;


const unsigned int BEATS_AVERGAGE = 2;
const float BEAT_WIDTH = 0.12f;
const unsigned int SAMPLES_COUNT = 36;
const float BEATS_CIRLCE_RADIUS = 57.0f;
const float BASE_PLAY_BUTTON_SCALE = 0.12f;
const float BEATS_LENGHT = 2.0f;
const float BEATS_DECAY_FACTOR = 1.0f;
const float OFFSET_ROTATION_SPEED = 50.0f;

const unsigned int BEATS_AMOUNT = 360 / BEATS_AVERGAGE;

float old_background_music_volume = 0.0f;

bool game_launched = false;

gd::string current_music_path;

bool block_music_stop;

class $modify(MyAudioEngine, FMODAudioEngine)
{	
	void update(float dt)
	{
		FMODAudioEngine::update(dt);
		if (!game_launched)
			return;

		bool isplaying;
		this->m_backgroundMusicChannel->isPlaying(&isplaying);
		if (!isplaying)
		{
			geode::log::info("Music finished picking another one!");
			this->play_random_music();
		}
	}

	void play_random_music()
	{
		auto music_manager = MusicDownloadManager::sharedState();

		bool isplaying;
		this->m_backgroundMusicChannel->isPlaying(&isplaying);
		if (!isplaying)
		{
			std::vector<int> songs;
			for (auto* song : CCArrayExt<SongInfoObject*>(music_manager->getDownloadedSongs()))
			{
				if (song->m_songID < 10000000)
					songs.push_back(song->m_songID);
			}

			int song_id = songs[random::nextU64() % songs.size()];
			auto song_path = music_manager->pathForSong(song_id);
			this->stopAllMusic(true);
			current_music_path = song_path;

			this->playMusic(song_path, false, 0.0f, 0);
			geode::log::info("Let's play music {}!", song_id);
		}
	}
};

class $modify(MyCreatorLayer, CreatorLayer)
{
	void onBack(CCObject* sender) {
		unsigned int backgroundMusicPosition;

		FMOD::Channel *channel = nullptr;
		auto fmod = FMODAudioEngine::sharedEngine();
		auto result = fmod->m_backgroundMusicChannel->getChannel(0, &channel);
		if (result == FMOD_OK && channel) {
			channel->getPosition(&backgroundMusicPosition, FMOD_TIMEUNIT_MS);
		}

		block_music_stop = true;
		
		CreatorLayer::onBack(sender);

		fmod->resumeAllMusic();

		//block_music_stop = false;

		/*auto main_menu = MenuLayer::scene(false);
		auto fader = CCTransitionFade::create(0.5, main_menu);
		auto director = CCDirector::sharedDirector();
		if (!director->replaceScene(fader))
			return ;*/

		//sender->release()

		//fmod->set
		
		/*if (result == FMOD_OK && channel) {
			fmod->playMusic(current_music_path, false, 0.0f, 0);
			geode::log::debug("pos: {}", backgroundMusicPosition);
			channel->setPosition(backgroundMusicPosition, FMOD_TIMEUNIT_MS);
		}*/
	}
};

class $modify(AppDelegate)
{
	void applicationWillEnterForeground() {
        AppDelegate::applicationWillEnterForeground();
		auto fmod = FMODAudioEngine::sharedEngine();
		fmod->setBackgroundMusicVolume(old_background_music_volume);
    }

    // Keep the music playing in the background
    void applicationDidEnterBackground() {
		auto fmod = FMODAudioEngine::sharedEngine();
        AppDelegate::applicationDidEnterBackground();
		old_background_music_volume = fmod->getBackgroundMusicVolume();
		fmod->setBackgroundMusicVolume(old_background_music_volume / 3.0f);
		fmod->resumeAllMusic();
    }
};

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
        FMOD::DSP* m_fftDSP = nullptr;
		float beats_amplitude[BEATS_AMOUNT];
		float beats_offset = 0.;
    };

	bool init() {
		// Call the default function
		if (!MenuLayer::init()) {
			return false;
		}
		geode::log::debug("in main menu");

		game_launched = true;
		
		auto main_menu = this->getChildByID("main-menu");
		auto play_button = main_menu->getChildByID("play-button");
		auto sprite = play_button->getChildByIndex(0);

		// Remove the default sprite for the play button
		play_button->removeAllChildren();

		// Replace the sprite with our round version of the button
		auto new_sprite = CCSprite::create("play_button.png"_spr);
		new_sprite->setScale(BASE_PLAY_BUTTON_SCALE);
		new_sprite->setPosition({play_button->getContentWidth() / 2.0f, play_button->getContentHeight() / 2.0f});
		play_button->setZOrder(10);
		play_button->addChild(new_sprite);

		// Setup the beats nodes pool
		auto beats_pool = CCNode::create();
		beats_pool->setPosition({main_menu->getContentWidth() / 2.0f, main_menu->getContentHeight() / 2.0f});
		beats_pool->setID("beats-menu");
		beats_pool->setAnchorPoint({0.5f, 0.5f});
		beats_pool->setZOrder(-1);
		
		// Create the beats
		for (int i = 0; i < 360; i += BEATS_AVERGAGE)
		{
			float angle = i * std::numbers::pi / 180.f;

			auto sprite = CCSprite::create("beat.png"_spr);
			
			sprite->setPositionX(BEATS_CIRLCE_RADIUS * cosf(angle));
			sprite->setPositionY(BEATS_CIRLCE_RADIUS * sinf(angle));
			sprite->setScaleY(BEAT_WIDTH);
			sprite->setAnchorPoint({0.0f, 0.5f});
			sprite->setRotation(-static_cast<float>(i));
			sprite->setOpacity(200);
			beats_pool->addChild(sprite);
		}
		main_menu->addChild(beats_pool);

		this->updateLayout();

		// Reset the amplitude values
		std::fill(this->m_fields->beats_amplitude, this->m_fields->beats_amplitude + BEATS_AMOUNT, 0.0f);

		// Setup the DSPs for volume and spectrum analysis
		auto engine = FMODAudioEngine::sharedEngine();
        auto system = engine->m_system;
        FMOD::ChannelGroup* channel = engine->m_backgroundMusicChannel;
		
		// Spectrum
        if (channel && system) {
            system->createDSPByType(FMOD_DSP_TYPE_FFT, &m_fields->m_fftDSP);
            m_fields->m_fftDSP->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, SAMPLES_COUNT);
            channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, m_fields->m_fftDSP);
        }

		// Volume
		if (channel->getDSP(FMOD_CHANNELCONTROL_DSP_TAIL, &m_fields->m_fftDSP) == FMOD_OK) {
			m_fields->m_fftDSP->setMeteringEnabled(true, false);
		}

		this->schedule(schedule_selector(MyMenuLayer::update_beats));

		return true;
	}
	
	void update_beats(float dt) {
		// Get the volume
		FMOD_DSP_METERING_INFO info;
		if (m_fields->m_fftDSP->getMeteringInfo(&info, nullptr) != FMOD_OK) {
			return ;
		}

		float volume = 0.0f;
		if (info.numchannels > 0) {
			for (int i = 0; i < info.numchannels; i++) {
				volume += info.rmslevel[i] * info.rmslevel[i];
			}
			volume = std::sqrt(volume / info.numchannels);
		}

		// Get the spectrum
		if (!m_fields->m_fftDSP) return;

		FMOD_DSP_PARAMETER_FFT* fftData;
		unsigned int length;

		auto res = m_fields->m_fftDSP->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void**)&fftData, &length, nullptr, 0);
		
		if (res == FMOD_OK && fftData->spectrum[0]) {			
			for (int i = 0; i < SAMPLES_COUNT; i++) {
				unsigned int ampl_index = (i - static_cast<int>(this->m_fields->beats_offset)) % SAMPLES_COUNT;
				float amplitude = fftData->spectrum[0][ampl_index];
				amplitude = powf(amplitude, 1.0f / 3.0f);
				float old_amplitude = this->m_fields->beats_amplitude[i];
				if (amplitude > old_amplitude)
					this->m_fields->beats_amplitude[i] = amplitude;	
			}
		}

		// Rotate the beats
		this->m_fields->beats_offset += dt * OFFSET_ROTATION_SPEED;
		if (this->m_fields->beats_offset > SAMPLES_COUNT)
		{
			this->m_fields->beats_offset = 0.0f;
		}

		// Fetch the nodes
		auto main_menu = this->getChildByID("main-menu");
		auto play_button = main_menu->getChildByID("play-button");
		auto sprite = play_button->getChildByIndex(0);

		auto beats_menu = main_menu->getChildByID("beats-menu");

		// Scale the play button based on the music volume
		sprite->setScale(BASE_PLAY_BUTTON_SCALE + volume * 0.01f);

		// Edit the lenght of the beats based on the spectrum
		for (int i = 0; i < BEATS_AMOUNT; i++)
		{
			unsigned int ampl_index = i % SAMPLES_COUNT;
			float amplitude = this->m_fields->beats_amplitude[ampl_index];
			this->m_fields->beats_amplitude[ampl_index] -= dt * BEATS_DECAY_FACTOR * ((1.0f - amplitude) * 0.8f + 0.2f);
			if (this->m_fields->beats_amplitude[ampl_index] < 0.0)
				this->m_fields->beats_amplitude[ampl_index] = 0.0; 
			auto beat = beats_menu->getChildByIndex(i);
			beat->setScaleX(amplitude * BEATS_LENGHT);
			beat->updateLayout();
		}

		this->updateLayout();
    }
};