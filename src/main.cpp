
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
			log::error("Failed to create custom sprite directory: {}", e.what());
		}
	}
}

std::string defaultAudio = "cb_default.ogg"_spr;
std::mt19937 rng;

class $modify(MyPlayLayer, PlayLayer) {

	struct Fields {
		int m_loadedCharacters = 0; // Number of characters loaded
		int m_lastPercent = 0; // Last percent to prevent multiple bursts at the same percent
		int m_prevChar = 0; // ID of the previously bursted character
		bool m_isPlatformer = true; // Whether or not platformer is enabled
		std::string m_defaultAudio = "";
		std::filesystem::path m_spriteDir; // Directory of the Sprites
		std::vector<std::string> m_spriteAudio; // List of audio files
	};

	// Check if player is trying to use custom sprites (Sprite pack ID 0)
	bool usingCustomSprites() {
		return (Mod::get()->getSettingValue<int64_t>("sprite-pack") == 0);
	}

	// Check if any action is running
	bool anyActionRunning() {
		for (int i = 0; i <= m_fields->m_loadedCharacters; i++) {
			// Check if character exists and an action is running
			if (this->getChildByID(fmt::format("cb_char{}", i))) {
				if (this->getChildByID(fmt::format("cb_char{}", i))->numberOfRunningActions() > 0) {
					return true;
				}
			}
		}
		return false;
	}

	// Get sound file (TODO: Add cyrillic support (which is impossible because of playEffect() not supporting cyrillic characters))
	std::string getSoundFile(std::string name) {
		std::vector<std::string> extensions = { ".ogg", ".wav", ".mp3", ".m4a", ".flic" };
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
			} while (m_fields->m_prevChar == characterID && m_fields->m_loadedCharacters != 1);
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

		// Randomly select a character to burst, that has not been previously played before.
		int characterID = selectCharacterID();

		// If no animation is running
		if (!anyActionRunning()) {
			auto character = this->getChildByID(Mod::get()->expandSpriteName(fmt::format("cb_char{}", characterID).c_str()));

			// Audio Engine
			std::string sfx = m_fields->m_spriteAudio[characterID-1];
			if (sfx.empty()) sfx = m_fields->m_defaultAudio;

			auto fae = FMODAudioEngine::sharedEngine();
			if (Mod::get()->getSettingValue<bool>("popup-sfxslider")) fae->playEffect(sfx);

			// Use volume settings from mod options
			else {
				auto system = fae->m_system;
				FMOD::Channel* channel;
				FMOD::Sound* sound;
				
                auto sfxPath = Mod::get()->getResourcesDir().parent_path() / sfx;
				if (usingCustomSprites() && sfx != defaultAudio)
					system->createSound(sfx.c_str(), FMOD_DEFAULT, nullptr, &sound);
				else
					system->createSound(sfxPath.string().c_str(), FMOD_DEFAULT, nullptr, &sound);
				system->playSound(sound, nullptr, false, &channel);
				channel->setVolume((Mod::get()->getSettingValue<int64_t>("popup-volume")/100.f));
			}

			// Set starting position to the left side of the screen
			CCSize winSize = CCDirector::get()->getWinSize();
			character->setPosition({ 0, winSize.height / 2 });

			// Move character to the right side of the screen while fading in
			auto moveIn = CCMoveTo::create(1, { 75, character->getPositionY() });
			auto moveInEase = CCEaseBackOut::create(moveIn);
			auto fadeInEase = CCEaseExponentialOut::create(CCFadeIn::create(1));

			// Move character back to the left side of the screen while fading out
			auto moveOut = CCMoveTo::create(1, { 65, character->getPositionY() });
			auto moveOutEase = CCEaseBackIn::create(moveOut);
			auto fadeOut = CCFadeOut::create(0.5);

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
		auto adjustedScale = Mod::get()->getSettingValue<double>("popup-size");
		CCSize winSize = CCDirector::get()->getWinSize();

		// Load characters until a character is not found
		int i = 1;

		// Set default audio for combo bursts
		if (usingCustomSprites()) { // Use custom default sound effect if provided
			m_fields->m_defaultAudio = getSoundFile("comboburst-0");
		}							// Otherwise use the mod's default sound effect
		if (m_fields->m_defaultAudio.empty() && Mod::get()->getSettingValue<bool>("popup-defaultsfx")) {
			m_fields->m_defaultAudio = defaultAudio;
		}

		while (true) {
			auto charname = fmt::format("cb_char{}", i);
			auto filename = fmt::format("cb_char{}_{}.png", Mod::get()->getSettingValue<int64_t>("sprite-pack"), i);
			auto character = CCSprite::create(Mod::get()->expandSpriteName(filename.c_str()));

			if (usingCustomSprites()) {
				filename = fmt::format("comboburst-{}.png", i);
				character = CCSprite::create((getSpriteDir() / filename).string().c_str());
				// There won't be two CCSprite instances created because the initial variable should lead to a nullptr (i think)
			}

			if (!character) {
				i -= 1;
				break;
			}

			character->setID(Mod::get()->expandSpriteName(charname.c_str()));

			// Scale character to fit screen
			float scaleRatio = (winSize.height / character->getContentSize().height) * adjustedScale;
			character->setScaleX(scaleRatio);
			character->setScaleY(scaleRatio);

			this->addChild(character, 100);

			// Set character opacity to 0
			character->setOpacity(0);

			// Push audio files to the audio list
			if (usingCustomSprites()) {
				m_fields->m_spriteAudio.push_back(getSoundFile((fmt::format("comboburst-{}", i).c_str())));
			}
			else {
				m_fields->m_spriteAudio.push_back(Mod::get()->expandSpriteName(fmt::format("cb_char{}_{}.ogg", Mod::get()->getSettingValue<int64_t>("sprite-pack"), i).c_str()));
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
		if (delta == Mod::get()->getSettingValue<int64_t>("popup-percent") && percent <= 100) {
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
			//log::info("Platformer: {}", m_fields->m_isPlatformer);
		}
		return true;
	}

	#ifndef GEODE_IS_MACOS
	// Platformer GD
	// Burst for every activated checkpoint
	void checkpointActivated(CheckpointGameObject* p0) {
		PlayLayer::checkpointActivated(p0);
		// Check if platformer is enabled and settings are enabled
		if (m_fields->m_isPlatformer) if (Mod::get()->getSettingValue<bool>("popup-platformer")) charBurst();
	}
	#endif
};