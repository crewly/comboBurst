
#include <Geode/Geode.hpp>
#include <random>

using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

class $modify(PlayLayer) {

	bool anyActionRunning() {
		const auto runningScene = CCDirector::get()->getRunningScene();
		for (int i = 0; i < 4; i++) {
			// Check if character exists and an action is running
			if (runningScene->getChildByID(fmt::format("character{}", i))) {
				if (runningScene->getChildByID(fmt::format("character{}", i))->numberOfRunningActions() > 0) {
					return true;
				}
			}
		}
		return false;
	}

	void characterBurst() {
		const auto runningScene = CCDirector::get()->getRunningScene();

		// Randomly select a character to burst
		int characterID = (rand() % 3)+1;
		auto character = runningScene->getChildByID(fmt::format("character{}", characterID));

		// If character exists and no action is running
		if (character && !this->anyActionRunning()) {

			FMODAudioEngine::sharedEngine()->playEffect(fmt::format("crewly.comboburst/cb_character{}_{}.wav", Mod::get()->getSettingValue<int64_t>("sprite-pack"), characterID));

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


	void updateProgressbar() {
		PlayLayer::updateProgressbar();

		// Check if mod is enabled
		if (!(Mod::get()->getSettingValue<bool>("popup-enable"))) {
			return;
		}

#ifdef GEODE_IS_IOS
		int percent = static_cast<int>(PlayLayer::getCurrentPercent());
#else
		int percent = PlayLayer::getCurrentPercentInt();
#endif

		const auto runningScene = CCDirector::get()->getRunningScene();

		// Check if characters are loaded
		if (runningScene->getChildByID("character1")) {
			auto popupPercent = Mod::get()->getSettingValue<int64_t>("popup-percent");
			if (percent > 0 && percent < 100 && percent % popupPercent == 0) {
				characterBurst();
			}
		}
		// Create characters if they don't exist
		else {
			auto adjustedScale = Mod::get()->getSettingValue<double>("popup-size");
			CCSize winSize = CCDirector::get()->getWinSize();
			int i = 1;
			while (true) {
				auto charname = fmt::format("character{}", i);
				auto filename = fmt::format("crewly.comboburst/cb_character{}_{}.png", Mod::get()->getSettingValue<int64_t>("sprite-pack"), i);
				auto character = CCSprite::create(filename.c_str());
				if (!character) {
					break;
				}
				character->setID(charname.c_str());

				// Scale character to fit screen
				float scaleRatio = (winSize.height / character->getContentSize().height) * adjustedScale;
				character->setScaleX(scaleRatio);
				character->setScaleY(scaleRatio);

				runningScene->addChild(character, 100);

				// Set character opacity to 0
				character->setOpacity(0);
				i++;
			}
		}
	}

};
