#include "gui.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <regex>

std::unique_ptr<GUI> GUI::sInstance;

static float Luma(const olc::Pixel& color) {
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

GUI &GUI::CutLeft(int width)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");
    
    Rect& rect = mLayoutStack.back();
    
    int xMin = rect.xMin;
    rect.xMin = std::min(rect.xMax, rect.xMin + width);
    mLayoutStack.push_back({ xMin, rect.yMin, rect.xMin, rect.yMax });

    return *this;
}

GUI &GUI::CutRight(int width)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");

    Rect& rect = mLayoutStack.back();
    int xMax = rect.xMax;
    rect.xMax = std::max(rect.xMin, rect.xMax - width);
    mLayoutStack.push_back({ rect.xMax, rect.yMin, xMax, rect.yMax });

    return *this;
}

GUI &GUI::CutTop(int height)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");

    Rect& rect = mLayoutStack.back();
    int yMin = rect.yMin;
    rect.yMin = std::min(rect.yMax, rect.yMin + height);
    mLayoutStack.push_back({ rect.xMin, yMin, rect.xMax, rect.yMin });

    return *this;
}

GUI &GUI::CutBottom(int height)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");

    Rect& rect = mLayoutStack.back();
    int yMax = rect.yMax;
    rect.yMax = std::max(rect.yMin, rect.yMax - height);
    mLayoutStack.push_back({ rect.xMin, rect.yMax, rect.xMax, yMax });

    return *this;
}

GUI &GUI::CutLeft(float ratio)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");

    Rect& rect = mLayoutStack.back();
    int width = static_cast<int>((rect.xMax - rect.xMin) * ratio);
    return CutLeft(width);
}

GUI &GUI::CutRight(float ratio)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");

    Rect& rect = mLayoutStack.back();
    int width = static_cast<int>((rect.xMax - rect.xMin) * ratio);
    return CutRight(width);
}

GUI &GUI::CutTop(float ratio)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");

    Rect& rect = mLayoutStack.back();
    int height = static_cast<int>((rect.yMax - rect.yMin) * ratio);
    return CutTop(height);
}

GUI &GUI::CutBottom(float ratio)
{
    assert(!mLayoutStack.empty() && "Cannot cut without a layout stack");

    Rect& rect = mLayoutStack.back();
    int height = static_cast<int>((rect.yMax - rect.yMin) * ratio);
    return CutBottom(height);
}

void GUI::AddIcon(const std::string &path)
{
    mIcons.push_back(std::make_unique<olc::Sprite>(path));
}

