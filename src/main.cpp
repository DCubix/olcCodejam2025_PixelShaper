#define OLC_PGE_APPLICATION
#define _USE_MATH_DEFINES
#include <cmath>
#include "olcPixelGameEngine.h"

#include "gui.h"
#include "shaper.h"
#include <regex>

const olc::Pixel gizmoColor = olc::Pixel(80, 139, 237);
const olc::Pixel gizmoColorGrey = olc::Pixel(128, 128, 128);
const int HANDLE_SIZE = 2;

class ExampleApp : public olc::PixelGameEngine
{
public:
    ExampleApp()
    {
        sAppName = "olcPixelGameEngine - Cross Platform Example";
    }

public:
    bool OnUserCreate() override
    {
        GUI::Init(this);

        // Add multiple icons for testing
        gui.AddIcon("assets/union.png"); // 0
        gui.AddIcon("assets/subtract.png"); // 1
        gui.AddIcon("assets/ellipse.png"); // 2
        gui.AddIcon("assets/rectangle.png"); // 3
        gui.AddIcon("assets/zoom.png"); // 4
        gui.AddIcon("assets/x.png"); // 5
        gui.AddIcon("assets/up.png"); // 6
        gui.AddIcon("assets/down.png"); // 7
        gui.AddIcon("assets/add.png"); // 8

        RecreateDrawing();

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        // Clear screen
        Clear(gui.AdjustValue(controlColor, 0.25f));
        gui.Begin();
        
        BuildBottomStatusbar();
        BuildLeftToolbar();
        BuildRightSidebar();
        BuildDrawingArea();

        gui.End();
        return true;
    }

    bool OnUserDestroy() override
    {
        // Called once when the application is terminating
        return true;
    }

    void BuildLeftToolbar()
    {
        gui.CutLeft(20).Panel(PanelStyle::Flat, gui.AdjustValue(controlColor, 0.5f));

        // Circle
        if (gui.CutTop(20).Button("add_ellipse", "$[2]", controlColor))
        {
            activeLayer->AddElement(
                new EllipseElement(olc::vi2d(mDrawing->GetWidth() / 2, mDrawing->GetHeight() / 2), olc::vi2d(40, 20), 0.0f, olc::WHITE)
            );
            mDrawing->RenderAll();
        }

        // Rectangle
        if (gui.CutTop(20).Button("add_rectangle", "$[3]", controlColor))
        {
            activeLayer->AddElement(
                new RectangleElement(olc::vi2d(mDrawing->GetWidth() / 2, mDrawing->GetHeight() / 2), olc::vi2d(40, 20), 0.0f, olc::WHITE)
            );
            mDrawing->RenderAll();
        }

        gui.Spacer();
    }

    void LayersTab()
    {
        // Layer add
        if (gui.CutTop(18).Button("add_layer", "$[8] Add Layer", controlColor)) {
            activeLayer = mDrawing->AddLayer();
        }

        // Layers
        auto layers = mDrawing->GetLayers();
        int i = 0;
        for (const auto& layer : layers)
        {
            gui.CutTop(18);

            // Delete button
            if (layers.size() > 1)
            {
                if (gui.CutLeft(18).Button(
                    "delete_layer_" + std::to_string(layer->GetID()),
                    "$[5]",
                    controlColor
                ))
                {
                    mDrawing->RemoveLayer(layer->GetID());
                    if (activeLayer == layer) {
                        activeLayer = layers.front();
                    }
                    break;
                }
            }

            // move up/down
            if (i > 0)
            {
                if (gui.CutRight(18).Button(
                    "move_layer_up_" + std::to_string(layer->GetID()),
                    "$[6]",
                    controlColor
                ))
                {
                    mDrawing->MoveLayerUp(layer->GetID());
                }
            }

            if (i < layers.size() - 1)
            {
                if (gui.CutRight(18).Button(
                    "move_layer_down_" + std::to_string(layer->GetID()),
                    "$[7]",
                    controlColor
                ))
                {
                    mDrawing->MoveLayerDown(layer->GetID());
                }
            }

            if (gui.Button(
                "layer_" + std::to_string(layer->GetID()),
                layer->GetName(),
                activeLayer == layer ? olc::GREEN : controlColor
            ))
            {
                activeLayer = layer;
            }

            i++;
        }
    }

