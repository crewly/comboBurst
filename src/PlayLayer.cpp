#include <Geode/Geode.hpp>
#include "ComboBurst.h"

using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

// Modify PlayLayer
class $modify(MyPlayLayer, PlayLayer) {

	// Fields
	struct Fields {
		// ComboBurst instance
		ComboBurst* m_comboBurst = nullptr;

		// Check if platformer is enabled
		bool m_isPlatformer = false;
	};

	// Initialize combo burst
	bool init(GJGameLevel* level, bool useReplay, bool setupObjects) {
		if (!PlayLayer::init(level, useReplay, setupObjects)) {
			return false;
		}

		// Set platformer status
		m_fields->m_isPlatformer = level->isPlatformer();

		// Create ComboBurst instance
		if (Mod::get()->getSettingValue<bool>("popup-enable")) {
			m_fields->m_comboBurst = ComboBurst::create(this);
		}
		return true;
	}

	// Classic GD
	// Update progress bar
	void updateProgressbar() {
		PlayLayer::updateProgressbar();

		if (!m_fields->m_comboBurst || m_fields->m_isPlatformer) {
			return;
		}
		m_fields->m_comboBurst->update(this);
	}

	// Reset level
	void resetLevel() {
		PlayLayer::resetLevel();
		if (!m_fields->m_comboBurst) {
			return;
		}
		m_fields->m_comboBurst->m_lastPercent = this->getCurrentPercentInt();
	}

	// Platformer GD
	// Checkpoint activated
	void checkpointActivated(CheckpointGameObject* p0) {
		PlayLayer::checkpointActivated(p0);
		if (!m_fields->m_comboBurst) {
			return;
		}

		// Check if platformer is enabled and settings are enabled
		if (m_fields->m_isPlatformer &&
			Mod::get()->getSettingValue<bool>("popup-platformer")) {
			m_fields->m_comboBurst->charBurst();
		}
	}

};

// Reload combo burst
void reloadComboBurst() {
	if (auto pl = PlayLayer::get()) {
		auto& myFields = static_cast<MyPlayLayer*>(pl)->m_fields;
		if (myFields->m_comboBurst) {
			myFields->m_comboBurst->loadSprites();
		}
	}
}

// Settings listener
// there's gotta be a better way to do this bruh :sob:
$execute {
	listenForSettingChanges("sprite-pack", [](std::string value) {
		reloadComboBurst();
	});
	listenForSettingChanges("popup-enable", [](bool value) {
		if (auto pl = PlayLayer::get()) {
			auto& myFields = static_cast<MyPlayLayer*>(pl)->m_fields;
			if (value) { // enable
				if (!myFields->m_comboBurst) {
					myFields->m_comboBurst = ComboBurst::create(pl);
				} else {
					myFields->m_comboBurst->loadSprites();
				}
				myFields->m_comboBurst->m_lastPercent = pl->getCurrentPercentInt();
			} else { // disable
				myFields->m_comboBurst->unloadSprites();
			}
		}
	});
	listenForSettingChanges("popup-size", [](double value) {
		reloadComboBurst();
	});
	listenForSettingChanges("popup-opacity", [](int64_t value) {
		reloadComboBurst();
	});
	listenForSettingChanges("popup-effect", [](std::string value) {
		if (auto pl = PlayLayer::get()) {
			auto& myFields = static_cast<MyPlayLayer*>(pl)->m_fields;
			if (myFields->m_comboBurst) {
				myFields->m_comboBurst->setEffect(value);
			}
		}
	});

}