GUI &GUI::Text(
    const std::string &text,
    Alignment align,
    const olc::Pixel &color,
    bool shadow
)
{
    assert(!mLayoutStack.empty() && "Cannot draw text without a layout stack");

    Rect layout = PopLayout();

    // Parse the text for icon placeholders and build render elements
    struct RenderElement
    {
        enum Type { TEXT, ICON, NEWLINE };
        Type type;
        std::string textContent;
        int iconIndex;
        int width;
        int height;
    };
    
    using RenderLine = std::vector<RenderElement>;
    
    // Parse text into elements
    auto parseTextElements = [&](const std::string& inputText) -> std::vector<RenderElement> {
        std::vector<RenderElement> elements;
        size_t pos = 0;
        
        while (pos < inputText.length())
        {
            // Check for newline first
            size_t newlinePos = inputText.find('\n', pos);
            size_t iconStart = inputText.find("$[", pos);
            
            // Determine which comes first
            size_t nextSpecial = std::min(newlinePos, iconStart);
            
            if (nextSpecial == std::string::npos)
            {
                // No more special characters, add remaining text
                if (pos < inputText.length())
                {
                    std::string textPart = inputText.substr(pos);
                    if (!textPart.empty())
                    {
                        auto textSize = mPGE->GetTextSizeProp(textPart);
                        elements.push_back({RenderElement::TEXT, textPart, -1, textSize.x, textSize.y});
                    }
                }
                break;
            }
            
            // Add text before the special character (if any)
            if (nextSpecial > pos)
            {
                std::string textPart = inputText.substr(pos, nextSpecial - pos);
                auto textSize = mPGE->GetTextSizeProp(textPart);
                elements.push_back({RenderElement::TEXT, textPart, -1, textSize.x, textSize.y});
            }
            
            if (nextSpecial == newlinePos)
            {
                // Handle newline
                elements.push_back({RenderElement::NEWLINE, "", -1, 0, 0});
                pos = newlinePos + 1;
            }
            else if (nextSpecial == iconStart)
            {
                // Handle icon
                size_t iconEnd = inputText.find("]", iconStart + 2);
                if (iconEnd != std::string::npos)
                {
                    std::string indexStr = inputText.substr(iconStart + 2, iconEnd - iconStart - 2);
                    try
                    {
                        int iconIndex = std::stoi(indexStr);
                        if (iconIndex >= 0 && iconIndex < static_cast<int>(mIcons.size()))
                        {
                            auto* sprite = mIcons[iconIndex].get();
                            elements.push_back({RenderElement::ICON, "", iconIndex, sprite->width, sprite->height});
                        }
                        else
                        {
                            // Invalid icon index, treat as regular text
                            std::string textPart = inputText.substr(iconStart, iconEnd - iconStart + 1);
                            auto textSize = mPGE->GetTextSizeProp(textPart);
                            elements.push_back({RenderElement::TEXT, textPart, -1, textSize.x, textSize.y});
                        }
                    }
                    catch (const std::exception&)
                    {
                        // Invalid icon index, treat as regular text
                        std::string textPart = inputText.substr(iconStart, iconEnd - iconStart + 1);
                        auto textSize = mPGE->GetTextSizeProp(textPart);
                        elements.push_back({RenderElement::TEXT, textPart, -1, textSize.x, textSize.y});
                    }
                    pos = iconEnd + 1;
                }
                else
                {
                    // No closing bracket found, treat as regular text
                    std::string textPart = inputText.substr(iconStart, 2);
                    auto textSize = mPGE->GetTextSizeProp(textPart);
                    elements.push_back({RenderElement::TEXT, textPart, -1, textSize.x, textSize.y});
                    pos = iconStart + 2;
                }
            }
        }
        
        return elements;
    };
    
    // Split elements into lines
    auto splitIntoLines = [](const std::vector<RenderElement>& elements) -> std::vector<RenderLine> {
        std::vector<RenderLine> lines;
        RenderLine currentLine;
        
        for (const auto& element : elements)
        {
            if (element.type == RenderElement::NEWLINE)
            {
                lines.push_back(currentLine);
                currentLine.clear();
            }
            else
            {
                currentLine.push_back(element);
            }
        }
        
        // Add the last line if it has content
        if (!currentLine.empty() || lines.empty())
        {
            lines.push_back(currentLine);
        }
        
        return lines;
    };
    
    // Calculate line dimensions
    auto calculateLineDimensions = [&](const RenderLine& line) -> std::pair<int, int> {
        int width = 0;
        int height = 0;
        
        for (const auto& element : line)
        {
            width += element.width;
            height = std::max(height, element.height);
        }
        
        // Ensure minimum height for empty lines
        if (height == 0)
        {
            height = mPGE->GetTextSizeProp("A").y; // Use height of 'A' as baseline
        }
        
        return {width, height};
    };
    
    // Calculate alignment offset
    auto calculateLineOffset = [](int lineWidth, int layoutWidth, Alignment alignment) -> int {
        switch (alignment)
        {
            case Alignment::Left:
                return 0;
            case Alignment::Center:
                return (layoutWidth - lineWidth) / 2;
            case Alignment::Right:
                return layoutWidth - lineWidth;
            default:
                return 0;
        }
    };
    
    // Render a single line
    auto renderLine = [&](const RenderLine& line, int x, int y, int lineHeight) {
        int currentX = x;
        
        for (const auto& element : line)
        {
            // Center each element vertically within the line
            int elementY = y + (lineHeight - element.height) / 2;
            
            if (element.type == RenderElement::TEXT)
            {
                if (shadow)
                {
                    mPGE->DrawStringProp(
                        currentX + 1,
                        elementY + 1,
                        element.textContent,
                        olc::BLACK
                    );
                }

                mPGE->DrawStringProp(
                    currentX,
                    elementY,
                    element.textContent,
                    color
                );
            }
            else if (element.type == RenderElement::ICON)
            {
                mPGE->DrawSprite(currentX, elementY, mIcons[element.iconIndex].get());
            }
            
            currentX += element.width;
        }
    };

    // Parse text into elements (text, icons, newlines)
    auto elements = parseTextElements(text);
    
    // Split elements into lines
    auto lines = splitIntoLines(elements);
    
    // Calculate dimensions for each line
    std::vector<std::pair<int, int>> lineDimensions; // width, height
    int totalHeight = 0;
    
    for (const auto& line : lines)
    {
        auto [lineWidth, lineHeight] = calculateLineDimensions(line);
        lineDimensions.push_back({lineWidth, lineHeight});
        totalHeight += lineHeight;
    }
    
    // Calculate starting Y position to center the entire text block vertically
    int startY = layout.yMin + (layout.yMax - layout.yMin - totalHeight) / 2;
    
    // Render each line
    int currentY = startY;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const auto& line = lines[i];
        const auto& [lineWidth, lineHeight] = lineDimensions[i];
        
        // Calculate X offset for this line based on alignment
        int xOffset = calculateLineOffset(lineWidth, layout.xMax - layout.xMin, align);
        int lineX = layout.xMin + xOffset;
        
        // Render the line
        renderLine(line, lineX, currentY, lineHeight);
        
        currentY += lineHeight;
    }

    return *this;
}

