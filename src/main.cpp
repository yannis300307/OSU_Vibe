#include "Geode/DefaultInclude.hpp"
#include "Geode/c++stl/string.hpp"
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/base_nodes/CCNode.h"
#include "Geode/cocos/cocoa/CCGeometry.h"
#include "Geode/cocos/cocoa/CCObject.h"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/cocos/layers_scenes_transitions_nodes/CCTransition.h"
#include "Geode/cocos/menu_nodes/CCMenu.h"
#include "Geode/cocos/misc_nodes/CCClippingNode.h"
#include "Geode/cocos/sprite_nodes/CCSprite.h"
#include "Geode/cocos/textures/CCTexture2D.h"
#include "Geode/loader/Log.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/LazySprite.hpp"
#include "Geode/utils/cocos.hpp"
#include "Geode/utils/web.hpp"
#include "fmod.hpp"
#include "fmod_common.h"
#include <Geode/Geode.hpp>
#include <Geode/binding/AppDelegate.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/MusicDownloadDelegate.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/SongInfoLayer.hpp>
#include <Geode/binding/SongInfoObject.hpp>
#include <Geode/cocos/draw_nodes/CCDrawingPrimitives.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <Geode/modify/MenuLayer.hpp>
#include <string>
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

std::string get_link_from_music_id(int music_id, std::string format)
{
	// @geode-ignore(unknown-resource)
	return "https://aicon.ngfiles.com/" + std::to_string(music_id / 1000) + "/" + std::to_string(music_id) + "." + format;
}

// Taken from https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
std::string url_encode(const std::string &value) {
    std::stringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}


class $modify(MyAudioEngine, FMODAudioEngine)
{
	struct Fields
	{
		std::vector<int> songs;
		int current_music = 0;
	};

	bool create_playlist() {
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
			this->play_next_music_if_finished();
		}
	}

	void play_next_music_if_finished()
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

	void play_previous_music()
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

	void skip_music()
	{
		this->stopAllMusic(true);
		this->play_next_music_if_finished();
	}
};

$execute {
	auto engine = FMODAudioEngine::sharedEngine();
	static_cast<MyAudioEngine *>(engine)->create_playlist();
}

class MusicPlayer : public CCMenu {
	protected:
		int old_music_id = -1;
		bool is_mouse_down = false;

		bool init()
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

		bool mouse_touches_progress_bar()
		{
			auto progress_bar_sprite = this->getChildByID("progress-bar");
			auto mouse_pos = getMousePos();
			auto relative_mouse_pos = mouse_pos - progress_bar_sprite->convertToWorldSpace(CCPointZero);
			auto actual_size = progress_bar_sprite->getContentSize() * 0.1f;

			return relative_mouse_pos.x > 0.0f && relative_mouse_pos.x < actual_size.width && relative_mouse_pos.y > 0 && relative_mouse_pos.y < actual_size.height;
		}

		void search_song(int songID, bool isCustom) {
			auto search_obj = GJSearchObject::create(SearchType::Search);
			
			search_obj->m_songID = songID;
			search_obj->m_songFilter = true;
			search_obj->m_customSongFilter = isCustom;
			
			auto scene = LevelBrowserLayer::scene(search_obj);
			
			CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, scene));
		}

		void refresh_cover_image(int music_id)
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

	public:
		bool ccTouchBegan(CCTouch* touch, CCEvent* event)
		{
			if (this->mouse_touches_progress_bar())
			{
				this->is_mouse_down = true;
				return true;
			}
			return CCMenu::ccTouchBegan(touch, event);
		}

		void ccTouchEnded(CCTouch* touch, CCEvent* event) {
    		this->is_mouse_down = false;
			CCMenu::ccTouchEnded(touch, event);
		}

		void ccTouchCancelled(CCTouch* touch, CCEvent* event) {
			this->is_mouse_down = false;
			CCMenu::ccTouchCancelled(touch, event);
		}

		void update_menu(float dt)
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

		static MusicPlayer* create() {
			auto ret = new MusicPlayer();
			if (ret->init()) {
				ret->autorelease();
				return ret;
			}

			delete ret;
			return nullptr;
    	}

		void next_music(CCObject* sender)
		{
			auto engine = FMODAudioEngine::sharedEngine();
			static_cast<MyAudioEngine *>(engine)->skip_music();
		}

		void previous_music(CCObject* sender)
		{
			auto engine = FMODAudioEngine::sharedEngine();
			static_cast<MyAudioEngine *>(engine)->play_previous_music();
		}

		void search_current_song(CCObject* sender)
		{
			auto engine = FMODAudioEngine::sharedEngine();
			auto custom_engine = static_cast<MyAudioEngine *>(engine);

			this->search_song(custom_engine->m_fields->songs[custom_engine->m_fields->current_music], true);
		}

		void open_current_song_url(CCObject* sender)
		{
			auto engine = FMODAudioEngine::sharedEngine();
			auto custom_engine = static_cast<MyAudioEngine *>(engine);
			int music_id = custom_engine->m_fields->songs[custom_engine->m_fields->current_music];
			auto music_manager = MusicDownloadManager::sharedState();
			auto music = static_cast<SongInfoObject *>(music_manager->m_songObjects->objectForKey(std::to_string(music_id)));
			auto ngArtistUrl = "https://" + music->m_artistName + ".newgrounds.com";
			if (music_id > 10000000) {
				if (!music->m_youtubeChannel.empty())
					web::openLinkInBrowser("https://www.youtube.com/channel/" + music->m_youtubeChannel);
				else
				{

				geode::createQuickPopup(
					"Music not found",
					"Sorry, I can't find anything for this music. Would you like to search it on Youtube?",
					"No", "Yes",
					[music](auto, bool btn2) {
						if (btn2) {
							web::openLinkInBrowser("https://www.youtube.com/results?search_query=" + url_encode(music->m_songName + " - " + music->m_artistName));
						}
					}
				);
				};
			} else {
				web::openLinkInBrowser("https://www.newgrounds.com/audio/listen/" + std::to_string(music_id));
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