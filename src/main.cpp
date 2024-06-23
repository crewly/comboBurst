#include <Geode/Geode.hpp>
#include <random>
#include <string>

using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

// Find save directory
std::filesystem::path getSpriteDir() {
	auto ghcPath = (Mod::get()->getSaveDir() / "custom-sprite");
	std::filesystem::path path = ghcPath.u8string();
	return path;
}

// Create Custom Sprite directory if it doesn't exist
$on_mod(Loaded) {
	auto path = getSpriteDir();
	if (!std::filesystem::exists(path)) {
		try {
			std::filesystem::create_directory(path);
		} catch (std::filesystem::filesystem_error& e) {
			log::error("Couldn't create custom sprite directory:"
						"{}", e.what());
		}
	}
}

std::string defaultAudio = "comboburst_default.ogg"_spr;
std::mt19937 rng;

// ComboBurst class
class ComboBurst : public CCNode {
	public:
		// Number of characters loaded
		int m_loadedCharacters = 0;

		// Last percent to prevent multiple bursts at the same percent
		int m_lastPercent = 0; 

		// ID of the previously bursted character
		int m_prevChar = 0; 

		// If action is playing
		bool m_actionRunning = false;

		// Default audio for combo bursts
		std::string m_defaultAudio = "";

		// Directory of the Sprites
		std::filesystem::path m_spriteDir; 
		
		// List of audio files
		std::vector<std::string> m_spriteAudio;

	static ComboBurst* create(PlayLayer* layer) {
		auto* ret = new (std::nothrow) ComboBurst;
		if (ret && ret->init(layer)) {
			ret->autorelease();
			return ret;
		} else {
			delete ret;
			return nullptr;
		}
	}

	bool init(PlayLayer* layer) {
		if (!CCNode::init()) return false;

		this->setID("characters"_spr);
		layer->addChild(this, 100);

		// Load sprites
		loadSprites();

		return true;
	}

	// Get player's percentage when they reset.
	int getPercent(PlayLayer* layer) {
		return layer->getCurrentPercentInt();
	}

	// Check if player is trying to use custom sprites (Sprite pack ID 0)
	bool usingCustomSprites() {
		return (Mod::get()->getSettingValue<int64_t>("sprite-pack") == 0);
	}

	// Get sound file
	// (TODO: Add cyrillic support)
	std::string getSoundFile(std::string name) {
		std::vector<std::string> extensions = { ".ogg", ".wav", ".mp3",
												".m4a", ".flic" };
		for (auto& ext : extensions) {
			auto file = fmt::format("{}{}", name, ext);
			if (std::filesystem::exists((getSpriteDir() / file))) {
				auto path = (getSpriteDir() / file);
				return path.string();
			}
		}
		// No file found
		return "";
	}

	int selectCharacterID() {
		int characterID;
		// Use random algorithm
		if (Mod::get()->getSettingValue<bool>("popup-random")) {
			do {
				characterID = (rand() % m_loadedCharacters)+1;
			} while (
				m_prevChar == characterID &&
				m_loadedCharacters != 1
			);
		} 
		// Sequentially show characters
		else {
			characterID = (m_prevChar%m_loadedCharacters)+1;
		}
		m_prevChar = characterID;
		return characterID;
	}

	// Reset actionRunning flag
	void actionEnd() {
		m_actionRunning = false;
	}

	void charBurst() {
		// Return if no characteers are loaded
		if (m_loadedCharacters == 0) return;

		// Randomly select a character to burst,
		// that has not been previously played before.
		int characterID = selectCharacterID();

		// If no animation is running
		if (!m_actionRunning) {
			// Set actionRunning to true
			m_actionRunning = true;

			// Get character by ID
			auto character = this->getChildByID(
				fmt::format("char-{}"_spr, characterID)
			);

			// Audio Engine
			std::string sfx = m_spriteAudio[characterID-1];
			auto fae = FMODAudioEngine::sharedEngine();

			// Use default audio if no custom audio is provided
			if (sfx.empty()) {
				sfx = m_defaultAudio;
			}

			if (Mod::get()->getSettingValue<bool>("popup-sfxslider")) {
				fae->playEffect(sfx);
			}
			// Use volume settings from mod options
			else {
				auto system = fae->m_system;
				FMOD::Channel* channel;
				FMOD::Sound* sound;
				
                auto sfxPath = Mod::get()->getResourcesDir().parent_path() / sfx;

				// Create sound object
				// Use custom sprites if enabled
				if (usingCustomSprites() && sfx != defaultAudio) {
					system->createSound(sfx.c_str(), 
										FMOD_DEFAULT, 
										nullptr,
										&sound);

				// Otherwise use the default SFX
				} else {
					system->createSound(sfxPath.string().c_str(),
										FMOD_DEFAULT,
										nullptr,
										&sound);
				}

				// Play sound
				system->playSound(sound, nullptr, false, &channel);
				channel->setVolume(
					Mod::get()->getSettingValue<int64_t>("popup-volume")/100.f
				);
			}

			// Set starting position to the left side of the screen
			character->setPositionX(0.0f);
			auto characterY = character->getPositionY();

			// Get player's opacity setting
			auto opacity = Mod::get()->getSettingValue<int64_t>("popup-opacity");

			// Character animations
			// Move character to the right side of the screen while fading in
			auto moveIn = CCMoveTo::create(1, { 75, characterY });
			auto moveInEase = CCEaseBackOut::create(moveIn);
			auto fadeInEase = CCEaseExponentialOut::create(
				CCFadeTo::create(1, opacity)
			);

			// Move character back to the left side of the screen while fading out
			auto moveOut = CCMoveTo::create(1, { 65, characterY });
			auto moveOutEase = CCEaseBackIn::create(moveOut);
			auto fadeOut = CCFadeTo::create(0.5, 0);

			// Spawn the move and fade actions
			auto spawn = CCSpawn::create(moveInEase, fadeInEase, nullptr);
			auto despawn = CCSpawn::create(moveOutEase, fadeOut, nullptr);

			// Add a callback to reset the actionRunning flag
			auto reset = CCCallFunc::create(
				this, callfunc_selector(ComboBurst::actionEnd)
			);

			auto actions = CCSequence::create(spawn, despawn, reset, nullptr);

			// Run the actions
			character->runAction(actions);
		}
	}