GUI &GUI::Panel(
    PanelStyle style,
    const olc::Pixel &color,
    uint32_t padding
)
{
    assert(!mLayoutStack.empty() && "Cannot draw panel without a layout stack");

    Rect layout = PopLayout();

    mPGE->FillRect(
        layout.xMin,
        layout.yMin,
        layout.xMax - layout.xMin,
        layout.yMax - layout.yMin,
        color
    );

    switch (style)
    {
        case PanelStyle::Flat:
            mPGE->DrawRect(
                layout.xMin,
                layout.yMin,
                layout.xMax - layout.xMin - 1,
                layout.yMax - layout.yMin - 1,
                AdjustValue(color, 0.45f)
            );
            break;
        case PanelStyle::Raised:
            mPGE->DrawLine(
                layout.xMin,
                layout.yMin,
                layout.xMax - 1,
                layout.yMin,
                AdjustValue(color, 2.0f)
            );
            mPGE->DrawLine(
                layout.xMin,
                layout.yMin,
                layout.xMin,
                layout.yMax - 1,
                AdjustValue(color, 2.0f)
            );
            mPGE->DrawLine(
                layout.xMax - 1,
                layout.yMin,
                layout.xMax - 1,
                layout.yMax - 1,
                AdjustValue(color, 0.5f)
            );
            mPGE->DrawLine(
                layout.xMin,
                layout.yMax - 1,
                layout.xMax - 1,
                layout.yMax - 1,
                AdjustValue(color, 0.5f)
            );
            break;
        case PanelStyle::Sunken:
            mPGE->DrawLine(
                layout.xMin,
                layout.yMin,
                layout.xMax - 1,
                layout.yMin,
                AdjustValue(color, 0.5f)
            );
            mPGE->DrawLine(
                layout.xMin,
                layout.yMin,
                layout.xMin,
                layout.yMax - 1,
                AdjustValue(color, 0.5f)
            );
            mPGE->DrawLine(
                layout.xMax - 1,
                layout.yMin,
                layout.xMax - 1,
                layout.yMax - 1,
                AdjustValue(color, 2.0f)
            );
            mPGE->DrawLine(
                layout.xMin,
                layout.yMax - 1,
                layout.xMax - 1,
                layout.yMax - 1,
                AdjustValue(color, 2.0f)
            );
            break;
    }

    // Give the layout back to the stack
    layout.xMin += padding;
    layout.yMin += padding;
    layout.xMax -= padding;
    layout.yMax -= padding;
    mLayoutStack.push_back(layout);

    return *this;
}

GUI &GUI::Spacer()
{
    PopLayout();
    return *this;
}

bool GUI::Button(
    const std::string& id,
    const std::string& text,
    const olc::Pixel& color,
    bool enabled
)
{
    auto& widget = GetWidget(id);

    if (enabled)
    {
        switch (widget.state)
        {
            case WidgetState::Clicked:
            case WidgetState::Normal:
                Panel(PanelStyle::Raised, color, 2);
                break;
            case WidgetState::Hovered:
                Panel(PanelStyle::Raised, AdjustValue(color, 1.2f), 2);
                break;
            case WidgetState::Active:
                Panel(PanelStyle::Sunken, AdjustValue(color, 0.8f), 2);
                break;
        }
    }
    else
    {
        Panel(PanelStyle::Flat, AdjustValue(color, 0.7f), 2);
    }

    auto textColor = Luma(color) > 0.45f ? olc::BLACK : olc::WHITE;
    textColor.a = enabled ? 255 : 128;

    Text(text, Alignment::Center, textColor);

    return enabled ?
        widget.state == WidgetState::Clicked
        : false;
}

