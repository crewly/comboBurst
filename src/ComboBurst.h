#pragma once
#include <Geode/Geode.hpp>
#include <random>
#include <string>
#include <Geode/binding/PlayLayer.hpp>

using namespace geode::prelude;

// Prerequisites //

// Find save directory
inline std::filesystem::path getSpriteDir() {
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

inline std::string defaultAudio = "comboburst_default.ogg"_spr;
inline FMOD::Channel* channel;

// ComboBurst class
class ComboBurst : public CCNode {
protected:
	bool init(PlayLayer* layer) {
		if (!CCNode::init()) return false;

		this->setID("characters"_spr);
		layer->addChild(this, 100);

		// Set RNG seed
		rng.seed(std::random_device()());

		// Load sprites
		loadSprites();
		if (m_loadedCharacters == 0) {
			return false;
		}

		return true;
	}

public:

	// Animation Effects
	enum class PopupEffect {
		SlideLeft,
		SlideRight,
		FadeLeft,
		FadeRight,
		SlideAlternate,
		FadeAlternate
	};

	// Pop-up Effect
	PopupEffect m_effect;

	// Map of animation effects
	std::map<std::string, PopupEffect> m_effects = {
		{"Slide Left", PopupEffect::SlideLeft},
		{"Slide Right", PopupEffect::SlideRight},
		{"Slide Alternate", PopupEffect::SlideAlternate},
		{"Fade Left", PopupEffect::FadeLeft},
		{"Fade Right", PopupEffect::FadeRight},
		{"Fade Alternate", PopupEffect::FadeAlternate}
	};

	// RNG-seed
	std::mt19937 rng;

	// Number of characters loaded
	int m_loadedCharacters = 0;

	// Current sprite pack ID
	int m_spritePackID = -1;

	// Last occurrence of a combo burst
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

	// Alternating bool for alternating effects
	bool m_popupAlternate = false;

	// ** Member Functions ** //

	// Create ComboBurst object
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

	// Assign ID to m_effect
	void setEffect(std::string effect) {
		m_effect = m_effects[effect];
	}

	// Get player's percentage
	int getPercent(PlayLayer* layer) {
		return layer->getCurrentPercentInt();
	}

	// Check if player is trying to use custom sprites (ID = 0)
	bool usingCustomSprites() {
		return getSpritePackID() == 0;
	}

	// Get sprite pack ID
	int getSpritePackID() {
		if (m_spritePackID != -1) {
			return m_spritePackID;
		}

		// Find the sprite pack ID
		auto pack = Mod::get()->getSettingValue<std::string>("sprite-pack");
		// The ID of the sprite packs are the same as their index in the array
		std::array packs = {"Custom", "Anime", "Meme"};
		for (int i = 0; i < packs.size(); i++) {
			if (pack == packs[i]) {
				m_spritePackID = i;
				break;
			}
		}
		return m_spritePackID;
	}

	// Get sound file
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

	// Burst character
	void charBurst() {
		// Return if no characteers are loaded
		if (m_loadedCharacters == 0) return;

		// If no animation is running
		if (!m_actionRunning) {
			// Set actionRunning to true
			m_actionRunning = true;

			// Randomly select a character to burst,
			// that has not been previously played before.
			int characterID = selectCharacterID();

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

			// Play sound effect in GD
			if (Mod::get()->getSettingValue<bool>("popup-sfxslider")) {
				fae->playEffect(sfx);
			}
			// Play sound effect using FMOD
			else {
				auto system = fae->m_system;
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
					Mod::get()->getSettingValue<int64_t>("popup-volume") / 100.f
				);
			}

			// ** Character animations ** //

			// Get window size
			auto winSize = CCDirector::get()->getWinSize();

			// Get player's settings
			auto opacity = Mod::get()->
				getSettingValue<int64_t>("popup-opacity");
			auto popupEndPos = Mod::get()->
				getSettingValue<int64_t>("popup-endpos");

			// Calculate the final X-position of the character
			popupEndPos = 
				(winSize.width) * 
				(popupEndPos/100.0f);


			// Create actions variable
			CCSequence* actions;

			// Callback Function to reset the actionRunning flag
			auto reset = CCCallFunc::create(
				this, callfunc_selector(ComboBurst::actionEnd)
			);

			// Function to alternate between two effects
			auto alternateEffect = [this](PopupEffect left, PopupEffect right) {
				m_popupAlternate = !m_popupAlternate;
				return m_popupAlternate? left : right;
			};

			// Helper variable for the effect type
			auto effectType = m_effect;

			switch (m_effect) {
				
				// Slide effect
				case PopupEffect::SlideAlternate:
					effectType = alternateEffect(
						PopupEffect::SlideLeft, 
						PopupEffect::SlideRight
					);
				case PopupEffect::SlideLeft:
				case PopupEffect::SlideRight: {
					float direction = 1;

					// Starting position
					switch (effectType) {
						case PopupEffect::SlideLeft:
							character->setPositionX(0.0f);
							break;
						case PopupEffect::SlideRight:
							character->setPositionX(winSize.width);
							direction = -1;
							break;
						default:
							break;
					}
					
					auto startingY = character->getPositionY(); 
					auto startingX = character->getPositionX();

					// Move character towards the center while fading in
					auto moveIn = CCMoveTo::create(
						1, 
						{ startingX + direction*popupEndPos, startingY }
					);
					auto moveInEase = CCEaseExponentialOut::create(moveIn);
					auto fadeInEase = CCEaseExponentialOut::create(
						CCFadeTo::create(1, opacity)
					);

					auto spawn = CCSpawn::createWithTwoActions(
						moveInEase, 
						fadeInEase
					);
					auto fadeOut = CCFadeTo::create(0.5, 0);

					actions = CCSequence::create(
						spawn, 
						fadeOut, 
						reset, 
						nullptr
					);

					break;
				}

				// Fade effect
				case PopupEffect::FadeAlternate:
					effectType = alternateEffect(
						PopupEffect::FadeLeft, 
						PopupEffect::FadeRight
					);
				case PopupEffect::FadeLeft:
				case PopupEffect::FadeRight: {

					// Starting position
					switch (effectType) {
						case PopupEffect::FadeLeft:
							character->setPositionX(popupEndPos);
							break;
						case PopupEffect::FadeRight:
							character->setPositionX(winSize.width-popupEndPos);
							break;
						default:
							break;
					}

					auto fadeInEase = CCEaseExponentialOut::create(
						CCFadeTo::create(1, opacity)
					);
					auto fadeOut = CCFadeTo::create(0.5, 0);
				
					actions = CCSequence::create(
						fadeInEase, 
						fadeOut, 
						reset, 
						nullptr
					);

					break;
				}
				default:
					break;
			}

			// Run the actions
			character->runAction(actions);
		}
	}

	// Unload characters
	void unloadSprites() {
		m_spritePackID = -1; // Reset selected sprite pack ID
		this->removeAllChildren(); // Remove all children
		m_spriteAudio.clear(); // Clear audio list
		m_actionRunning = false; // Reset actionRunning flag
		channel->stop(); // Stop audio
		m_loadedCharacters = 0; // Reset loaded characters
	}

	// Load characters
	void loadSprites() {
		
		// RESET: if sprites are already loaded, reset characters
		if (m_loadedCharacters > 0) {
			unloadSprites();
		}

		// Assign animation effect
		setEffect(Mod::get()->getSettingValue<std::string>("popup-effect"));

		// Get sprite pack ID
		int spritePack = getSpritePackID();

		// Set default audio for combo bursts
		// Use custom default SFX if provided
		if (usingCustomSprites()) { 
			m_defaultAudio = getSoundFile("comboburst-0");
		}
		// Otherwise use the default SFX
		if (m_defaultAudio.empty() && 
			Mod::get()->getSettingValue<bool>("popup-defaultsfx")) {
			m_defaultAudio = defaultAudio;
		}

		int i = 0;
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
				//log::info("File {} does exist", fileName);
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
			character->setScale(scale);

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
			unloadSprites();
		} else {
			log::info("Loaded {} characters", m_loadedCharacters);
		}
	}

	// Update function; checks percentage delta and calls charBurst()
	// GD Classic mode only
	void update(PlayLayer* layer) {

		// Check if a character is loaded
		if (m_loadedCharacters <= 0) {
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
