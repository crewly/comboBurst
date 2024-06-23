#pragma once
#include <Geode/loader/SettingNode.hpp>
#include <Geode/Geode.hpp>
#include <string>
#include <vector>

using namespace geode::prelude;

class MySettingValue;
class MySettingNode;

class MySettingValue : public SettingValue {
    // store the current value in some form.
    // this may be an enum, a class, or
    // whatever it is your setting needs -
    // you are free to do whatever!
	int m_someMember;

public:
    // Make sure to have a public constructor!
    // Typically you always have these first two args,
    // since Mod::addCustomSetting expects them.
    MySettingValue(std::string const& key, std::string const& mod, int someValue)
      : SettingValue(key, mod), m_someMember(someValue) {}

    bool load(matjson::Value const& json) override {
        // load the value of the setting from json,
        // returning true if loading was succesful
    }
    bool save(matjson::Value& json) const override {
        // save the value of the setting into json,
        // returning true if saving was succesful
    }
    SettingNode* createNode(float width) override {
        return MySettingNode::create(width);
    }

    // getters and setters for the value
};

class MySettingNode : public SettingNode {
protected:
    bool init(MySettingValue* value, float width) {
        if (!SettingNode::init(value))
            return false;

        // You may change the height to anything, but make sure to call
        // setContentSize!
        this->setContentSize({ width, 40.f });

        // Set up the UI. Note that Geode provides a background for the
        // setting automatically

        return true;
    }

    // Whenever the user interacts with your controls, you should call
    // this->dispatchChanged()

public:
    // When the user wants to save this setting value, this function is
    // called - this is where you should actually set the value of your
    // setting
    void commit() override {
        // Set the actual value

        // Let the UI know you have committed the value
        this->dispatchCommitted();
    }

    // Geode calls this to query if the setting value has been changed,
    // and those changes haven't been committed
    bool hasUncommittedChanges() override {
        // todo
    }

    // Geode calls this to query if the setting has a value that is
    // different from its default value
    bool hasNonDefaultValue() override {
        // todo
    }

    // Geode calls this to reset the setting's value back to default
    void resetToDefault() override {
        // todo
    }

    static MySettingNode* create(MySettingValue* value, float width) {
        auto ret = new MySettingNode();
        if (ret->init(value, width)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
