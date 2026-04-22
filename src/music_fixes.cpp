#include "includes/music_fixes.h"

void MyCreatorLayer::onBack(CCObject* sender) {
    unsigned int backgroundMusicPosition;

    FMOD::Channel *channel = nullptr;
    auto fmod = FMODAudioEngine::sharedEngine();
    auto result = fmod->m_backgroundMusicChannel->getChannel(0, &channel);
    if (result == FMOD_OK && channel) {
        channel->getPosition(&backgroundMusicPosition, FMOD_TIMEUNIT_PCM);
    }
    
    CreatorLayer::onBack(sender);

    fmod->playMusic(current_music_path, false, 0.0f, 0);
    result = fmod->m_backgroundMusicChannel->getChannel(0, &channel);
    if (result == FMOD_OK && channel) {
        geode::log::debug("pos: {}", backgroundMusicPosition);
        channel->setPosition(backgroundMusicPosition, FMOD_TIMEUNIT_PCM);
    }
}
