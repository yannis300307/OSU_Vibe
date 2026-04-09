#include "Geode/cocos/base_nodes/CCNode.h"
#include "Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h"
#include "Geode/cocos/menu_nodes/CCMenu.h"
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
    if (channel->getDSP(FMOD_CHANNELCONTROL_DSP_HEAD, &dsp) != FMOD_OK) {
        return 0.0f;
    }

    dsp->setMeteringEnabled(false, true); // (input, output)

    FMOD_DSP_METERING_INFO info;
    if (dsp->getMeteringInfo(nullptr, &info) != FMOD_OK) {
        return 0.0f;
    }

    float rms = 0.0f;
    if (info.numchannels > 0) {
        for (int i = 0; i < info.numchannels; i++) {
            rms += info.rmslevel[i] * info.rmslevel[i];
        }
        rms = std::sqrt(rms / info.numchannels);
    }
	float volume;

	channel->getVolume(&volume);

	rms /= volume;

    return rms;
}

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
        FMOD::DSP* m_fftDSP = nullptr;
		float beats_amplitude[90];
    };

	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}
		
		auto main_menu = this->getChildByID("main-menu");
		auto beats_menu = CCNode::create();
		beats_menu->setPosition({main_menu->getContentWidth() / 2.0f, main_menu->getContentHeight() / 2.0f});
		beats_menu->setID("beats-menu");
		beats_menu->setAnchorPoint({0.5f, 0.5f});
		beats_menu->setZOrder(-1);
		
		for (int i = 0; i < 360; i += 4)
		{
			float angle = i * std::numbers::pi / 180.f;

			auto sprite = CCSprite::createWithSpriteFrameName("block007_bgcolor_012_001.png");
			
			sprite->setPositionX(30.0f * cosf(angle));
			sprite->setPositionY(30.0f * sinf(angle));
			sprite->setScaleY(0.3f);
			sprite->setAnchorPoint({0.0f, 0.5f});
			sprite->setRotation(-static_cast<float>(i));
			beats_menu->addChild(sprite);
		}

		main_menu->addChild(beats_menu);

		this->updateLayout();

		std::fill(this->m_fields->beats_amplitude, this->m_fields->beats_amplitude + 90, 0.0f);

		auto engine = FMODAudioEngine::sharedEngine();
        auto system = engine->m_system; // The core FMOD system
        FMOD::ChannelGroup* channel = engine->m_backgroundMusicChannel;

        if (channel && system) {
            system->createDSPByType(FMOD_DSP_TYPE_FFT, &m_fields->m_fftDSP);
            m_fields->m_fftDSP->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, 20);
            channel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, m_fields->m_fftDSP);
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

		sprite->setScale(1.0 + level * 0.5);

		if (!m_fields->m_fftDSP) return;

		FMOD_DSP_PARAMETER_FFT* fftData;
		unsigned int length;

		auto res = m_fields->m_fftDSP->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void**)&fftData, &length, nullptr, 0);
		
		if (res == FMOD_OK && fftData->spectrum[0]) {			
			for (int i = 0; i < 20; i++) {
				float amplitude = fftData->spectrum[0][i];
				float old_amplitude = this->m_fields->beats_amplitude[i];
				if (amplitude > old_amplitude)
					this->m_fields->beats_amplitude[i] = amplitude;	
			}
		}

		for (int i = 0; i < 90; i++)
		{
			float amplitude = this->m_fields->beats_amplitude[i % 20];
			this->m_fields->beats_amplitude[i % 20] -= dt * 0.03f;
			if (this->m_fields->beats_amplitude[i % 20] < 0.0)
				this->m_fields->beats_amplitude[i % 20] = 0.0; 
			auto beat = beats_menu->getChildByIndex(i);
			beat->setScaleX(2.0f + log10f(1.0f + amplitude * 200.0f) * 6.0f);
			beat->updateLayout();
		}

		sprite->updateLayout();
    }
};