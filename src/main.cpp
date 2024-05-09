
#include <Geode/Geode.hpp>
#include <random>
#include <string>

using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>

// Create Custom Sprite directory if it doesn't exist
$on_mod(Loaded) {
	auto path = (Mod::get()->getSaveDir() / "custom-sprite");
	auto pathString = path.string().c_str();
	if (!std::filesystem::exists(pathString)) {
		std::filesystem::create_directory(pathString);
	}
}

std::mt19937 rng;

class $modify(PlayLayer) {

	struct Fields {
		int m_loadedCharacters = 0; // Number of characters loaded
		int m_lastPercent = 0; // Last percent to prevent multiple bursts at the same percent
		bool m_isPlatformer = true; // Check if level is a platformer
		std::filesystem::path m_spriteDir; // Directory of the Sprites
		std::vector<std::string> m_spriteAudio; // List of audio files
	};

	// Check if player is trying to use custom sprites (Sprite pack ID 0)
	bool usingCustomSprites() {
		return (Mod::get()->getSettingValue<int64_t>("sprite-pack") == 0);
	}

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

	std::string getSoundFile(std::string name) {
		std::vector<std::string> extensions = { ".ogg", ".wav", ".mp3", ".m4a", ".flic" };
		for (auto& ext : extensions) {
			auto file = fmt::format("{}{}", name, ext);
			if (std::filesystem::exists((Mod::get()->getSaveDir() / "custom-sprite" / file).string().c_str())) {
				auto path = (Mod::get()->getSaveDir() / "custom-sprite" / file).string().c_str();
				return path;
			}
		}
		// No file found
		return "";
	}

	void charBurst() {
		// Randomly select a character to burst
		if (m_fields->m_loadedCharacters == 0) return;
		int characterID = (rand() % m_fields->m_loadedCharacters)+1;
		auto character = this->getChildByID(fmt::format("cb_char{}", characterID));

		// If no action is running
		if (!anyActionRunning()) {
			std::string sfx = m_fields->m_spriteAudio[characterID-1];
				
			FMODAudioEngine::sharedEngine()->playEffect(sfx);

			CCSize winSize = CCDirector::get()->getWinSize();

			// Set starting position to the left side of the screen
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
		while (true) {
			auto charname = fmt::format("cb_char{}", i);
			auto filename = fmt::format("cb_char{}_{}.png", Mod::get()->getSettingValue<int64_t>("sprite-pack"), i);
			auto character = CCSprite::create(Mod::get()->expandSpriteName(filename.c_str()));

			if (usingCustomSprites()) {
				filename = fmt::format("cb_char{}.png", i);
				character = CCSprite::create((Mod::get()->getSaveDir() / "custom-sprite" / filename).string().c_str());
				// There won't be two CCSprite instances created because the initial variable should lead to a nullptr (i think)
			}

			if (!character) {
				i -= 1;
				break;
			}

			character->setID(charname.c_str());

			// Scale character to fit screen
			float scaleRatio = (winSize.height / character->getContentSize().height) * adjustedScale;
			character->setScaleX(scaleRatio);
			character->setScaleY(scaleRatio);

			this->addChild(character, 100);

			// Set character opacity to 0
			character->setOpacity(0);

			// Push audio files to the audio list
			if (usingCustomSprites()) {
				m_fields->m_spriteAudio.push_back(getSoundFile((fmt::format("cb_char{}", i).c_str())));
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
		#ifdef GEODE_IS_IOS
			return static_cast<int>(PlayLayer::getCurrentPercent());
		#else
			return PlayLayer::getCurrentPercentInt();
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
				auto path = (Mod::get()->getSaveDir() / "custom-sprite");
				auto pathString = path.string().c_str();
				m_fields->m_spriteDir = pathString;
			}
			loadSprites();
			rng.seed(time(NULL));
			//log::info("Platformer: {}", m_fields->m_isPlatformer);
		}
		return true;
	}

	// Platformer GD
	// Burst for every activated checkpoint
	void checkpointActivated(CheckpointGameObject* p0) {
		PlayLayer::checkpointActivated(p0);
		// Check if platformer is enabled and settings are enabled
		if (m_fields->m_isPlatformer) if (Mod::get()->getSettingValue<bool>("popup-platformer")) charBurst();
	}
};