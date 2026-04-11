#include "Geode/cocos/base_nodes/CCNode.h"
#include "Geode/cocos/cocoa/CCGeometry.h"
#include "Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h"
#include "Geode/cocos/menu_nodes/CCMenu.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "Geode/cocos/sprite_nodes/CCSprite.h"
#include "Geode/loader/Log.hpp"
#include "fmod.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/cocos/draw_nodes/CCDrawingPrimitives.h>
#include <algorithm>
#include <cmath>
#include <numbers>


using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>


float getCurrentMusicVolume() {
    auto engine = FMODAudioEngine::sharedEngine();
    
    FMOD::ChannelGroup* channel = engine->m_backgroundMusicChannel;
    if (!channel) return 0.0f;

    FMOD::DSP* dsp;
    if (channel->getDSP(FMOD_CHANNELCONTROL_DSP_TAIL, &dsp) != FMOD_OK) {
        return 0.0f;
    }

    dsp->setMeteringEnabled(true, false);

    FMOD_DSP_METERING_INFO info;
    if (dsp->getMeteringInfo(&info, nullptr) != FMOD_OK) {
        return 0.0f;
    }

    float rms = 0.0f;
    if (info.numchannels > 0) {
        for (int i = 0; i < info.numchannels; i++) {
            rms += info.rmslevel[i] * info.rmslevel[i];
        }
        rms = std::sqrt(rms / info.numchannels);
    }

    return rms;
}

const unsigned int BEATS_AVERGAGE = 2;
const float BEAT_WIDTH = 0.12f;
const unsigned int SAMPLES_COUNT = 36;
const float BEATS_CIRLCE_RADIUS = 57.0f;
const float BASE_PLAY_BUTTON_SCALE = 0.12f;
const float BEATS_LENGHT = 2.0f;
const float BEATS_DECAY_FACTOR = 1.0f;
const float OFFSET_ROTATION_SPEED = 50.0f;

const unsigned int BEATS_AMOUNT = 360 / BEATS_AVERGAGE;

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
        FMOD::DSP* m_fftDSP = nullptr;
		float beats_amplitude[BEATS_AMOUNT];
		float beats_offset = 0.;
    };

	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}
		
		auto main_menu = this->getChildByID("main-menu");

		auto play_button = main_menu->getChildByID("play-button");
		auto sprite = play_button->getChildByIndex(0);

		play_button->removeAllChildren();

		auto new_sprite = CCSprite::create("play_button.png"_spr);
		new_sprite->setScale(BASE_PLAY_BUTTON_SCALE);
		new_sprite->setPosition({play_button->getContentWidth() / 2.0f, play_button->getContentHeight() / 2.0f});
		play_button->setZOrder(10);
		play_button->addChild(new_sprite);


		auto beats_menu = CCNode::create();
		beats_menu->setPosition({main_menu->getContentWidth() / 2.0f, main_menu->getContentHeight() / 2.0f});
		beats_menu->setID("beats-menu");
		beats_menu->setAnchorPoint({0.5f, 0.5f});
		beats_menu->setZOrder(-1);
		
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
			beats_menu->addChild(sprite);
		}

		main_menu->addChild(beats_menu);

		this->updateLayout();

		std::fill(this->m_fields->beats_amplitude, this->m_fields->beats_amplitude + BEATS_AMOUNT, 0.0f);

		auto engine = FMODAudioEngine::sharedEngine();
        auto system = engine->m_system; // The core FMOD system
        FMOD::ChannelGroup* channel = engine->m_backgroundMusicChannel;

        if (channel && system) {
            system->createDSPByType(FMOD_DSP_TYPE_FFT, &m_fields->m_fftDSP);
            m_fields->m_fftDSP->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, SAMPLES_COUNT);
            channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, m_fields->m_fftDSP);
        }

		this->schedule(schedule_selector(MyMenuLayer::function_not_already_used));

		return true;
	}
	
	void function_not_already_used(float dt) {
		auto level = getCurrentMusicVolume();

		auto main_menu = this->getChildByID("main-menu");
		auto play_button = main_menu->getChildByID("play-button");
		auto sprite = play_button->getChildByIndex(0);

		auto beats_menu = main_menu->getChildByID("beats-menu");

		//geode::log::debug("dt: {}", dt);

		this->m_fields->beats_offset += dt * OFFSET_ROTATION_SPEED;
		if (this->m_fields->beats_offset > SAMPLES_COUNT)
		{
			this->m_fields->beats_offset = 0.0f;
		}

		sprite->setScale(BASE_PLAY_BUTTON_SCALE + level * 0.01f);

		if (!m_fields->m_fftDSP) return;

		FMOD_DSP_PARAMETER_FFT* fftData;
		unsigned int length;

		auto res = m_fields->m_fftDSP->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void**)&fftData, &length, nullptr, 0);
		
		if (res == FMOD_OK && fftData->spectrum[0]) {			
			for (int i = 0; i < SAMPLES_COUNT; i++) {
				unsigned int ampl_index = (i - static_cast<int>(this->m_fields->beats_offset)) % SAMPLES_COUNT;
				float amplitude = fftData->spectrum[0][ampl_index];
				amplitude = powf(amplitude, 1.0f/3.0f);
				float old_amplitude = this->m_fields->beats_amplitude[i];
				if (amplitude > old_amplitude)
					this->m_fields->beats_amplitude[i] = amplitude;	
			}
		}

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