    void PropertiesTab()
    {
        if (!selectedElement)
        {
            gui.CutTop(36);
            gui.Text("No element selected", Alignment::Center, olc::BLACK);
            return;
        }

        // Position
        gui.CutTop(18).Text("Position", Alignment::Left, olc::BLACK);
        gui.CutTop(18);
        if (gui.CutLeft(0.5f).Spinner("pos_x", selectedElement->mPosition.x, -999, 999, 1, controlColor))
            mDrawing->RenderAll();
        if (gui.CutRight(1.0f).Spinner("pos_y", selectedElement->mPosition.y, -999, 999, 1, controlColor))
            mDrawing->RenderAll();
        gui.Spacer();

        gui.CutTop(3).Spacer();

        // Size
        gui.CutTop(18).Text("Size", Alignment::Left, olc::BLACK);
        gui.CutTop(18);
        if (gui.CutLeft(0.5f).Spinner("size_x", selectedElement->mSize.x, 1, 1000, 1, controlColor))
            mDrawing->RenderAll();
        if (gui.CutRight(1.0f).Spinner("size_y", selectedElement->mSize.y, 1, 1000, 1, controlColor))
            mDrawing->RenderAll();
        gui.Spacer();

        gui.CutTop(3).Spacer();

        // Rotation
        gui.CutTop(18).Text("Rotation", Alignment::Left, olc::BLACK);
        gui.CutTop(18);
        if (gui.Spinner("rotation", selectedElementRotation, -180, 180, 1, controlColor))
        {
            selectedElement->mRotation = static_cast<float>(selectedElementRotation) * M_PI / 180.0f;
            mDrawing->RenderAll();
        }

        // Color
        gui.CutTop(18).Text("Color", Alignment::Left, olc::BLACK);
        
        // Color picker widget
        gui.CutTop(100);
        if (gui.ColorPicker("element_color", selectedElement->mColor))
        {
            mDrawing->RenderAll();
            UpdateHTMLColor();
        }

        gui.CutTop(3).Spacer();

        // HTML color code
        gui.CutTop(16);
        if (gui.EditBox("html_color", selectedElementHtmlColor, [](const std::string& newValue) {
            // Simple hex color validation
            return std::regex_match(newValue, std::regex("^#[0-9A-Fa-f]{1,8}$"));
        }, controlColor))
        {
            if (std::sscanf(selectedElementHtmlColor.c_str(), "#%02hhx%02hhx%02hhx%02hhx",
                &selectedElement->mColor.r, &selectedElement->mColor.g,
                &selectedElement->mColor.b, &selectedElement->mColor.a) == 4)
            {
                mDrawing->RenderAll();
            }
        }

        // Subtractive setting
        gui.CutTop(3).Spacer();

        gui.CutTop(18);
        if (gui.CheckBox("subtractive", "Is Subtractive", selectedElement->mSubtractive, olc::WHITE, olc::BLACK))
        {
            mDrawing->RenderAll();
        }
    }

