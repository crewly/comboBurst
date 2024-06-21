
#include <Geode/Geode.hpp>
#include <random>
#include <string>

using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>

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

class $modify(PlayLayer) {

	struct Fields {
		// Number of characters loaded
		int m_loadedCharacters = 0;

		// Last percent to prevent multiple bursts at the same percent
		int m_lastPercent = 0; 

		// ID of the previously bursted character
		int m_prevChar = 0; 

		// Whether or not platformer is enabled
		bool m_isPlatformer = true; 

		// Container for the characters
		CCNode* m_container = nullptr; 

		// Default audio for combo bursts
		std::string m_defaultAudio = "";

		// Directory of the Sprites
		std::filesystem::path m_spriteDir; 
		
		// List of audio files
		std::vector<std::string> m_spriteAudio; 
	};

	// Check if player is trying to use custom sprites (Sprite pack ID 0)
	bool usingCustomSprites() {
		return (Mod::get()->getSettingValue<int64_t>("sprite-pack") == 0);
	}

	// Check if any action is running
	bool anyActionRunning() {
		for (int i = 0; i <= m_fields->m_loadedCharacters; i++) {
			// Check if character exists and an action is running
			auto child = m_fields->m_container->getChildByID(
				fmt::format("char-{}"_spr, i).c_str()
			);
			if (child) {
				if (child->numberOfRunningActions() > 0) {
					return true;
				}
			}
		}
		return false;
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
				characterID = (rand() % m_fields->m_loadedCharacters)+1;
			} while (
				m_fields->m_prevChar == characterID &&
				m_fields->m_loadedCharacters != 1
			);
		} 
		// Sequentially show characters
		else {
			characterID = (m_fields->m_prevChar%m_fields->m_loadedCharacters)+1;
		}
		m_fields->m_prevChar = characterID;
		return characterID;
	}

	void charBurst() {
		// Return if no characteers are loaded
		if (m_fields->m_loadedCharacters == 0) return;

		// Randomly select a character to burst,
		// that has not been previously played before.
		int characterID = selectCharacterID();

		// If no animation is running
		if (!anyActionRunning()) {
			auto character = m_fields->m_container->getChildByID(
				fmt::format("char-{}"_spr, characterID)
			);

			// Audio Engine
			std::string sfx = m_fields->m_spriteAudio[characterID-1];
			auto fae = FMODAudioEngine::sharedEngine();

			if (sfx.empty()) {
				sfx = m_fields->m_defaultAudio;
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
			auto spawn = CCSpawn::createWithTwoActions(moveInEase, fadeInEase);
			auto despawn = CCSpawn::createWithTwoActions(moveOutEase, fadeOut);
			auto actions = CCSequence::create(spawn, despawn, nullptr);

			// Run the actions
			character->runAction(actions);
		}
	}

	// Load characters
	void loadSprites() {

		// Load characters until a character is not found
		int i = 1;

		// Set default audio for combo bursts
		if (usingCustomSprites()) { // Use custom default SFX if provided
			m_fields->m_defaultAudio = getSoundFile("comboburst-0");
		}							// Otherwise use the default SFX
		if (m_fields->m_defaultAudio.empty() && 
			Mod::get()->getSettingValue<bool>("popup-defaultsfx")) {
			m_fields->m_defaultAudio = defaultAudio;
		}

		// Create a container for the characters
		m_fields->m_container = CCNode::create();
		m_fields->m_container->setID("container"_spr);
		this->addChild(m_fields->m_container, 100);

		// Load characters until a character is not found
		int spritePack = Mod::get()->getSettingValue<int64_t>("sprite-pack");
		while (true) {
			std::string charName = fmt::format("char-{}", i);
			std::string fileName;
			CCSprite* character;

			// Load custom sprites if enabled
			if (usingCustomSprites()) {
				fileName = fmt::format("comboburst-{}.png", i);
				character = CCSprite::create(
					(getSpriteDir() / fileName).string().c_str()
				);
			} 
			// Load sprites from the sprite pack
			else {
				fileName = fmt::format("comboburst-{}_{}.png", spritePack, i);
				character = CCSprite::create(
					fmt::format("{}"_spr, fileName).c_str()
				);
			}

			if (!character) {
				i -= 1;
				break;
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

			m_fields->m_container->addChild(character, 100);
			character->setPosition({ 0, winSize.height / 2 });

			// Set character opacity to 0
			character->setOpacity(0);

			// Push audio files to the audio list
			// Load custom audio if provided
			if (usingCustomSprites()) {
				m_fields->m_spriteAudio.push_back(
					getSoundFile((fmt::format("comboburst-{}", i).c_str()))
				);
			}
			// Otherwise use the default audio
			else {
				m_fields->m_spriteAudio.push_back(
					fmt::format("comboburst-{}_{}.ogg"_spr, spritePack, i)
				);
			}
			i++;
		}
		m_fields->m_loadedCharacters = i;
		log::info("Loaded {} characters", i);
	}

	// Get player's current percent
	int getPercent() {
		#ifndef GEODE_IS_MACOS
			return PlayLayer::getCurrentPercentInt();
		#else
			return static_cast<int>(PlayLayer::getCurrentPercent());
		#endif
	}

	// Classical GD
	void updateProgressbar() {
		PlayLayer::updateProgressbar();

		// Check if a character is loaded
		if (m_fields->m_loadedCharacters <= 0) {
			return;
		}

		// Check if player is in platformer
		if (m_fields->m_isPlatformer) {
			return;
		}

		// Check if practice mode is enabled and the setting is disabled
		else if (PlayLayer::get()->m_isPracticeMode) {
			if (!(Mod::get()->getSettingValue<bool>("popup-practice"))) {
				return;
			}
		}

		// Check if percent is equal to the setting value
		auto percent = getPercent();
		auto delta = percent - m_fields->m_lastPercent;
		if (delta == Mod::get()->getSettingValue<int64_t>("popup-percent") &&
			percent <= 100) {
			m_fields->m_lastPercent = percent;
			charBurst();
		}
	}

	// Get player's percentage when they reset.
	void resetLevel() {
		PlayLayer::resetLevel();
		m_fields->m_lastPercent = getPercent();
	}

	// Setup
	bool init(GJGameLevel* level, bool useReplay, bool setupObjects) {
		if (!PlayLayer::init(level, useReplay, setupObjects)) return false;

		// If mod is enabled
		if (Mod::get()->getSettingValue<bool>("popup-enable")) {
			// Check if platformer is disabled
			m_fields->m_isPlatformer = level->isPlatformer();

			if (usingCustomSprites()) {
				m_fields->m_spriteDir = getSpriteDir();
			}
			loadSprites();
			rng.seed(time(NULL));
		}
		return true;
	}

	#ifndef GEODE_IS_MACOS
	// Platformer GD
	// Burst for every activated checkpoint
	void checkpointActivated(CheckpointGameObject* p0) {
		PlayLayer::checkpointActivated(p0);
		// Check if platformer is enabled and settings are enabled
		if (m_fields->m_isPlatformer &&
			Mod::get()->getSettingValue<bool>("popup-platformer")) {
			charBurst();
		}
	}
	#endif
};
