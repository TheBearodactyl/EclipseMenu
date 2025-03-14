#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/LevelInfoLayer.hpp>

namespace eclipse::hacks::Bypass {
    class $hack(CopyBypass) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.creator");
            tab->addToggle("bypass.copybypass")->handleKeybinds()->setDescription();
        }

        [[nodiscard]] const char* getId() const override { return "Level Copy Bypass"; }
    };

    REGISTER_HACK(CopyBypass)

    class $modify(CopyBypassLILHook, LevelInfoLayer) {
        struct Fields {
            int password;
        };

        bool init(GJGameLevel* level, bool challenge) {
            m_fields->password = level->m_password;

            if (config::get<bool>("bypass.copybypass", false))
                level->m_password = 1;

            return LevelInfoLayer::init(level, challenge);
        }

        void onBack(cocos2d::CCObject* sender) {
            this->m_level->m_password = m_fields->password;

            LevelInfoLayer::onBack(sender);
        }

        void confirmClone(cocos2d::CCObject* sender) {
            this->m_level->m_password = m_fields->password;

            LevelInfoLayer::confirmClone(sender);
        }
    };
}