    void FXTab()
    {
        if (!activeLayer)
        {
            gui.CutTop(36);
            gui.Text("No layer selected", Alignment::Center, olc::BLACK);
            return;
        }

        gui.CutTop(18).Text("Merge Smoothness", Alignment::Left, olc::BLACK);
        gui.CutTop(18);

        int smoothness = static_cast<int>(activeLayer->GetMergeSmoothness() * 5.0f);
        if (gui.HSlider("fx_merge_smoothness", smoothness, 0, 100, controlColor))
        {
            activeLayer->SetMergeSmoothness(smoothness / 5.0f);
            mDrawing->RenderAll();
        }

        gui.CutTop(3).Spacer();

        gui.CutTop(18);
        if (gui.CheckBox("fx_contour_enabled", "Contour", activeLayer->GetContourEffect()->mEnabled, olc::WHITE, olc::BLACK))
        {
            mDrawing->RenderAll();
        }

        if (activeLayer->GetContourEffect()->mEnabled)
        {
            gui.CutTop(18).Text("Contour Color", Alignment::Left, olc::BLACK);
            gui.CutTop(100);
            if (gui.ColorPicker("fx_contour_color", activeLayer->GetContourEffect()->mColor))
            {
                mDrawing->RenderAll();
            }
        }

        gui.CutTop(3).Spacer();

        gui.CutTop(18);
        if (gui.CheckBox("fx_shading_enabled", "Shading", activeLayer->GetShadingEffect()->mEnabled, olc::WHITE, olc::BLACK))
        {
            mDrawing->RenderAll();
        }

        if (activeLayer->GetShadingEffect()->mEnabled)
        {
            gui.CutTop(18).Text("Light Position", Alignment::Left, olc::BLACK);
            gui.CutTop(18);
            if (gui.CutLeft(0.5f).Spinner("fx_light_x", activeLayer->GetShadingEffect()->mLightPosition.x, 0, mDrawing->GetWidth(), 1, controlColor))
                mDrawing->RenderAll();
            if (gui.CutRight(1.0f).Spinner("fx_light_y", activeLayer->GetShadingEffect()->mLightPosition.y, 0, mDrawing->GetHeight(), 1, controlColor))
                mDrawing->RenderAll();
            gui.Spacer();

            gui.CutTop(3).Spacer();

            gui.CutTop(18).Text("Intensity", Alignment::Left, olc::BLACK);
            gui.CutTop(18);
            int intensity = static_cast<int>(activeLayer->GetShadingEffect()->mIntensity * 10.0f);
            if (gui.HSlider("fx_intensity", intensity, 0, 10, controlColor))
            {
                activeLayer->GetShadingEffect()->mIntensity = intensity / 10.0f;
                mDrawing->RenderAll();
            }

            gui.CutTop(3).Spacer();

            gui.CutTop(18).Text("Shadow Color", Alignment::Left, olc::BLACK);
            gui.CutTop(100);
            if (gui.ColorPicker("fx_shadow_color", activeLayer->GetShadingEffect()->mColor))
            {
                mDrawing->RenderAll();
            }
        }
    }

    void BuildRightSidebar()
    {
        const std::vector<std::string> tabs = { "Layers", "Properties", "FX" };

        // layers
        gui.CutRight(160).Panel(PanelStyle::Flat, gui.AdjustValue(controlColor, 0.5f), 2);

        // Tabs
        gui.CutTop(18);
        {
            gui.TabBar(tabs, activeTab, controlColor);
        }
        gui.Spacer();

        gui.Panel(PanelStyle::Raised, controlColor, 2);
        if (activeTab == 0) // Layers tab
        {
            LayersTab();
        }
        else if (activeTab == 1) // Properties tab
        {
            PropertiesTab();
        }
        else if (activeTab == 2) // FX tab
        {
            FXTab();
        }

        gui.Spacer();
    }

    void BuildBottomStatusbar()
    {
        gui.CutBottom(16).Panel(PanelStyle::Raised, controlColor, 2);

        gui.CutLeft(0.25f);
        gui.Text("Pixel Shaper", Alignment::Left, gui.AdjustValue(controlColor, 0.15f));

        gui.CutRight(64);
        gui.HSlider("zoom", zoom, 1, 4);

        gui.CutRight(32);
        gui.Text("$[4] Zoom ", Alignment::Right, gui.AdjustValue(controlColor, 0.15f));

        gui.Spacer();
    }

