#include <Geode/Geode.hpp>
#include "ComboBurst.h"

using namespace geode::prelude;

#include <Geode/modify/PauseLayer.hpp>

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