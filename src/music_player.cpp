#include "includes/music_player.h"

bool MusicPlayer::init()
{
    if (!CCMenu::init())
        return false;

    this->setContentSize({100.0f, 50.0f});
    auto center = this->getContentSize() / 2.0f;

    auto music_player_menu_background = CCSprite::create("music_player_container.png"_spr);
    music_player_menu_background->setPosition(center);
    music_player_menu_background->setScale(0.1f);

    this->addChild(music_player_menu_background);

    auto next_sprite = CCSprite::create("next_icon.png"_spr);
    next_sprite->setScale(0.05f);

    auto music_player_next_music = CCMenuItemSpriteExtra::create(
        next_sprite, this, menu_selector(MusicPlayer::next_music)
    );
    music_player_next_music->setPosition({38 + center.width, -16 + center.height});

    this->addChild(music_player_next_music);

    auto previous_sprite = CCSprite::create("next_icon.png"_spr);
    previous_sprite->setScale(0.05f);
    previous_sprite->setFlipX(true);

    auto music_player_previous_music = CCMenuItemSpriteExtra::create(
        previous_sprite, this, menu_selector(MusicPlayer::previous_music)
    );
    music_player_previous_music->setPosition({-38 + center.width, -16 + center.height});

    this->addChild(music_player_previous_music);

    // @geode-ignore(unknown-resource)
    auto music_id_sprite = CCLabelBMFont::create("ID: test", "MusicPlayerFont.fnt"_spr);
    auto music_id = CCMenuItemSpriteExtra::create(
        music_id_sprite, this, menu_selector(MusicPlayer::search_current_song)
    );
    music_id->setID("music-id");
    music_id->setPosition({0 + center.width, -24 + center.height});

    this->addChild(music_id);

    // @geode-ignore(unknown-resource)
    auto music_name_sprite = CCLabelBMFont::create("Name test", "MusicPlayerFont.fnt"_spr);
    music_name_sprite->setWidth(50.0f);
    music_name_sprite->setAlignment(CCTextAlignment::kCCTextAlignmentRight);
    auto music_name = CCMenuItemSpriteExtra::create(
        music_name_sprite, this, menu_selector(MusicPlayer::open_current_song_url)
    );
    music_name->setID("music-name");
    music_name->setPosition({19 + center.width, 16 + center.height});
    music_name->setContentSize({57, 25});
    music_name_sprite->setPosition(music_name->getContentSize() / 2.0f);

    this->addChild(music_name);

    // @geode-ignore(unknown-resource)
    auto artist_name_sprite = CCLabelBMFont::create("name", "MusicPlayerFont.fnt"_spr);
    auto artist_name = CCMenuItemSpriteExtra::create(
        artist_name_sprite, this, nullptr
    );
    artist_name->setID("artist-name");
    artist_name->setPosition({17 + center.width, -1 + center.height});
    artist_name_sprite->setWidth(15);

    this->addChild(artist_name);

    auto vinyl_sprite = CCSprite::create("vinyl.png"_spr);
    vinyl_sprite->setScale(0.125f);
    vinyl_sprite->setID("vinyl");
    vinyl_sprite->setPosition({-28 + center.width, 11 + center.height});
    this->addChild(vinyl_sprite);

    auto vinyl_head_sprite = CCSprite::create("vinyl_head.png"_spr);
    vinyl_head_sprite->setScale(0.125f);
    vinyl_head_sprite->setPosition({-17.0f + center.width, 29.0f + center.height});
    vinyl_head_sprite->setAnchorPoint({1.0f, 1.0f});
    vinyl_head_sprite->setID("vinyl-head");
    vinyl_head_sprite->setZOrder(3);
    this->addChild(vinyl_head_sprite);

    auto progress_bar_left_edge_sprite = CCSprite::create("progress_edge.png"_spr);
    progress_bar_left_edge_sprite->setScale(0.1f);
    progress_bar_left_edge_sprite->setPosition({-24.9f + center.width, -16.45f + center.height});
    progress_bar_left_edge_sprite->setAnchorPoint({1.0f, 0.5f});
    progress_bar_left_edge_sprite->setID("progress-bar-left-edge");
    this->addChild(progress_bar_left_edge_sprite);

    auto progress_bar_sprite = CCSprite::create("progress_bar.png"_spr);
    progress_bar_sprite->setScale(0.1f);
    progress_bar_sprite->setPosition({-24.9f + center.width, -16.45f+ center.height});
    progress_bar_sprite->setAnchorPoint({0.0f, 0.5f});
    progress_bar_sprite->setID("progress-bar");
    this->addChild(progress_bar_sprite);

    auto progress_bar_right_edge_sprite = CCSprite::create("progress_edge.png"_spr);
    progress_bar_right_edge_sprite->setScale(0.1f);
    progress_bar_right_edge_sprite->setFlipX(true);
    progress_bar_right_edge_sprite->setPosition({24.9f + center.width, -16.45f + center.height});
    progress_bar_right_edge_sprite->setAnchorPoint({0.0f, 0.5f});
    progress_bar_right_edge_sprite->setID("progress-bar-right-edge");
    this->addChild(progress_bar_right_edge_sprite);

    auto vinyl_center = CCPoint(-28 + center.width, 11 + center.height);
    auto mask = CCSprite::create("vinyl_mask.png"_spr);
    mask->setScale(0.125f);
    mask->setID("vinyl");
    auto vinyl_clipping = CCClippingNode::create();
    vinyl_clipping->setPosition({-28 + center.width, 11 + center.height});
    //vinyl_clipping->setContentSize(m_bgSprite->getContentSize());
    vinyl_clipping->setStencil(mask);
    vinyl_clipping->setID("custom-vinyl");
    vinyl_clipping->setZOrder(1);
    vinyl_clipping->setAlphaThreshold(0.5f);
    this->addChild(vinyl_clipping);

    auto vinyl_middle = CCSprite::create("vinyl_middle.png"_spr);
    vinyl_middle->setPosition(vinyl_center);
    vinyl_middle->setScale(0.125f);
    vinyl_middle->setZOrder(2);
    this->addChild(vinyl_middle);

    auto vinyl_outline = CCSprite::create("vinyl_outline.png"_spr);
    vinyl_outline->setPosition(vinyl_center);
    vinyl_outline->setScale(0.125f);
    vinyl_outline->setZOrder(2);
    this->addChild(vinyl_outline);

    this->schedule(schedule_selector(MusicPlayer::update_menu));

    this->updateLayout();

    refresh_cover_image(-1);

    return true;
}