	// Load characters
	void loadSprites() {

		// Load characters until a character is not found
		int i = 0;

		// Set default audio for combo bursts
		if (usingCustomSprites()) { // Use custom default SFX if provided
			m_defaultAudio = getSoundFile("comboburst-0");
		}							// Otherwise use the default SFX
		if (m_defaultAudio.empty() && 
			Mod::get()->getSettingValue<bool>("popup-defaultsfx")) {
			m_defaultAudio = defaultAudio;
		}

		// Load characters until a character is not found
		int spritePack = Mod::get()->getSettingValue<int64_t>("sprite-pack");
		
		// Load characters until a character is not found
		while (true) {
			std::string charName = fmt::format("char-{}", i+1);
			std::string fileName;
			CCSprite* character;
			// Load custom sprites if enabled
			if (usingCustomSprites()) {
				fileName = fmt::format("comboburst-{}.png", i+1);

				// Break if file is not found
				if (!std::filesystem::exists(
					(getSpriteDir() / fileName)
				)) {
					break;
				}

				character = CCSprite::create(
					(getSpriteDir() / fileName).string().c_str()
				);
			} 
			
			// Load sprites from the sprite pack
			else {
				fileName = fmt::format("comboburst-{}_{}.png", spritePack, i+1);

				// Break if file is not found
				if (!std::filesystem::exists(
					Mod::get()->getResourcesDir() / fileName
				)) {
					break;
				}

				character = CCSprite::create(
					fmt::format("{}"_spr, fileName).c_str()
				);
			}

			character->setID(
				fmt::format("{}"_spr, charName)
			);

			// Scale character to fit screen
			auto popupSize = Mod::get()->getSettingValue<double>("popup-size");
			auto winSize = CCDirector::get()->getWinSize();
			auto characterSize = character->getContentSize();
		
			float scale = (winSize.height / characterSize.height) * popupSize;
			character->setScaleX(scale);
			character->setScaleY(scale);

			this->addChild(character, 100);
			character->setPosition({ 0, winSize.height / 2 });

			// Set character opacity to 0
			character->setOpacity(0);

			// Push audio files to the audio list
			// Load custom audio if provided
			if (usingCustomSprites()) {
				m_spriteAudio.push_back(
					getSoundFile((fmt::format("comboburst-{}", i+1).c_str()))
				);
			}
			// Otherwise use the default audio
			else {
				m_spriteAudio.push_back(
					fmt::format("comboburst-{}_{}.ogg"_spr, spritePack, i+1)
				);
			}
			i++;
		}
		m_loadedCharacters = i;

		// Log the number of characters loaded
		if (m_loadedCharacters == 0) {
			log::warn("No characters found");
		} else {
			log::info("Loaded {} characters", m_loadedCharacters);
		}
	}

	// 
	void update(PlayLayer* layer) {

		// Check if a character is loaded
		if (m_loadedCharacters <= 0) {
			return;
		}

		// Check if player is in platformer
		if (m_isPlatformer) {
			return;
		}

		// Check if practice mode is enabled and the setting is disabled
		else if (layer->m_isPracticeMode) {
			if (!(Mod::get()->getSettingValue<bool>("popup-practice"))) {
				return;
			}
		}

		auto percent = getPercent(layer);
		// Check if percent is equal to the setting value
		auto delta = percent - m_lastPercent;
		if (delta == Mod::get()->getSettingValue<int64_t>("popup-percent") &&
			percent <= 100) {
			m_lastPercent = percent;
			charBurst();
		}
	}

};

// Modify PlayLayer
class $modify(PlayLayer) {
	struct Fields {
		// ComboBurst object
		ComboBurst* m_comboBurst = nullptr;

		// Check if platformer is enabled
		int m_isPlatformer = 0;
	};

	bool init(GJGameLevel* level, bool useReplay, bool setupObjects) {
		if (!PlayLayer::init(level, useReplay, setupObjects)) {
			return false;
		}
		m_fields->m_isPlatformer = level->isPlatformer();
		// Create ComboBurst object
		m_fields->m_comboBurst = ComboBurst::create(this);
		return true;
	}

	void updateProgressbar() {
		PlayLayer::updateProgressbar();
		if (!m_fields->m_comboBurst) {
			return;
		}
		m_fields->m_comboBurst->update(this);
	}
	void resetLevel() {
		PlayLayer::resetLevel();
		if (!m_fields->m_comboBurst) {
			return;
		}
		m_fields->m_comboBurst->m_lastPercent = this->getCurrentPercentInt();
	}

	// No MacOS Support :(
	#ifndef GEODE_IS_MACOS
	// Checkpoint activated (for platformer)
	void checkpointActivated(CheckpointGameObject* p0) {
		PlayLayer::checkpointActivated(p0);
		// Check if platformer is enabled and settings are enabled
		if (m_fields->m_isPlatformer &&
			Mod::get()->getSettingValue<bool>("popup-platformer")) {
			m_fields->m_comboBurst->charBurst();
		}
	}
	#endif

};