bool GUI::HSlider(
    const std::string &id,
    int &value,
    int min, int max,
    const olc::Pixel &color,
    bool showValue
)
{
    const int thumbWidth = 8;
    auto& widget = GetWidget(id);

    // Track
    Panel(PanelStyle::Sunken, AdjustValue(color, 0.25f), 1);

    auto rect = PeekLayout();

    // Handle
    float ratio = float(value - min) / (max - min);
    int trackXMin = rect.xMin + thumbWidth / 2;
    int trackXMax = rect.xMax - thumbWidth / 2;
    int xPos = trackXMin + static_cast<int>(ratio * (trackXMax - trackXMin));

    // we push the layout for the handle
    PushLayout(
        xPos - thumbWidth / 2,
        rect.yMin,
        thumbWidth,
        rect.yMax - rect.yMin
    );
    Panel(PanelStyle::Raised, color);
    PopLayout();

    if (showValue && widget.state != WidgetState::Normal)
    {
        std::string valueText = std::to_string(value); // TODO: Add a formatting function
        const auto textSize = mPGE->GetTextSizeProp(valueText);
        int valuePanelWidth = textSize.x + 6;
        int valuePanelHeight = textSize.y + 4;
        int valuePanelX = xPos - valuePanelWidth / 2;
        int valuePanelY = rect.yMin - (valuePanelHeight + 2);

        if (valuePanelX < 0)
            valuePanelX = rect.xMin;
        else if (valuePanelX + valuePanelWidth > mPGE->ScreenWidth())
            valuePanelX = mPGE->ScreenWidth() - valuePanelWidth;

        if (valuePanelY < 0)
            valuePanelY = rect.yMax + 2;
        else if (valuePanelY + valuePanelHeight > mPGE->ScreenHeight())
            valuePanelY = mPGE->ScreenHeight() - valuePanelHeight;

        auto fnDrawValuePanel = [=](olc::PixelGameEngine* pge) {
            pge->FillRect(valuePanelX, valuePanelY, valuePanelWidth, valuePanelHeight, olc::BLACK);
            pge->DrawStringProp(valuePanelX + 3, valuePanelY + 2, valueText, olc::WHITE);
        };
        mLateDrawFuncs.push_back(fnDrawValuePanel);
    }

    // dragging logic
    bool valueChanged = false;
    if (mState.active == widget.id && mState.mouseButton == 0)
    {
        // Calculate the new value based on mouse position
        int dragXMin = rect.xMin + thumbWidth / 2;
        int dragXMax = rect.xMax - thumbWidth / 2;
        int newValue = min + static_cast<int>(
            (mState.mouseX - dragXMin) / float(dragXMax - dragXMin) * (max - min)
        );
        valueChanged = newValue != value;
        value = std::clamp(newValue, min, max);
    }

    PopLayout();

    return valueChanged;
}

bool GUI::VSlider(
    const std::string &id,
    int &value,
    int min, int max,
    const olc::Pixel &color,
    bool showValue
)
{
    const int thumbHeight = 8;
    auto& widget = GetWidget(id);

    // Track
    Panel(PanelStyle::Sunken, AdjustValue(color, 0.25f), 1);

    auto rect = PeekLayout();

    // Handle - inverted so min is at bottom, max at top
    float ratio = float(value - min) / (max - min);
    int trackYMin = rect.yMin + thumbHeight / 2;
    int trackYMax = rect.yMax - thumbHeight / 2;
    int yPos = trackYMax - static_cast<int>(ratio * (trackYMax - trackYMin));

    // we push the layout for the handle
    PushLayout(
        rect.xMin,
        yPos - thumbHeight / 2,
        rect.xMax - rect.xMin,
        thumbHeight
    );
    Panel(PanelStyle::Raised, color);
    PopLayout();

    if (showValue && widget.state != WidgetState::Normal)
    {
        std::string valueText = std::to_string(value); // TODO: Add a formatting function
        const auto textSize = mPGE->GetTextSizeProp(valueText);
        int valuePanelWidth = textSize.x + 6;
        int valuePanelHeight = textSize.y + 4;
        int valuePanelX = rect.xMin - (valuePanelWidth + 2);
        int valuePanelY = yPos - valuePanelHeight / 2;

        if (valuePanelX < 0)
            valuePanelX = rect.xMax + 2;
        else if (valuePanelX + valuePanelWidth > mPGE->ScreenWidth())
            valuePanelX = mPGE->ScreenWidth() - valuePanelWidth;

        if (valuePanelY < 0)
            valuePanelY = rect.yMin;
        else if (valuePanelY + valuePanelHeight > mPGE->ScreenHeight())
            valuePanelY = mPGE->ScreenHeight() - valuePanelHeight;

        auto fnDrawValuePanel = [=](olc::PixelGameEngine* pge) {
            pge->FillRect(valuePanelX, valuePanelY, valuePanelWidth, valuePanelHeight, olc::BLACK);
            pge->DrawStringProp(valuePanelX + 3, valuePanelY + 2, valueText, olc::WHITE);
        };
        mLateDrawFuncs.push_back(fnDrawValuePanel);
    }

    // dragging logic - inverted so min is at bottom, max at top
    bool valueChanged = false;
    if (mState.active == widget.id && mState.mouseButton == 0)
    {
        // Calculate the new value based on mouse position
        int dragYMin = rect.yMin + thumbHeight / 2;
        int dragYMax = rect.yMax - thumbHeight / 2;
        int newValue = min + static_cast<int>(
            (dragYMax - mState.mouseY) / float(dragYMax - dragYMin) * (max - min)
        );
        valueChanged = newValue != value;
        value = std::clamp(newValue, min, max);
    }

    PopLayout();

    return valueChanged;
}