bool MusicPlayer::mouse_touches_progress_bar()
{
    auto progress_bar_sprite = this->getChildByID("progress-bar");
    auto mouse_pos = getMousePos();
    auto relative_mouse_pos = mouse_pos - progress_bar_sprite->convertToWorldSpace(CCPointZero);
    auto actual_size = progress_bar_sprite->getContentSize() * 0.1f;

    return relative_mouse_pos.x > 0.0f && relative_mouse_pos.x < actual_size.width && relative_mouse_pos.y > 0 && relative_mouse_pos.y < actual_size.height;
}

void MusicPlayer::search_song(int songID, bool isCustom) {
    auto search_obj = GJSearchObject::create(SearchType::Search);
    
    search_obj->m_songID = songID;
    search_obj->m_songFilter = true;
    search_obj->m_customSongFilter = isCustom;
    
    auto scene = LevelBrowserLayer::scene(search_obj);
    
    CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void MusicPlayer::refresh_cover_image(int music_id)
{
    if (music_id < 0)
    {
        auto engine = FMODAudioEngine::sharedEngine();
        auto custom_engine = static_cast<MyAudioEngine *>(engine);

        music_id = custom_engine->m_fields->songs[custom_engine->m_fields->current_music];
    }

    auto vinyl_clipping = this->getChildByID("custom-vinyl");
    vinyl_clipping->removeAllChildren();

    auto vinyl_image = LazySprite::create({32, 32}, false);
    vinyl_image->setAutoResize(true);
    vinyl_image->setPosition({0, 0});
    auto url = get_link_from_music_id(music_id, "png");
    vinyl_image->loadFromUrl(url);
    vinyl_image->setLoadCallback([vinyl_image, music_id](Result<> res) {
    if (res) {
        geode::log::info("Cover loaded successfully! PNG is fine.");
        vinyl_image->setVisible(true);
    } else {
        geode::log::info("Png not found for cover. Trying Webp. Otherwise, we can't do much more.");
        
        
        vinyl_image->setLoadCallback([vinyl_image, music_id](Result<> res) {
        if (res) {
            geode::log::info("Cover loaded successfully! Webp was definitely the solution!");
            vinyl_image->setVisible(true);
        } else {
            geode::log::info("Webp not found. Sad...");
            vinyl_image->setVisible(false);
        }
        });

        auto url = get_link_from_music_id(music_id, "webp");
        vinyl_image->loadFromUrl(url);
        vinyl_image->setVisible(false);
    }
    });
    vinyl_clipping->addChild(vinyl_image);
}

bool MusicPlayer::ccTouchBegan(CCTouch* touch, CCEvent* event)
{
    if (this->mouse_touches_progress_bar())
    {
        this->is_mouse_down = true;
        return true;
    }
    return CCMenu::ccTouchBegan(touch, event);
}

void MusicPlayer::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    this->is_mouse_down = false;
    CCMenu::ccTouchEnded(touch, event);
}

