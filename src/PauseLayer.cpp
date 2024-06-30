#include <Geode/Geode.hpp>
#include "ComboBurst.h"

using namespace geode::prelude;

#include <Geode/modify/PauseLayer.hpp>


class $modify(MyPauseLayer, PauseLayer) {

	// Pause SFX
	void pause() {
		if (!channel) return;
		log::info("Paused");
		channel->setPaused(true);
	}

	// Resume SFX
	void resume() {
		if (!channel) return;
		log::info("Resumed");
		channel->setPaused(false);
	}

	// Stop SFX
	void stop() {
		if (!channel) return;
		log::info("Stopped");
		channel->stop();
	}

	// Event listeners
    void customSetup() {
		PauseLayer::customSetup();
		pause();
	}
	void onResume(CCObject* sender) {
		PauseLayer::onResume(sender);
		resume();
	}	
	void onQuit(CCObject* sender) {
		PauseLayer::onQuit(sender);
		stop();
	}
};