bool GUI::CheckBox(
    const std::string &id,
    const std::string &label,
    bool &value,
    const olc::Pixel &color,
    const olc::Pixel &labelColor
)
{
    auto& widget = GetWidget(id);
    if (widget.state == WidgetState::Clicked)
    {
        value = !value;
    }

    // Calculate vertical centering
    int rectHeight = widget.rect.yMax - widget.rect.yMin;
    int checkboxSize = 12;
    int yOffset = (rectHeight - checkboxSize) / 2;

    // Draw the checkbox
    PushLayout(widget.rect.xMin, widget.rect.yMin + yOffset, checkboxSize, checkboxSize);
    Panel(PanelStyle::Sunken, olc::VERY_DARK_GREY); // bg color
    PopLayout();

    // Tick
    if (value) {
        mPGE->FillRect(
            widget.rect.xMin + 2,
            widget.rect.yMin + yOffset + 2, checkboxSize - 5, checkboxSize - 5,
            color
        );
    }

    PushLayout(widget.rect.xMin + 16, widget.rect.yMin, widget.rect.xMax - widget.rect.xMin - 20, rectHeight);
    Text(label, Alignment::Left, labelColor);
    PopLayout();

    return widget.state == WidgetState::Clicked;
}

bool GUI::ToggleButton(const std::string &id, const std::string &text, bool &value, const olc::Pixel &color)
{
    auto& widget = TabToggleButton(id, text, value, color);
    if (widget.state == WidgetState::Clicked)
    {
        value = !value;
    }

    return widget.state == WidgetState::Clicked;
}

bool GUI::TabBar(const std::vector<std::string> &tabs, int &activeTab, const olc::Pixel &color, bool fitWidth)
{
    bool tabClicked = false;
    int layoutWidth = PeekLayout().xMax - PeekLayout().xMin;
    for (size_t i = 0; i < tabs.size(); ++i)
    {
        int w = fitWidth
            ? layoutWidth / tabs.size()
            : mPGE->GetTextSizeProp(tabs[i]).x + 6;
        CutLeft(w);
        auto& widget = TabToggleButton("tab_" + std::to_string(i) + "_" + tabs[i], tabs[i], activeTab == i, color);
        if (widget.state == WidgetState::Clicked)
        {
            activeTab = static_cast<int>(i);
            tabClicked = true;
        }
    }
    return tabClicked;
}

bool GUI::EditBox(const std::string &id, std::string &value, const std::function<bool(const std::string&)>& validator, const olc::Pixel &color)
{
    auto& widget = GetWidget(id);
    Panel(PanelStyle::Sunken, AdjustValue(color, 0.25f), 2);
    LineEditor(widget, value, validator);
    return widget.state == WidgetState::Unfocused;
}

bool GUI::Spinner(const std::string &id, int &value, int min, int max, int step, const olc::Pixel &color)
{
    bool valueChanged = false;

    CutLeft(16);
    if (Button(id + "_sp_dec", "<", color))
    {
        int oldValue = value;
        value = std::max(min, value - step);
        valueChanged = oldValue != value;
    }

    CutRight(16);
    if (Button(id + "_sp_inc", ">", color))
    {
        int oldValue = value;
        value = std::min(max, value + step);
        valueChanged = oldValue != value;
    }

    auto& widget = GetWidget(id);

    auto it = mSpinnerStates.find(widget.id);
    if (it == mSpinnerStates.end())
    {
        mSpinnerStates[widget.id] = SpinnerData{ std::to_string(value) };
        it = mSpinnerStates.find(widget.id);
    }
    auto& spinner = it->second;

    Panel(PanelStyle::Sunken, AdjustValue(color, 0.25f));

    if (widget.state == WidgetState::Unfocused)
    {
        int oldValue = value;
        value = std::clamp(std::stoi(spinner.text), min, max);
        valueChanged = oldValue != value;
    }

    if (mState.focused == widget.id)
    {
        if (widget.state == WidgetState::Active)
        {
            spinner.text = std::to_string(value);
        }

        LineEditor(widget, spinner.text, [&](const std::string& newValue) {
            std::regex intRegex("^-?\\d+$");
            return std::regex_match(newValue, intRegex);
        });
    }
    else
    {
        Text(std::to_string(value), Alignment::Center, olc::WHITE);
    }

    return valueChanged;
}

static bool RectHasPoint(const Rect& rect, int x, int y)
{
    return (x >= rect.xMin && x < rect.xMax && y >= rect.yMin && y < rect.yMax);
}

// HSV to RGB conversion
olc::Pixel GUI::HSVtoRGB(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0f - abs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h >= 0 && h < 60) {
        r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
        r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
        r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return olc::Pixel(
        static_cast<uint8_t>((r + m) * 255),
        static_cast<uint8_t>((g + m) * 255),
        static_cast<uint8_t>((b + m) * 255)
    );
}

