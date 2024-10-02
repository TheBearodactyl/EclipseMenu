#include "manager.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <modules/config/config.hpp>
#include <modules/gui/imgui/imgui.hpp>
#include <Geode/Loader.hpp>
#include <Geode/utils/cocos.hpp>

namespace eclipse::gui {
    static auto IMPORT_PICK_OPTIONS = geode::utils::file::FilePickOptions {
        std::nullopt,
        {
            {
                "Eclipse Themes",
                { "*.zip" }
            }
        }
    };

    void ThemeManager::init() {
        if (!loadTheme(geode::Mod::get()->getSaveDir() / "theme.json")) {
            // theme not created, load a built-in one
            auto themes = listAvailableThemes();
            if (themes.empty()) {
                // okay, we're screwed, just set the defaults
                return setDefaults();
            }

            // TODO: add a priority for default theme
            loadTheme(themes[0].path);
        }

        // load values into temp storage for use with other components
        applyValues(config::getTempStorage(), true);
    }

    void ThemeManager::setDefaults() {
#ifdef GEODE_IS_DESKTOP
        m_renderer = RendererType::ImGui;
        m_layoutMode = imgui::LayoutMode::Tabbed;
#else
        m_renderer = RendererType::Cocos2d;
        m_layoutMode = imgui::LayoutMode::Panel;
#endif

        m_componentTheme = imgui::ComponentTheme::MegaHack;

        m_uiScale = 1.f;
        m_borderSize = 1.f;
        m_windowRounding = 0.f;
        m_frameRounding = 4.f;
        m_windowMargin = 4.f;

        m_enableBlur = true;
        m_blurSpeed = 0.3f;
        m_blurRadius = 1.f;

        // TODO: fill this after all properties are figured out
    }

    std::shared_ptr<ThemeManager> ThemeManager::get() {
        static std::shared_ptr<ThemeManager> instance;
        if (!instance) {
            instance = std::make_shared<ThemeManager>();
            instance->init();
        }
        return instance;
    }

    template <typename T>
    std::optional<T> json_try_get(nlohmann::json const& j, std::string_view key) {
        if (!j.is_object()) return std::nullopt;
        if (!j.contains(key)) return std::nullopt;
        return j.at(key).get<T>();
    }

    template <typename T>
    void try_assign(T& v, nlohmann::json const& j, std::string_view key) {
        auto value = json_try_get<T>(j, key);
        if (value) v = *value;
        else geode::log::warn("Failed to read \"{}\" from theme", key);
    }

    bool ThemeManager::loadTheme(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) return false;
        std::ifstream file(path);
        if (!file.is_open()) return false;

        auto json = nlohmann::json::parse(file, nullptr);
        if (json.is_discarded()) return false;

        setDefaults();
        auto details = json["details"];
        try_assign(m_themeName, details, "name");
        try_assign(m_themeDescription, details, "description");
        try_assign(m_themeAuthor, details, "author");

        auto renderer = json_try_get<int>(details, "renderer");
        auto layout = json_try_get<int>(details, "layout");
        auto theme = json_try_get<int>(details, "style");

        if (renderer) this->setRenderer(static_cast<RendererType>(*renderer));
        if (layout) this->setLayoutMode(static_cast<imgui::LayoutMode>(*layout));
        if (theme) this->setComponentTheme(static_cast<imgui::ComponentTheme>(*theme));

        auto other = json["other"];
        try_assign(m_uiScale, other, "uiScale");
        try_assign(m_selectedFont, other, "font");
        try_assign(m_fontSize, other, "fontSize");
        try_assign(m_framePadding, other, "framePadding");
        try_assign(m_windowMargin, other, "windowMargin");
        try_assign(m_windowRounding, other, "windowRounding");
        try_assign(m_frameRounding, other, "frameRounding");
        try_assign(m_borderSize, other, "borderSize");

        // blur
        auto blur = json["blur"];
        try_assign(m_enableBlur, blur, "blurEnabled");
        try_assign(m_blurSpeed, blur, "blurSpeed");
        try_assign(m_blurRadius, blur, "blurRadius");

        // overrides
        auto colors = json["colors"];
        try_assign(m_backgroundColor, colors, "backgroundColor");
        try_assign(m_foregroundColor, colors, "foregroundColor");
        try_assign(m_frameBackground, colors, "frameBackground");
        try_assign(m_disabledColor, colors, "disabledColor");
        try_assign(m_borderColor, colors, "borderColor");
        try_assign(m_titleBackgroundColor, colors, "titleBackgroundColor");
        try_assign(m_titleForegroundColor, colors, "titleForegroundColor");
        try_assign(m_checkboxBackgroundColor, colors, "checkboxBackgroundColor");
        try_assign(m_checkboxCheckmarkColor, colors, "checkboxCheckmarkColor");
        try_assign(m_checkboxForegroundColor, colors, "checkboxForegroundColor");
        try_assign(m_buttonBackgroundColor, colors, "buttonBackgroundColor");
        try_assign(m_buttonForegroundColor, colors, "buttonForegroundColor");
        try_assign(m_buttonDisabledColor, colors, "buttonDisabledColor");
        try_assign(m_buttonDisabledForeground, colors, "buttonDisabledForeground");
        try_assign(m_buttonHoveredColor, colors, "buttonHoveredColor");
        try_assign(m_buttonHoveredForeground, colors, "buttonHoveredForeground");
        try_assign(m_buttonActivatedColor, colors, "buttonActivatedColor");
        try_assign(m_buttonActiveForeground, colors, "buttonActiveForeground");

