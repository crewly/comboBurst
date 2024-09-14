#include <Geode/Geode.hpp>
#include "ComboBurst.h"

using namespace geode::prelude;

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

class $modify(MyPauseLayer, PauseLayer) {

	// Pause SFX
	void pause() {
		if (!channel) return;
		channel->setPaused(true);
	}

	// Resume SFX
	void resume() {
		if (!channel) return;
		channel->setPaused(false);
	}

	// Stop SFX
	void stop() {
		if (!channel) return;
		channel->stop();
	}
	void onResume(CCObject* sender) {
		PauseLayer::onResume(sender);
		resume();
	}	
	void onQuit(CCObject* sender) {
		PauseLayer::onQuit(sender);
		stop();
	}

    void customSetup() {
		PauseLayer::customSetup();
		pause();

		// Add settings button
		if (!Mod::get()->getSettingValue<bool>("comboburst-settingsbtn")) return;
		auto sprite = CircleButtonSprite::create(
			CCSprite::create("settingsBtn.png"_spr),
			CircleBaseColor::Green,
			CircleBaseSize::Small
		);
		auto btn = CCMenuItemSpriteExtra::create(	
			sprite,
			this,
			menu_selector(MyPauseLayer::onClick)
		);
        auto menu = this->getChildByID("right-button-menu");
        menu->addChild(btn);
		menu->updateLayout();
	}

	// Settings button callback
	void onClick(CCObject* sender) {
		openSettingsPopup(Mod::get());
	}
};