void MusicPlayer::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    this->is_mouse_down = false;
    CCMenu::ccTouchCancelled(touch, event);
}

void MusicPlayer::update_menu(float dt)
{
    auto engine = FMODAudioEngine::sharedEngine();
    auto custom_engine = static_cast<MyAudioEngine *>(engine);

    auto vinyl_sprite = this->getChildByID("vinyl");
    auto custom_vinyl_sprite = static_cast<LazySprite *>(this->getChildByID("custom-vinyl")->getChildByIndex(0));
    auto vinyl_head_sprite = this->getChildByID("vinyl-head");
    int music_id = custom_engine->m_fields->songs[custom_engine->m_fields->current_music];


    if (music_id != this->old_music_id)
    {
        this->old_music_id = music_id;

        auto music_manager = MusicDownloadManager::sharedState();
        auto music_name = music_manager->getSongInfoObject(music_id)->m_songName;
        auto artist_name = music_manager->getSongInfoObject(music_id)->m_artistName;

        auto music_id_sprite = static_cast<CCLabelBMFont *>(this->getChildByID("music-id")->getChildByIndex(0));

        music_id_sprite->setString(("ID: " + std::to_string(music_id)).c_str());
        music_id_sprite->updateLabel();

        auto music_name_container = this->getChildByID("music-name");
        auto music_name_sprite = static_cast<CCLabelBMFont *>(music_name_container->getChildByIndex(0));

        music_name_sprite->setCString(music_name.c_str());

        music_name_sprite->setScale(1.0f);

        float currentHeight = music_name_sprite->getContentSize().height;
        float max_height = music_name_container->getContentHeight();

        if (currentHeight > max_height) {
            float newScale = max_height / currentHeight;
            music_name_sprite->setScale(newScale);
        }
        music_name_sprite->updateLabel();
        
        auto artist_name_sprite = static_cast<CCLabelBMFont *>(this->getChildByID("artist-name")->getChildByIndex(0));

        artist_name_sprite->setCString(artist_name.c_str());
        artist_name_sprite->updateLabel();

        //custom_vinyl_sprite->cancelLoad();
        //custom_vinyl_sprite->initWithSpriteFrameName("vinyl.png"_spr);

        if (music_id < 10000000) {
            refresh_cover_image(music_id);
            custom_vinyl_sprite = static_cast<LazySprite *>(this->getChildByID("custom-vinyl")->getChildByIndex(0));
        } else if (custom_vinyl_sprite) {
            custom_vinyl_sprite->setVisible(false);
        }
    }

    vinyl_sprite->setRotation(vinyl_sprite->getRotation() + dt * 100.0f);
    if (vinyl_sprite->getRotation() > 360.0f)
        vinyl_sprite->setRotation(0.0f);
    
    if (custom_vinyl_sprite){
        custom_vinyl_sprite->setRotation(custom_vinyl_sprite->getRotation() + dt * 100.0f);
        if (custom_vinyl_sprite->getRotation() > 360.0f)
            custom_vinyl_sprite->setRotation(0.0f);
    }

    FMOD::Channel *channel = nullptr;
    auto result = engine->m_backgroundMusicChannel->getChannel(0, &channel);

    if (result == FMOD_OK && channel) {
        FMOD::Sound* current_sound = nullptr;
        unsigned int music_pos;
        channel->getPosition(&music_pos, FMOD_TIMEUNIT_MS);

        channel->getCurrentSound(&current_sound);
        unsigned int music_lenght;
        current_sound->getLength(&music_lenght, FMOD_TIMEUNIT_MS);
        
        if (music_lenght > 0) {
            float progress =  static_cast<float>(music_pos) / static_cast<float>(music_lenght);
            vinyl_head_sprite->setRotation(-22.0f + 27.0f * progress);

            auto progress_bar_sprite = this->getChildByID("progress-bar");
            auto progress_bar_right_edge_sprite = this->getChildByID("progress-bar-right-edge");

            auto mouse_pos = getMousePos();

            progress_bar_sprite->setScaleX(progress * 0.1f);
            auto relative_mouse_pos = mouse_pos - progress_bar_sprite->convertToWorldSpace(CCPointZero);

            auto actual_size = progress_bar_sprite->getContentSize() * 0.1f;

            if (is_mouse_down && mouse_touches_progress_bar())
            {
                channel->setPaused(true);
                channel->setPosition(music_lenght * relative_mouse_pos.x / actual_size.width, FMOD_TIMEUNIT_MS);
            } else if (!is_mouse_down)
            {
                channel->setPaused(false);
            }

            progress_bar_right_edge_sprite->setPositionX(-actual_size.width / 2.0f + actual_size.width * progress + this->getContentWidth() / 2.0f - 0.05f);

            /*relative_mouse_pos = mouse_pos - vinyl_sprite->convertToWorldSpace(CCPointZero);
            auto radius  = vinyl_sprite->getContentWidth() * 0.125f / 2.0f;
            geode::log::debug("dist : {}", relative_mouse_pos.getDistance(CCPointZero));
            if (is_mouse_down && relative_mouse_pos.x > 0.0f && relative_mouse_pos.getDistance(CCPointZero) < radius)
            {
                geode::log::debug("{}", (mouse_pos - vinyl_sprite->convertToWorldSpace(CCPointZero)).getAngle());
            }*/
        }
    }
}