// RGB to HSV conversion
void GUI::RGBtoHSV(const olc::Pixel& rgb, float& h, float& s, float& v)
{
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    
    float max = std::max({r, g, b});
    float min = std::min({r, g, b});
    float delta = max - min;
    
    v = max;
    s = (max == 0) ? 0 : delta / max;
    
    if (delta == 0) {
        h = 0;
    } else if (max == r) {
        h = 60 * fmod((g - b) / delta, 6.0f);
    } else if (max == g) {
        h = 60 * ((b - r) / delta + 2);
    } else {
        h = 60 * ((r - g) / delta + 4);
    }
    
    if (h < 0) h += 360;
}

bool GUI::ColorPicker(const std::string& id, olc::Pixel& color)
{
    auto& widget = GetWidget(id);
    auto rect = widget.rect;
    
    // Get dimensions from the widget's layout
    int width = rect.xMax - rect.xMin;
    int height = rect.yMax - rect.yMin;
    
    // Initialize color picker data if it doesn't exist
    auto it = mColorPickerStates.find(widget.id);
    if (it == mColorPickerStates.end()) {
        ColorPickerData data;
        RGBtoHSV(color, data.hue, data.saturation, data.brightness);
        mColorPickerStates[widget.id] = data;
        it = mColorPickerStates.find(widget.id);
    }
    auto& cpData = it->second;
    
    bool colorChanged = false;
    
    // Hue bar width
    int hueBarWidth = 12;
    const int spacing = 1;
    
    // Main color area (saturation/brightness)
    int colorAreaWidth = width - hueBarWidth - spacing;
    int colorAreaHeight = height;
    
    Rect colorArea = {
        rect.xMin + 1,
        rect.yMin + 1,
        rect.xMin + colorAreaWidth - 1,
        rect.yMin + colorAreaHeight - 1
    };
    
    // Hue bar area
    Rect hueArea = {
        rect.xMin + colorAreaWidth + spacing + 1,
        rect.yMin + 1,
        rect.xMin + width - 1,
        rect.yMin + colorAreaHeight - 1
    };

    colorAreaWidth = colorArea.xMax - colorArea.xMin;
    colorAreaHeight = colorArea.yMax - colorArea.yMin;
    hueBarWidth = hueArea.xMax - hueArea.xMin;

    // Panels for border
    PushLayout({ colorArea.xMin - 1, colorArea.yMin - 1, colorArea.xMax + 1, colorArea.yMax + 1 });
    Panel(PanelStyle::Sunken, olc::GREY, 0);
    PopLayout();

    // for hue as well
    PushLayout({ hueArea.xMin - 1, hueArea.yMin - 1, hueArea.xMax + 1, hueArea.yMax + 1 });
    Panel(PanelStyle::Sunken, olc::GREY, 0);
    PopLayout();

    // Draw the main color area (saturation/brightness)
    for (int y = 0; y < colorAreaHeight; y++) {
        for (int x = 0; x < colorAreaWidth; x++) {
            float saturation = static_cast<float>(x) / (colorAreaWidth - 1);
            float brightness = 1.0f - static_cast<float>(y) / (colorAreaHeight - 1);
            
            olc::Pixel pixelColor = HSVtoRGB(cpData.hue, saturation, brightness);
            mPGE->Draw(colorArea.xMin + x, colorArea.yMin + y, pixelColor);
        }
    }
    
    // Draw the hue bar
    for (int y = 0; y < colorAreaHeight; y++) {
        float hue = static_cast<float>(y) / (colorAreaHeight - 1) * 360.0f;
        olc::Pixel hueColor = HSVtoRGB(hue, 1.0f, 1.0f);
        
        mPGE->DrawLine(
            hueArea.xMin,
            hueArea.yMin + y,
            hueArea.xMax - 1,
            hueArea.yMin + y,
            hueColor
        );
    }
    
    // Handle mouse interaction
    if (widget.state == WidgetState::Active && mState.mouseButton == 0) {
        if (RectHasPoint(colorArea, mState.mouseX, mState.mouseY)) {
            // Update saturation and brightness
            float newSaturation = static_cast<float>(mState.mouseX - colorArea.xMin) / (colorAreaWidth - 1);
            float newBrightness = 1.0f - static_cast<float>(mState.mouseY - colorArea.yMin) / (colorAreaHeight - 1);
            
            newSaturation = std::clamp(newSaturation, 0.0f, 1.0f);
            newBrightness = std::clamp(newBrightness, 0.0f, 1.0f);
            
            if (cpData.saturation != newSaturation || cpData.brightness != newBrightness) {
                cpData.saturation = newSaturation;
                cpData.brightness = newBrightness;
                colorChanged = true;
            }
        }
        else if (RectHasPoint(hueArea, mState.mouseX, mState.mouseY)) {
            // Update hue
            float newHue = static_cast<float>(mState.mouseY - hueArea.yMin) / (colorAreaHeight - 1) * 360.0f;
            newHue = std::clamp(newHue, 0.0f, 360.0f);
            
            if (cpData.hue != newHue) {
                cpData.hue = newHue;
                colorChanged = true;
            }
        }
    }
    
    // Draw selection indicators
    // Saturation/Brightness indicator
    mPGE->SetClippingRect(colorArea.xMin, colorArea.yMin, colorArea.xMax - colorArea.xMin, colorArea.yMax - colorArea.yMin);

    int sbIndicatorX = colorArea.xMin + static_cast<int>(cpData.saturation * (colorAreaWidth - 1));
    int sbIndicatorY = colorArea.yMin + static_cast<int>((1.0f - cpData.brightness) * (colorAreaHeight - 1));
    mPGE->DrawCircle(sbIndicatorX, sbIndicatorY, 4, olc::WHITE);
    mPGE->DrawCircle(sbIndicatorX, sbIndicatorY, 3, olc::BLACK);

    mPGE->DisableClipping();
    
    // Hue indicator
    int hueIndicatorY = hueArea.yMin + static_cast<int>((cpData.hue / 360.0f) * (colorAreaHeight - 1));
    mPGE->FillRect(hueArea.xMin - 2, hueIndicatorY - 2, hueBarWidth + 4, 4, olc::WHITE);
    mPGE->FillRect(hueArea.xMin - 1, hueIndicatorY - 1, hueBarWidth + 2, 2, olc::BLACK);
    
    // Update the output color if changed
    if (colorChanged) {
        color = HSVtoRGB(cpData.hue, cpData.saturation, cpData.brightness);
    }
    
    PopLayout();
    return colorChanged;
}

