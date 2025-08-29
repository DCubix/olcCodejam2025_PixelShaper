#pragma once

#include "olcPixelGameEngine.h"

#include <map>
#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include <functional>

using wid = uint64_t;

constexpr wid gNullWidget = 0;

// rectcut rectangle
struct Rect {
    int xMin{ 0 }, yMin{ 0 }, xMax{ 0 }, yMax{ 0 };
};

enum class Alignment {
    Left,
    Center,
    Right
};

enum class PanelStyle {
    Flat,
    Raised,
    Sunken
};

enum class WidgetState {
    Normal,
    Hovered,
    Active,
    Clicked,
    Unfocused
};

enum class DragAxis : uint8_t {
    Horizontal = (1 << 0),
    Vertical = (1 << 1),
    Both = Horizontal | Vertical
};

// Enable bitwise operations for DragAxis
inline DragAxis operator|(DragAxis a, DragAxis b) {
    return static_cast<DragAxis>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline DragAxis operator&(DragAxis a, DragAxis b) {
    return static_cast<DragAxis>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool operator!=(DragAxis a, uint8_t b) {
    return static_cast<uint8_t>(a) != b;
}

// A widget structure to hold relative values and mouse actions inside of a widget
struct Widget {
    Rect rect;
    WidgetState state{ WidgetState::Normal };
    int mouseButton{ -1 };

    // Unique ID for the widget
    wid id{ gNullWidget };
};

/**
 * Immediate Mode GUI
 */
class GUI {
public:
    GUI(olc::PixelGameEngine* pge) : mPGE(pge) {}

    GUI& CutLeft(int width);
    GUI& CutRight(int width);
    GUI& CutTop(int height);
    GUI& CutBottom(int height);

    GUI& CutLeft(float ratio);
    GUI& CutRight(float ratio);
    GUI& CutTop(float ratio);
    GUI& CutBottom(float ratio);

    void AddIcon(const std::string& path);

    GUI& Text(
        const std::string& text,
        Alignment align = Alignment::Left,
        const olc::Pixel& color = olc::WHITE,
        bool shadow = false
    );

    GUI& Panel(
        PanelStyle style = PanelStyle::Flat,
        const olc::Pixel& color = olc::DARK_GREY,
        uint32_t padding = 0
    );

    GUI& Spacer();

    bool Button(
        const std::string& id,
        const std::string& text,
        const olc::Pixel& color = olc::VERY_DARK_GREY
    );

    bool HSlider(
        const std::string& id,
        int& value,
        int min = 0,
        int max = 100,
        const olc::Pixel& color = olc::WHITE,
        bool showValue = true
    );

    bool VSlider(
        const std::string& id,
        int& value,
        int min = 0,
        int max = 100,
        const olc::Pixel& color = olc::WHITE,
        bool showValue = true
    );

    bool CheckBox(
        const std::string& id,
        const std::string& label,
        bool& value,
        const olc::Pixel& color = olc::WHITE,
        const olc::Pixel& labelColor = olc::WHITE
    );

    bool ToggleButton(
        const std::string& id,
        const std::string& text,
        bool& value,
        const olc::Pixel& color = olc::WHITE
    );

    void TabBar(
        const std::vector<std::string>& tabs,
        int& activeTab,
        const olc::Pixel& color = olc::WHITE
    );

    bool EditBox(
        const std::string& id,
        std::string& value,
        const std::function<bool(const std::string&)>& validator,
        const olc::Pixel& color = olc::WHITE
    );

    bool Spinner(
        const std::string& id,
        int& value,
        int min = 0,
        int max = 100,
        int step = 1,
        const olc::Pixel& color = olc::WHITE
    );

    bool ColorPicker(
        const std::string& id,
        olc::Pixel& color
    );

    Widget& GetWidget(const std::string& id);
    bool WasClicked(const std::string& id);

    // Adjust the brightness of a color (0.0 - 1.0)
    olc::Pixel AdjustValue(const olc::Pixel& color, float value);

    Rect PopLayout();
    const Rect& PeekLayout() const;
    void PushLayout(const Rect& rect);
    void PushLayout(int x, int y, int width, int height);

    olc::PixelGameEngine* GetPGE() const { return mPGE; }

    void Begin();
    void End();

    static wid ID(const std::string& name);
    static void Init(olc::PixelGameEngine* pge);
    static std::unique_ptr<GUI> sInstance;
private:
    struct {
        wid focused{ gNullWidget };
        wid lastFocused{ gNullWidget };
        wid hovered{ gNullWidget };
        wid active{ gNullWidget };

        int mouseX{ 0 };
        int mouseY{ 0 };
        int mouseButton{ -1 }; // -1 = no button, 0 = left, 1 = middle, 2 = right
    } mState;

    struct SpinnerData {
        std::string text;
    };

    struct ColorPickerData {
        float hue{ 0.0f };        // 0-360
        float saturation{ 1.0f }; // 0-1
        float brightness{ 1.0f }; // 0-1
    };

    // HSV to RGB conversion
    static olc::Pixel HSVtoRGB(float h, float s, float v);
    static void RGBtoHSV(const olc::Pixel& rgb, float& h, float& s, float& v);

    std::map<wid, Widget> mWidgets;
    std::map<wid, SpinnerData> mSpinnerStates;
    std::map<wid, ColorPickerData> mColorPickerStates;

    std::vector<Rect> mLayoutStack;
    std::vector<std::function<void(olc::PixelGameEngine*)>> mLateDrawFuncs;
    std::vector<std::unique_ptr<olc::Sprite>> mIcons;
    bool mBlink{ false };
    float mBlinkTime{ 0.0f };

    olc::PixelGameEngine* mPGE;

    Widget& TabToggleButton(
        const std::string& id,
        const std::string& text,
        bool value,
        const olc::Pixel& color = olc::WHITE
    );

    void LineEditor(
        Widget& widget,
        std::string& value,
        const std::function<bool(const std::string&)>& validator
    );
};

#define gui (*GUI::sInstance.get())