        return true;
    }

    void ThemeManager::saveTheme(const std::filesystem::path& path) const {
        std::ofstream file(path);
        if (!file.is_open()) return;

        nlohmann::json json;
        applyValues(json);

        file << json.dump(4);
        file.close();
    }

    void ThemeManager::saveTheme() const {
        saveTheme(geode::Mod::get()->getSaveDir() / "theme.json");
    }

    void ThemeManager::applyValues(nlohmann::json& json, bool flatten) const {
        auto& details = flatten ? json : json["details"];
        auto& blur = flatten ? json : json["blur"];
        auto& other = flatten ? json : json["other"];
        auto& colors = flatten ? json : json["colors"];

        details["name"] = m_themeName;
        details["description"] = m_themeDescription;
        details["author"] = m_themeAuthor;
        details["renderer"] = m_renderer;
        details["layout"] = m_layoutMode;
        details["style"] = m_componentTheme;

        blur["blurEnabled"] = m_enableBlur;
        blur["blurSpeed"] = m_blurSpeed;
        blur["blurRadius"] = m_blurRadius;

        other["uiScale"] = m_uiScale;
        other["font"] = m_selectedFont;
        other["fontSize"] = m_fontSize;
        other["framePadding"] = m_framePadding;
        other["windowMargin"] = m_windowMargin;
        other["windowRounding"] = m_windowRounding;
        other["frameRounding"] = m_frameRounding;
        other["borderSize"] = m_borderSize;

        colors["backgroundColor"] = m_backgroundColor;
        colors["frameBackground"] = m_frameBackground;
        colors["foregroundColor"] = m_foregroundColor;
        colors["disabledColor"] = m_disabledColor;
        colors["borderColor"] = m_borderColor;
        colors["titleBackgroundColor"] = m_titleBackgroundColor;
        colors["titleForegroundColor"] = m_titleForegroundColor;
        colors["checkboxBackgroundColor"] = m_checkboxBackgroundColor;
        colors["checkboxCheckmarkColor"] = m_checkboxCheckmarkColor;
        colors["checkboxForegroundColor"] = m_checkboxForegroundColor;
        colors["buttonBackgroundColor"] = m_buttonBackgroundColor;
        colors["buttonForegroundColor"] = m_buttonForegroundColor;
        colors["buttonDisabledColor"] = m_buttonDisabledColor;
        colors["buttonDisabledForeground"] = m_buttonDisabledForeground;
        colors["buttonHoveredColor"] = m_buttonHoveredColor;
        colors["buttonHoveredForeground"] = m_buttonHoveredForeground;
        colors["buttonActivatedColor"] = m_buttonActivatedColor;
        colors["buttonActiveForeground"] = m_buttonActiveForeground;
    }

    bool ThemeManager::importTheme(const std::filesystem::path& path) {
        return false;
    }

    void ThemeManager::exportTheme(const std::filesystem::path &path) {

    }

    float ThemeManager::getGlobalScale() const {
        return m_uiScale * config::getTemp<float>("ui.scale", 1.f) * imgui::DEFAULT_SCALE;
    }

    std::optional<ThemeMeta> ThemeManager::checkTheme(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) return std::nullopt;
        std::ifstream file(path);
        if (!file.is_open()) return std::nullopt;

        auto json = nlohmann::json::parse(file, nullptr);
        if (json.is_discarded()) return std::nullopt;

        auto details = json["details"];
        auto name = json_try_get<std::string>(details, "name");
        if (!name) return std::nullopt;

        return ThemeMeta { name.value(), path };
    }

    std::vector<ThemeMeta> ThemeManager::listAvailableThemes() {
        std::vector<ThemeMeta> themes;
        auto globThemes = [&](std::filesystem::path const& path) {
            std::filesystem::create_directories(path);
            for (auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.path().extension() != ".json") continue;
                if (auto theme = checkTheme(entry.path()))
                    themes.push_back(*theme);
            }
        };

        globThemes(geode::Mod::get()->getResourcesDir());
        globThemes(geode::Mod::get()->getConfigDir() / "themes");

        return themes;
    }

    void ThemeManager::setRenderer(RendererType renderer) {
        auto engine = Engine::get();
        if (engine->isInitialized()) {
            engine->setRenderer(renderer);
        }
        m_renderer = renderer;
    }

    void ThemeManager::setLayoutMode(imgui::LayoutMode mode) {
        if (auto imgui = imgui::ImGuiRenderer::get()) {
            geode::log::debug("ThemeManager::setLayoutMode - setting new layout");
            imgui->setLayoutMode(mode);
        }
        m_layoutMode = mode;
    }

    void ThemeManager::setComponentTheme(imgui::ComponentTheme theme) {
        if (auto imgui = imgui::ImGuiRenderer::get()) {
            imgui->setComponentTheme(theme);
        }
        m_componentTheme = theme;
    }

    void ThemeManager::setSelectedFont(const std::string &value) {
        if (auto imgui = imgui::ImGuiRenderer::get()) {
            imgui->getFontManager().setFont(value);
        }
        m_selectedFont = value;
    }

    void ThemeManager::setSelectedFont(int index) {
        auto fonts = getFontNames();
        if (fonts.size() <= index) return;
        setSelectedFont(fonts[index]);
    }

    std::vector<std::string> ThemeManager::getFontNames() {
        auto fonts = imgui::FontManager::fetchAvailableFonts();
        std::vector<std::string> result;
        result.reserve(fonts.size());
        for (auto& font : fonts) {
            result.emplace_back(font.getName());
        }
        return result;
    }

    void ThemeManager::setFontSize(float value) {
        m_fontSize = value;
    }
}
