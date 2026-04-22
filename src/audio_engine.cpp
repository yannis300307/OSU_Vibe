#include "includes/audio_engine.h"

bool MyAudioEngine::create_playlist() {
    auto music_manager = MusicDownloadManager::sharedState();
    for (auto* song : CCArrayExt<SongInfoObject*>(music_manager->getDownloadedSongs()))
    {
        if (!song->m_artistName.empty())
            this->m_fields->songs.push_back(song->m_songID);
    }
    
    std::random_device rd;
    std::mt19937 g(rd());

    shuffle(this->m_fields->songs.begin(), this->m_fields->songs.end(), g);
    geode::log::info("Creating playlist");
    return true;
}

void MyAudioEngine::update(float dt)
{
    FMODAudioEngine::update(dt);
    if (!game_launched)
        return;

    bool isplaying;
    this->m_backgroundMusicChannel->isPlaying(&isplaying);
    if (!isplaying)
    {
        geode::log::info("Music finished picking another one!");
        this->play_next_music_if_finished();
    }
}

void MyAudioEngine::play_next_music_if_finished()
{
    auto music_manager = MusicDownloadManager::sharedState();

    bool isplaying;
    this->m_backgroundMusicChannel->isPlaying(&isplaying);
    if (!isplaying)
    {
        this->m_fields->current_music++;
        if (this->m_fields->current_music >= this->m_fields->songs.size())
            this->m_fields->current_music = 0;
        int song_id = this->m_fields->songs[this->m_fields->current_music];

        auto song_path = music_manager->pathForSong(song_id);
        this->stopAllMusic(true);
        current_music_path = song_path;

        this->playMusic(song_path, false, 0.0f, 0);
        geode::log::info("Let's play music {}!", song_id);
    }
}

void MyAudioEngine::play_previous_music()
{
    auto music_manager = MusicDownloadManager::sharedState();

    this->m_fields->current_music--;
    if (this->m_fields->current_music < 0)
        this->m_fields->current_music = this->m_fields->songs.size() - 1;

    int song_id = this->m_fields->songs[this->m_fields->current_music];

    auto song_path = music_manager->pathForSong(song_id);
    this->stopAllMusic(true);
    current_music_path = song_path;

    this->playMusic(song_path, false, 0.0f, 0);
    geode::log::info("Let's play music {}!", song_id);
}

void MyAudioEngine::skip_music()
{
    this->stopAllMusic(true);
    this->play_next_music_if_finished();
}

$execute {
	auto engine = FMODAudioEngine::sharedEngine();
	static_cast<MyAudioEngine *>(engine)->create_playlist();
}