    void BuildDrawingArea()
    {
        auto& widget = gui.GetWidget("drawing_area");

        Rect drawingArea = widget.rect;
        int drawingAreaW = drawingArea.xMax - drawingArea.xMin;
        int drawingAreaH = drawingArea.yMax - drawingArea.yMin;

        int centerX = drawingArea.xMin + (drawingAreaW / 2);
        int centerY = drawingArea.yMin + (drawingAreaH / 2);

        // mDrawing should be drawn at the center with a frame around it, taking pan and zoom into account
        int drawingX = centerX - (drawingWidth * zoom) / 2 + pan.x;
        int drawingY = centerY - (drawingHeight * zoom) / 2 + pan.y;

        // mouse coords with pan and zoom
        int mouseX = GetMouseX() - drawingArea.xMin;
        int mouseY = GetMouseY() - drawingArea.yMin;

        SetClippingRect(drawingArea.xMin, drawingArea.yMin, drawingAreaW, drawingAreaH);
        // Draw the mDrawing sprite at the calculated position
        if (mDrawing)
        {
            for (const auto& layer : mDrawing->GetLayers())
            {
                DrawSprite(drawingX, drawingY, layer->GetSurface(), zoom);
            }
        }

        DrawRect(
            drawingX - 1, drawingY - 1,
            (drawingWidth * zoom) + 1, (drawingHeight * zoom) + 1,
            gui.AdjustValue(controlColor, 0.1f)
        );

        // Draw gizmos for selected elements and handle their interactions
        bool gizmoInteraction = false;
        for (const auto& el : activeLayer->GetElements())
        {
            bool gizmoHit = EditElement(el, el == selectedElement);
            if (gizmoHit) gizmoInteraction = true;
        }

        // Handle element selection only if not interacting with gizmos and drawing area is active
        if (GetMouse(0).bPressed && !gizmoInteraction && widget.state != WidgetState::Normal)
        {
            Element* clickedElement = nullptr;
            
            // Transform mouse coordinates to drawing coordinates
            olc::vi2d mouseDrawingPos = olc::vi2d(
                (mouseX - (drawingX - drawingArea.xMin)) / zoom,
                (mouseY - (drawingY - drawingArea.yMin)) / zoom
            );
            
            // Check all elements for clicks (reverse order to prioritize top elements)
            auto elements = activeLayer->GetElements();
            for (auto it = elements.rbegin(); it != elements.rend(); ++it)
            {
                Element* el = *it;
                
                if (el->IsPointInside(mouseDrawingPos))
                {
                    clickedElement = el;
                    break; // Found the topmost element
                }
            }
            
            // Update selection
            selectedElement = clickedElement;
            if (selectedElement) {
                selectedElementRotation = static_cast<int>(selectedElement->mRotation / M_PI * 180.0f);
                UpdateHTMLColor();
            }
        }

        DisableClipping();

        if (GetMouse(1).bPressed && widget.state != WidgetState::Normal)
        {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            dragging = true;
        }
        else if (GetMouse(1).bReleased)
        {
            dragging = false;
        }

        if (dragging)
        {
            pan.x += (mouseX - lastMouseX);
            pan.y += (mouseY - lastMouseY);
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }
    }

    void RecreateDrawing()
    {
        mDrawing.reset(new Shaper(drawingWidth, drawingHeight));
        activeLayer = mDrawing->AddLayer();
        mDrawing->RenderAll();
    }

