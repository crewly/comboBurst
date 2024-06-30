#include <Geode/Geode.hpp>
#include "ComboBurst.h"

using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

// Modify PlayLayer
class $modify(PlayLayer) {

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

	// TODO: Add MacOS support
	#ifndef GEODE_IS_MACOS

	// Checkpoint activated (for platformer)
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
	#endif

};