#pragma once

#include "ComboBurst.hpp"
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>

// If you use PCH these are most likely not necessary
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/FLAlertLayer.hpp>

using namespace geode::prelude;

// Inherit from SettingV3 directly over SettingBaseValueV3, as our setting 
// doesn't actually control any value, but it just a simple button
class MyButtonSettingV3 : public SettingV3 {
public:
    // Once again implement the parse function
    static Result<std::shared_ptr<SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
        auto res = std::make_shared<MyButtonSettingV3>();
        auto root = checkJson(json, "MyButtonSettingV3");

        // `parseBaseProperties` parses all base properties, including 
        // `requires-restart` - which does not really make sense in our use case, 
        // as there's nothing in this setting that could require a restart.
        // So, we instead parse the base properties that actually apply to this 
        // setting manually:
        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);
        
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    // Since there's no data to save or load, these can just return true
    // Although you could use these for example to store how many times the 
    // button has been clicked 
    bool load(matjson::Value const& json) override {
        return true;
    }
    bool save(matjson::Value& json) const override {
        return true;
    }

    // This setting can't ever have anything but the default value, as it has 
    // no value
    bool isDefaultValue() const override {
        return true;
    }
    void reset() override {}

    // Once again defined out-of-line
    SettingNodeV3* createNode(float width) override;
};

// We are inheriting from `SettingNodeV3` directly again, as we don't need the 
// boilerplate `SettingValueNodeV3` fills in for us because our setting has no 
// value!
class MyButtonSettingNodeV3 : public SettingNodeV3 {
protected:
    CCSprite* m_buttonSprite;
    CCMenuItemSpriteExtra* m_button;

    bool init(std::shared_ptr<MyButtonSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;
    
        // We just create the button and add it to the setting's menu
		m_buttonSprite = CCSprite::createWithSpriteFrameName("folderIcon_001.png");
        m_buttonSprite->setScale(0.75f);
		m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite, this, menu_selector(MyButtonSettingNodeV3::onButton)
        );
        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->setContentWidth(60);
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);
        
        return true;
    }
    
    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);

        // In case there's an "enable-if" condition on this button, for example 
        // if this played a test notification in a notification mod and you 
        // want to disable the button if notifications are disabled entirely
        auto shouldEnable = this->getSetting()->shouldEnable();
        m_button->setEnabled(shouldEnable);
        m_buttonSprite->setCascadeColorEnabled(true);
        m_buttonSprite->setCascadeOpacityEnabled(true);
        m_buttonSprite->setOpacity(shouldEnable ? 255 : 155);
        m_buttonSprite->setColor(shouldEnable ? ccWHITE : ccGRAY);
    }
    void onButton(CCObject*) {
		auto path = getSpriteDir();
		if (!std::filesystem::exists(path)) {
			try {
				std::filesystem::create_directory(path);
			} catch (std::filesystem::filesystem_error& e) {
				log::error("Couldn't create custom sprite directory:"
							"{}", e.what());
			}
		}
		file::openFolder(
			path
		);
    }

    // Both of these can just be no-ops, since they make no sense for our 
    // setting as it's just a button
    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static MyButtonSettingNodeV3* create(std::shared_ptr<MyButtonSettingV3> setting, float width) {
        auto ret = new MyButtonSettingNodeV3();
        if (ret && ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    // Both of these can just return false, since they make no sense for our 
    // setting as it's just a button
    bool hasUncommittedChanges() const override {
        return false;
    }
    bool hasNonDefaultValue() const override {
        return false;
    }

    // This is not necessary, but it makes it so you don't have to do the 
    // pointer cast every time you want to access the properties of the button 
    // setting
    std::shared_ptr<MyButtonSettingV3> getSetting() const {
        return std::static_pointer_cast<MyButtonSettingV3>(SettingNodeV3::getSetting());
    }
};


// Register as before
$execute {
    (void)Mod::get()->registerCustomSettingType("sprite-folder", &MyButtonSettingV3::parse);
}