    bool EditElement(Element* shape, bool isSelected)
    {
        // Get the drawing area widget to calculate proper offsets
        auto& widget = gui.GetWidget("drawing_area");
        Rect drawingArea = widget.rect;
        int drawingAreaW = drawingArea.xMax - drawingArea.xMin;
        int drawingAreaH = drawingArea.yMax - drawingArea.yMin;
        int centerX = drawingArea.xMin + (drawingAreaW / 2);
        int centerY = drawingArea.yMin + (drawingAreaH / 2);
        
        // Calculate drawing offset
        int drawingX = centerX - (drawingWidth * zoom) / 2 + pan.x;
        int drawingY = centerY - (drawingHeight * zoom) / 2 + pan.y;
        
        // mouse coords with pan and zoom
        int mouseX = GetMouseX() - drawingArea.xMin;
        int mouseY = GetMouseY() - drawingArea.yMin;
        
        // Transform shape position to screen coordinates
        olc::vi2d shapeScreenPos = olc::vi2d(
            drawingX + shape->GetPosition().x * zoom,
            drawingY + shape->GetPosition().y * zoom
        );
        
        // Get rectangle size in screen coordinates
        olc::vi2d rectSize = shape->GetSize() * zoom;
        float rotation = shape->GetRotation();
        
        // Calculate corner positions (relative to center, before rotation)
        olc::vi2d corners[4] = {
            olc::vi2d(-rectSize.x / 2, -rectSize.y / 2), // Top-left
            olc::vi2d(rectSize.x / 2, -rectSize.y / 2),  // Top-right
            olc::vi2d(rectSize.x / 2, rectSize.y / 2),   // Bottom-right
            olc::vi2d(-rectSize.x / 2, rectSize.y / 2)   // Bottom-left
        };
        
        // Rotate corners and transform to screen coordinates
        olc::vi2d screenCorners[4];
        for (int i = 0; i < 4; i++)
        {
            float x = corners[i].x * ::cos(rotation) - corners[i].y * ::sin(rotation);
            float y = corners[i].x * ::sin(rotation) + corners[i].y * ::cos(rotation);
            screenCorners[i] = shapeScreenPos + olc::vi2d(x, y);
        }
        
        // Calculate rotation handle position (at top edge center)
        olc::vi2d rotationHandleOffset = olc::vi2d(0, -rectSize.y / 2 - 20); // 20 pixels above rectangle
        float rotX = rotationHandleOffset.x * ::cos(rotation) - rotationHandleOffset.y * ::sin(rotation);
        float rotY = rotationHandleOffset.x * ::sin(rotation) + rotationHandleOffset.y * ::cos(rotation);
        olc::vi2d rotationHandlePos = shapeScreenPos + olc::vi2d(rotX, rotY);
        
        // Transform mouse coordinates to match screen coordinates
        olc::vi2d mouseScreenPos = olc::vi2d(
            drawingArea.xMin + mouseX,
            drawingArea.yMin + mouseY
        );

        // Always draw rectangle outline
        for (int i = 0; i < 4; i++)
        {
            int next = (i + 1) % 4;
            DrawLine(screenCorners[i] + olc::vi2d{1, 1}, screenCorners[next] + olc::vi2d{1, 1}, olc::BLACK);
            DrawLine(screenCorners[i], screenCorners[next], isSelected ? gizmoColor : gizmoColorGrey);
        }
        
        // Only draw handles and rotation gizmo if selected
        if (isSelected)
        {
            // Draw corner handles (shadow)
            for (int i = 0; i < 4; i++)
            {
                FillCircle(screenCorners[i] + olc::vi2d{1, 1}, HANDLE_SIZE, olc::BLACK);
            }
            
            // Draw center handle (shadow)
            FillCircle(shapeScreenPos + olc::vi2d{1, 1}, HANDLE_SIZE - 1, olc::BLACK);
            
            // Draw rotation handle (shadow)
            FillCircle(rotationHandlePos + olc::vi2d{1, 1}, HANDLE_SIZE, olc::BLACK);
            DrawLine(shapeScreenPos + olc::vi2d{1, 1}, rotationHandlePos + olc::vi2d{1, 1}, olc::BLACK);

            // Draw corner handles
            for (int i = 0; i < 4; i++)
            {
                FillCircle(screenCorners[i], HANDLE_SIZE, gizmoColor);
            }
            
            // Draw center handle
            FillCircle(shapeScreenPos, HANDLE_SIZE - 1, gizmoColor);
            
            // Draw rotation handle
            FillCircle(rotationHandlePos, HANDLE_SIZE, gizmoColor);
            DrawLine(shapeScreenPos, rotationHandlePos, gizmoColor);
        }

        // Mouse events
        if (!isSelected) return false;

        bool gizmoHit = false;
        
        if (GetMouse(0).bPressed)
        {
            // Check rotation handle first (highest priority)
            if (PointInCircle(mouseScreenPos, rotationHandlePos, HANDLE_SIZE))
            {
                manipulationMode = ManipulationMode::Rotate;
                gizmoHit = true;
            }
            // Check corner handles next
            else 
            {
                for (int i = 0; i < 4; i++)
                {
                    if (PointInCircle(mouseScreenPos, screenCorners[i], HANDLE_SIZE))
                    {
                        manipulationMode = ManipulationMode::Resize;
                        resizeCorner = i;
                        gizmoHit = true;
                        break;
                    }
                }
                
                // Check center handle
                if (!gizmoHit && PointInCircle(mouseScreenPos, shapeScreenPos, HANDLE_SIZE - 1))
                {
                    manipulationMode = ManipulationMode::Move;
                    // Calculate offset from mouse to shape center in drawing coordinates
                    olc::vi2d mouseDrawingPos = olc::vi2d(
                        (mouseScreenPos.x - drawingX) / zoom,
                        (mouseScreenPos.y - drawingY) / zoom
                    );
                    dragOffset = shape->GetPosition() - mouseDrawingPos;
                    gizmoHit = true;
                }
                
                // If no specific gizmo was hit, but we clicked on the shape itself, start moving
                if (!gizmoHit)
                {
                    // Transform mouse to drawing coordinates to check if it's inside the shape
                    olc::vi2d mouseDrawingPos = olc::vi2d(
                        (mouseScreenPos.x - drawingX) / zoom,
                        (mouseScreenPos.y - drawingY) / zoom
                    );
                    
                    if (shape->IsPointInside(mouseDrawingPos))
                    {
                        manipulationMode = ManipulationMode::Move;
                        // Calculate offset from mouse to shape center in drawing coordinates
                        dragOffset = shape->GetPosition() - mouseDrawingPos;
                        gizmoHit = true;
                    }
                }
            }
        }
        else if (GetMouse(0).bReleased)
        {
            manipulationMode = ManipulationMode::None;
        }

        if (manipulationMode == ManipulationMode::Move)
        {
            // Calculate new position in drawing coordinates with drag offset
            olc::vi2d mouseDrawingPos = olc::vi2d(
                (mouseScreenPos.x - drawingX) / zoom,
                (mouseScreenPos.y - drawingY) / zoom
            );
            shape->SetPosition(mouseDrawingPos + dragOffset);
            mDrawing->RenderAll();
        }
        else if (manipulationMode == ManipulationMode::Resize)
        {
            // Calculate new size based on mouse position
            olc::vi2d mouseDrawingPos = olc::vi2d(
                (mouseScreenPos.x - drawingX) / zoom,
                (mouseScreenPos.y - drawingY) / zoom
            );
            
            // Calculate relative position from shape center
            olc::vi2d relativePos = mouseDrawingPos - shape->GetPosition();
            
            // Rotate relative position by negative rotation to get local coordinates
            float cosAngle = ::cos(-rotation);
            float sinAngle = ::sin(-rotation);
            float localX = relativePos.x * cosAngle - relativePos.y * sinAngle;
            float localY = relativePos.x * sinAngle + relativePos.y * cosAngle;
            
            // Calculate new size based on which corner is being dragged
            olc::vi2d newSize = shape->GetSize();
            if (resizeCorner == 0 || resizeCorner == 3) // Left side
                newSize.x = std::max(1, (int)(-localX * 2));
            else // Right side
                newSize.x = std::max(1, (int)(localX * 2));
                
            if (resizeCorner == 0 || resizeCorner == 1) // Top side
                newSize.y = std::max(1, (int)(-localY * 2));
            else // Bottom side
                newSize.y = std::max(1, (int)(localY * 2));
            
            shape->SetSize(newSize);
            mDrawing->RenderAll();
        }
        else if (manipulationMode == ManipulationMode::Rotate)
        {
            // Calculate angle from center to mouse
            olc::vi2d mouseDrawingPos = olc::vi2d(
                (mouseScreenPos.x - drawingX) / zoom,
                (mouseScreenPos.y - drawingY) / zoom
            );
            olc::vi2d directionVector = mouseDrawingPos - shape->GetPosition();
            float newRotation = ::atan2(directionVector.y, directionVector.x) + 3.14159f / 2; // Add 90 degrees so handle points up initially

            selectedElementRotation = static_cast<int>(newRotation / M_PI * 180.0f);

            shape->SetRotation(newRotation);
            mDrawing->RenderAll();
        }
        
        return gizmoHit;
    }

