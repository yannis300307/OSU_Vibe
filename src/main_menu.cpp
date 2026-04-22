#include "includes/main_menu.h"

bool MyMenuLayer::init() {
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
    
    this->removeChildByID("more-games-menu"); // Sorry but this button is currently useless

    auto music_player = MusicPlayer::create();
    music_player->setPosition({456, 20});
    music_player->setID("music-player-menu");

    this->addChild(music_player);

    this->updateLayout();

    return true;
}

void MyMenuLayer::update_beats(float dt) {
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
        // All the objects in beats_menu are the same so we have to acess them using index
        auto beat = beats_menu->getChildByIndex(i);
        beat->setScaleX(amplitude * BEATS_LENGHT);
        beat->updateLayout();
    }

    this->updateLayout();
}
