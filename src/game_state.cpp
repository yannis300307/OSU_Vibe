#include "includes/game_state.h"

void MyAppDelegate::applicationWillEnterForeground() {
	AppDelegate::applicationWillEnterForeground();
	auto fmod = FMODAudioEngine::sharedEngine();
	fmod->setBackgroundMusicVolume(old_background_music_volume);
}

void MyAppDelegate::applicationDidEnterBackground() {
	auto fmod = FMODAudioEngine::sharedEngine();
	AppDelegate::applicationDidEnterBackground();
	old_background_music_volume = fmod->getBackgroundMusicVolume();
	fmod->setBackgroundMusicVolume(old_background_music_volume / 3.0f);
	fmod->resumeAllMusic();
}