    bool PointInCircle(const olc::vi2d& point, const olc::vi2d& center, int radius)
    {
        return (point - center).mag2() < radius * radius;
    }

    void UpdateHTMLColor()
    {
        if (selectedElement)
        {
            selectedElementHtmlColor = 
                "#" + std::format("{:02X}", selectedElement->mColor.r) +
                std::format("{:02X}", selectedElement->mColor.g) +
                std::format("{:02X}", selectedElement->mColor.b) +
                std::format("{:02X}", selectedElement->mColor.a);
        }
    }

    int zoom{ 1 };
    olc::vi2d pan{ 0, 0 };
    int drawingWidth{ 200 }, drawingHeight{ 200 };
    int lastMouseX{ 0 }, lastMouseY{ 0 };
    bool dragging{ false };
    int activeTab{ 0 };

    enum class ManipulationMode {
        None,
        Move,
        Resize,
        Rotate
    } manipulationMode;

    Element* selectedElement{ nullptr };
    int selectedElementRotation{ 0 };
    std::string selectedElementHtmlColor{ "#00000000" };

    int resizeCorner{ 0 };
    olc::vi2d dragOffset{ 0, 0 }; // Offset from mouse to shape center when drag starts

    std::unique_ptr<Shaper> mDrawing;
    Layer* activeLayer;

    olc::Pixel controlColor = olc::Pixel(212, 208, 200);
};

int main()
{
    ExampleApp demo;
    
    // Initialize the engine with screen dimensions and pixel size
    if (demo.Construct(640, 360, 2, 2))
    {
        demo.Start();
    }
    
    return 0;
}