Widget& GUI::GetWidget(const std::string& id)
{
    const auto widgetId = ID(id);
    auto it = mWidgets.find(widgetId);
    if (it == mWidgets.end())
    {
        mWidgets[widgetId] = Widget{};
        it = mWidgets.find(widgetId);
    }

    auto& widget = it->second;
    widget.id = widgetId;
    widget.rect = PopLayout();
    widget.state = WidgetState::Normal;

    mLayoutStack.push_back(widget.rect);

    if (RectHasPoint(widget.rect, mState.mouseX, mState.mouseY))
    {
        widget.mouseButton = mState.mouseButton;

        mState.hovered = widget.id;
        widget.state = WidgetState::Hovered;
        if (mState.active == gNullWidget && mState.mouseButton != -1)
        {
            mState.active = widget.id;
            widget.state = WidgetState::Active;
            if (mState.mouseButton == 0) {
                mState.focused = widget.id;
            }
        }
    }
    else
    {
        if (mState.focused == widget.id && mState.mouseButton == 0)
        {
            mState.lastFocused = widget.id;
            mState.focused = gNullWidget;
            widget.state = WidgetState::Unfocused;
        }
        widget.mouseButton = -1;
    }

    if (mState.active == widget.id && mState.hovered == widget.id)
    {
        widget.state = WidgetState::Active;
        if (mState.mouseButton == -1)
        {
            widget.state = WidgetState::Clicked;
        }
    }

    return widget;
}

bool GUI::WasClicked(const std::string &id)
{
    return mState.active == ID(id) && mState.mouseButton == -1;
}

void GUI::Begin()
{
    mPGE->SetPixelMode(olc::Pixel::ALPHA);
    mLayoutStack.push_back({ 0, 0, mPGE->ScreenWidth(), mPGE->ScreenHeight() });
    mState.mouseX = mPGE->GetMouseX();
    mState.mouseY = mPGE->GetMouseY();
    
    // Determine which mouse button is pressed
    if (mPGE->GetMouse(0).bHeld)
        mState.mouseButton = 0; // Left button
    else if (mPGE->GetMouse(2).bHeld)
        mState.mouseButton = 1; // Middle button
    else if (mPGE->GetMouse(1).bHeld)
        mState.mouseButton = 2; // Right button
    else
        mState.mouseButton = -1; // No button

    if (mState.mouseButton == -1)
        mState.hovered = gNullWidget;
}

void GUI::End()
{
    for (auto& func : mLateDrawFuncs)
    {
        func(mPGE);
    }
    mLateDrawFuncs.clear();

    if (mState.mouseButton == -1)
        mState.active = gNullWidget;

    mLayoutStack.clear();

    mBlinkTime += mPGE->GetElapsedTime();
    if (mBlinkTime >= 0.5f)
    {
        mBlink = !mBlink;
        mBlinkTime = 0.0f;
    }

    if (mState.focused == gNullWidget && mPGE->IsTextEntryEnabled())
    {
        mPGE->TextEntryEnable(false);
    }
}