MusicPlayer* MusicPlayer::create() {
    auto ret = new MusicPlayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

void MusicPlayer::next_music(CCObject* sender)
{
    auto engine = FMODAudioEngine::sharedEngine();
    static_cast<MyAudioEngine *>(engine)->skip_music();
}

void MusicPlayer::previous_music(CCObject* sender)
{
    auto engine = FMODAudioEngine::sharedEngine();
    static_cast<MyAudioEngine *>(engine)->play_previous_music();
}

void MusicPlayer::search_current_song(CCObject* sender)
{
    auto engine = FMODAudioEngine::sharedEngine();
    auto custom_engine = static_cast<MyAudioEngine *>(engine);

    this->search_song(custom_engine->m_fields->songs[custom_engine->m_fields->current_music], true);
}

void MusicPlayer::open_current_song_url(CCObject* sender)
{
    auto engine = FMODAudioEngine::sharedEngine();
    auto custom_engine = static_cast<MyAudioEngine *>(engine);
    int music_id = custom_engine->m_fields->songs[custom_engine->m_fields->current_music];
    auto music_manager = MusicDownloadManager::sharedState();
    auto music = static_cast<SongInfoObject *>(music_manager->m_songObjects->objectForKey(std::to_string(music_id)));
    auto ngArtistUrl = fmt::format("https://{}.newgrounds.com", music->m_artistName);
    if (music_id > 10000000) {
        if (!music->m_youtubeChannel.empty())
            web::openLinkInBrowser(fmt::format("https://www.youtube.com/channel/{}", music->m_youtubeChannel));
        else
        {

        geode::createQuickPopup(
            "Music not found",
            "Sorry, I can't find anything for this music. Would you like to search it on Youtube?",
            "No", "Yes",
            [music](auto, bool btn2) {
                if (btn2) {
                    web::openLinkInBrowser(fmt::format("https://www.youtube.com/results?search_query={}", url_encode(fmt::format("{} - {}", music->m_songName, music->m_artistName))));
                }
            }
        );
        };
    } else {
        web::openLinkInBrowser(fmt::format("https://www.newgrounds.com/audio/listen/", music_id));
    }
}