wid GUI::ID(const std::string &name)
{
    std::hash<std::string> hasher;
    return static_cast<wid>(hasher(name)) + 1; // Avoid gNullWidget
}

void GUI::Init(olc::PixelGameEngine *pge)
{
    sInstance = std::make_unique<GUI>(pge);
}

Widget& GUI::TabToggleButton(const std::string &id, const std::string &text, bool value, const olc::Pixel &color)
{
    auto& widget = GetWidget(id);

    if (value)
    {
        switch (widget.state)
        {
            case WidgetState::Clicked:
            case WidgetState::Normal:
                Panel(PanelStyle::Sunken, AdjustValue(color, 0.8f), 2);
                break;
            case WidgetState::Hovered:
                Panel(PanelStyle::Sunken, AdjustValue(color, 1.0f), 2);
                break;
            case WidgetState::Active:
                Panel(PanelStyle::Sunken, AdjustValue(color, 0.6f), 2);
                break;
        }
    }
    else
    {
        switch (widget.state)
        {
            case WidgetState::Clicked:
            case WidgetState::Normal:
                Panel(PanelStyle::Raised, color, 2);
                break;
            case WidgetState::Hovered:
                Panel(PanelStyle::Raised, AdjustValue(color, 1.2f), 2);
                break;
            case WidgetState::Active:
                Panel(PanelStyle::Sunken, AdjustValue(color, 0.8f), 2);
                break;
        }
    }

    const auto textColor = Luma(color) > 0.45f ? olc::BLACK : olc::WHITE;
    Text(text, Alignment::Center, textColor);

    return widget;
}

void GUI::LineEditor(
    Widget& widget,
    std::string &value,
    const std::function<bool(const std::string &)> &validator
)
{
    bool focused = mState.focused == widget.id;

    auto rect = PeekLayout();

    // Handle input
    if (focused)
    {
        if (!mPGE->IsTextEntryEnabled())
        {
            mPGE->TextEntryEnable(true, value);
        }
        else
        {
            // If text entry is already enabled but this is a different widget,
            // we need to re-initialize it with this widget's value
            static wid sLastTextEntryWidget = gNullWidget;
            if (sLastTextEntryWidget != widget.id)
            {
                mPGE->TextEntryEnable(true, value);
                sLastTextEntryWidget = widget.id;
            }
        }

        auto newValue = mPGE->TextEntryGetString();
        if (validator(newValue))
            value = newValue;
    }

    auto sz = value.length() > 0
        ? mPGE->GetTextSizeProp(value.substr(0, mPGE->TextEntryGetCursor()))
        : olc::vi2d(0, 0);
    int cursorX = rect.xMin + sz.x;
    int textViewOffsetX = 0;

    // cursor is out of bounds?
    if (cursorX - textViewOffsetX > rect.xMax - 8 && focused)
    {
        textViewOffsetX = cursorX - rect.xMax + 8;
    }

    int centerY = widget.rect.yMin + (widget.rect.yMax - widget.rect.yMin) / 2;

    // Draw the text
    mPGE->SetClippingRect(
        rect.xMin,
        rect.yMin,
        rect.xMax - rect.xMin,
        rect.yMax - rect.yMin
    );
    mPGE->DrawStringProp(rect.xMin + 2 - textViewOffsetX, centerY - sz.y / 2, value, olc::WHITE);
    mPGE->DisableClipping();

    if (focused && mBlink)
    {
        mPGE->DrawStringProp(cursorX + 2 - textViewOffsetX, centerY - sz.y / 2, "_", olc::WHITE);
    }

    PopLayout();
}

Rect GUI::PopLayout()
{
    assert(!mLayoutStack.empty() && "Cannot pop layout stack when empty");
    Rect rect = mLayoutStack.back();
    mLayoutStack.pop_back();
    return rect;
}

const Rect &GUI::PeekLayout() const
{
    assert(!mLayoutStack.empty() && "Cannot peek layout stack when empty");
    return mLayoutStack.back();
}

void GUI::PushLayout(const Rect &rect)
{
    mLayoutStack.push_back(rect);
}

void GUI::PushLayout(int x, int y, int width, int height)
{
    mLayoutStack.push_back({ x, y, x + width, y + height });
}

olc::Pixel GUI::AdjustValue(const olc::Pixel &color, float value)
{
    float r = float(color.r) / 255.0f;
    float g = float(color.g) / 255.0f;
    float b = float(color.b) / 255.0f;
    float a = float(color.a) / 255.0f;

    r = std::clamp(r * value, 0.0f, 1.0f);
    g = std::clamp(g * value, 0.0f, 1.0f);
    b = std::clamp(b * value, 0.0f, 1.0f);

    return olc::PixelF(r, g, b, a);
}
