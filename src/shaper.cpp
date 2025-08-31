#include "shaper.h"

#include <algorithm>
#include <cmath>

#include "stb_image_write.h"

size_t Layer::mNextID = 1;
size_t Element::mNextID = 1;

float EllipseElement::GetSDF(olc::vf2d p) const
{
    // p is in normalized coordinates where the ellipse should be a unit circle
    // Simple circle SDF - distance from origin minus radius of 1
    return p.mag() - 1.0f;
}

bool EllipseElement::IsPointInside(const olc::vi2d &point) const
{
    olc::vi2d center = GetPosition();
    olc::vi2d size = GetSize();
    float rotation = GetRotation();
    
    // Translate point to ellipse's local coordinate system
    olc::vi2d localPoint = point - center;
    
    // Rotate point by negative rotation to align with ellipse's axes
    float cosAngle = ::cos(-rotation);
    float sinAngle = ::sin(-rotation);
    
    float rotatedX = localPoint.x * cosAngle - localPoint.y * sinAngle;
    float rotatedY = localPoint.x * sinAngle + localPoint.y * cosAngle;
    
    // Check if rotated point is inside the axis-aligned ellipse
    float normX = (rotatedX * rotatedX) / ((size.x / 2) * (size.x / 2));
    float normY = (rotatedY * rotatedY) / ((size.y / 2) * (size.y / 2));
    
    return (normX + normY) <= 1.0f;
}

void EllipseElement::Serialize(json &out) const
{
    out["type"] = "ellipse";
    Element::Serialize(out);
}

void Element::Serialize(json &out) const
{
    out["id"] = mID;
    out["position"] = { mPosition.x, mPosition.y };
    out["size"] = { mSize.x, mSize.y };
    out["rotation"] = mRotation;
    out["color"] = { mColor.r, mColor.g, mColor.b, mColor.a };
    out["join_op"] = static_cast<int>(mJoinOp);
}

void Element::Deserialize(const json &in)
{
    if (in.contains("id")) {
        mID = in["id"];
        mNextID = std::max(mNextID, mID + 1);
    } else {
        mID = mNextID++;
    }
    if (in.contains("position")) {
        mPosition = { in["position"][0], in["position"][1] };
    }
    if (in.contains("size")) {
        mSize = { in["size"][0], in["size"][1] };
    }
    if (in.contains("rotation")) {
        mRotation = in["rotation"];
    }
    if (in.contains("color")) {
        mColor = { in["color"][0], in["color"][1], in["color"][2], in["color"][3] };
    }
    if (in.contains("subtractive")) {
        mJoinOp = in["subtractive"].get<bool>() ? JoinOperation::Subtraction : JoinOperation::Union;
    }
    if (in.contains("join_op")) {
        mJoinOp = static_cast<JoinOperation>(in["join_op"].get<int>());
    }
}

float RectangleElement::GetSDF(olc::vf2d p) const
{
    // p is in normalized coordinates where the rectangle should be a unit square
    // Simple box SDF - unit square centered at origin with extents [-1, 1]
    olc::vf2d ap{ ::abs(p.x), ::abs(p.y) };
    olc::vf2d d = ap - olc::vf2d(1.0f, 1.0f);
    return d.max({ 0.0f, 0.0f }).mag() + std::min(std::max(d.x, d.y), 0.0f);
}

bool RectangleElement::IsPointInside(const olc::vi2d &point) const
{
    olc::vi2d center = GetPosition();
    olc::vi2d size = GetSize();
    float rotation = GetRotation();
    
    // Translate point to rectangle's local coordinate system
    olc::vi2d localPoint = point - center;
    
    // Rotate point by negative rotation to align with rectangle's axes
    float cosAngle = ::cos(-rotation);
    float sinAngle = ::sin(-rotation);
    
    float rotatedX = localPoint.x * cosAngle - localPoint.y * sinAngle;
    float rotatedY = localPoint.x * sinAngle + localPoint.y * cosAngle;
    
    // Check if rotated point is inside the axis-aligned rectangle
    return (rotatedX >= -size.x / 2 && rotatedX <= size.x / 2 &&
            rotatedY >= -size.y / 2 && rotatedY <= size.y / 2);
}

void RectangleElement::Serialize(json &out) const
{
    out["type"] = "rectangle";
    Element::Serialize(out);
}

Element* Layer::AddElement(Element *element)
{
    mElements.push_back(std::unique_ptr<Element>(element));
    return mElements.back().get();
}

void Layer::RemoveElement(Element *element)
{
    auto it = std::remove_if(mElements.begin(), mElements.end(),
        [element](const std::unique_ptr<Element>& e) { return e.get() == element; });
    mElements.erase(it, mElements.end());
}

Element *Layer::GetElement(size_t id) const
{
    for (const auto& elem : mElements)
    {
        if (elem->GetID() == id)
            return elem.get();
    }
    return nullptr;
}

void Layer::Resize(int width, int height)
{
    mSurface.reset(new olc::Sprite(width, height));
    mNormals.reset(new olc::Sprite(width, height));
    Clear();
}

void Layer::Clear()
{
    if (!mSurface) return;

    for (int y = 0; y < mSurface->height; y++)
    {
        for (int x = 0; x < mSurface->width; x++)
        {
            mSurface->SetPixel(x, y, olc::Pixel(0, 0, 0, 0));
            mNormals->SetPixel(x, y, olc::Pixel(128, 128, 255, 255));
        }
    }
}

template <typename T>
T mod(T x, T m)
{
    T r = std::fmod(x, m);
    return (r < 0) ? r + m : r;
}

void Layer::Render()
{
    if (!mSurface) return;

    auto fnStep = [](float a, float b)
    {
        return (a < b) ? 1.0f : 0.0f;
    };

    auto fnUnion = [](float a, float b, float k)
    {
        // Circular smooth union
        k *= 1.0f / (1.0f - std::sqrt(0.5f));
        float h = std::max(k - std::abs(a - b), 0.0f) / k;
        return std::min(a, b) - k * 0.5f * (1.0f + h - std::sqrt(1.0f - h * (h - 2.0f)));
    };

    auto fnIntersection = [](float d1, float d2)
    {
        return std::max(d1, d2);
    };

    auto fnSubtract = [=](float d1, float d2)
    {
        // Subtraction is intersection with negated second operand
        return fnIntersection(d1, -d2);
    };

    std::vector<float> sdfMap;
    sdfMap.resize(mSurface->width * mSurface->height);

    auto fnPixelsToNormalized = [&](int x, int y, Element* el) {
        olc::vf2d worldPos{ float(x), float(y) };
        olc::vf2d localPos = worldPos - el->GetPosition();

        // Rotation
        float cosAngle = ::cos(-el->GetRotation());
        float sinAngle = ::sin(-el->GetRotation());
        olc::vf2d rotatedPos{
            localPos.x * cosAngle - localPos.y * sinAngle,
            localPos.x * sinAngle + localPos.y * cosAngle
        };

        // Scaling
        olc::vf2d scale{ el->GetSize().x / 2.0f, el->GetSize().y / 2.0f };
        if (scale.x > 0.0f && scale.y > 0.0f) {
            rotatedPos.x /= scale.x;
            rotatedPos.y /= scale.y;
        }

        // Modifier test: Repetition
        // rotatedPos.x = mod(rotatedPos.x + 5.0f, 10.0f) - 5.0f;
        // rotatedPos.y = mod(rotatedPos.y + 5.0f, 10.0f) - 5.0f;

        return rotatedPos;
    };

    for (int y = 0; y < mSurface->height; y++)
    {
        for (int x = 0; x < mSurface->width; x++)
        {
            // First pass: Find the closest non-subtractive element for color
            float closestDistance = 1e30f;
            olc::Pixel pixelColor{ 0, 0, 0, 0 };
            
            for (const auto& el : mElements)
            {
                if (el->IsSubtractive()) continue; // Skip subtractive elements for color
                
                olc::vf2d rotatedPos = fnPixelsToNormalized(x, y, el.get());
                float sdf = el->GetSDF(rotatedPos);

                if (sdf < closestDistance)
                {
                    closestDistance = sdf;
                    pixelColor = el->GetColor();
                }
            }

            // Second pass: Calculate the final SDF for the merged shape
            float sdfAccum = 1e30f;
            bool firstElement = true;
            
            for (const auto& el : mElements)
            {
                olc::vf2d rotatedPos = fnPixelsToNormalized(x, y, el.get());
                float sdf = el->GetSDF(rotatedPos);

                switch (el->GetJoinOperation())
                {
                    case JoinOperation::Union:
                        if (firstElement) {
                            sdfAccum = sdf;
                            firstElement = false;
                        } else {
                            sdfAccum = fnUnion(sdfAccum, sdf, mMergeSmoothness + 0.001f);
                        }
                        break;
                    case JoinOperation::Intersection:
                        if (firstElement) {
                            sdfAccum = sdf;
                            firstElement = false;
                        } else {
                            sdfAccum = fnIntersection(sdfAccum, sdf);
                        }
                        break;
                    case JoinOperation::Subtraction:
                        sdfAccum = fnSubtract(sdfAccum, sdf);
                        break;
                }

            }

            float inside = 1.0f - fnStep(sdfAccum, 0.0f);
            if (inside < 1.0f)
            {
                mSurface->SetPixel(x, y, pixelColor);
            }
            sdfMap[y * mSurface->width + x] = sdfAccum;
        }
    }

    // Compute normals
    auto fnSampleSDF = [&](int x, int y)
    {
        if (x < 0 || x >= mSurface->width || y < 0 || y >= mSurface->height)
            return 1e30f;
        return sdfMap[y * mSurface->width + x];
    };

    const float e = 10.0f / mSurface->width;
    for (int y = 0; y < mSurface->height; y++)
    {
        for (int x = 0; x < mSurface->width; x++)
        {
            float dx = fnSampleSDF(x + 1, y) - fnSampleSDF(x - 1, y);
            float dy = fnSampleSDF(x, y + 1) - fnSampleSDF(x, y - 1);
            vec3 n = vec3{ -dx, -dy, 2.0f * e }.norm();
            mNormals->SetPixel(x, y, olc::PixelF(
                n.x * 0.5f + 0.5f,
                n.y * 0.5f + 0.5f,
                n.z * 0.5f + 0.5f
            ));
        }
    }

    if (mShadingEffect->mEnabled)
    {
        mShadingEffect->Apply(this);
    }

    if (mContourEffect->mEnabled)
    {
        mContourEffect->Apply(this);
    }
}

void Layer::Serialize(json &out) const
{
    out["id"] = mID;
    out["name"] = mName;
    out["merge_smoothness"] = mMergeSmoothness;

    // Serialize elements
    for (const auto &element : mElements)
    {
        json elementData;
        element->Serialize(elementData);
        out["elements"].push_back(elementData);
    }

    // Serialize effects
    json effectsData;
    mShadingEffect->Serialize(effectsData);
    out["effects"]["shading"] = effectsData;

    mContourEffect->Serialize(effectsData);
    out["effects"]["contour"] = effectsData;
}

void Layer::Deserialize(const json &in)
{
    if (in.contains("id")) {
        mID = in["id"];
        Layer::mNextID = std::max(Layer::mNextID, mID + 1);
    }
    if (in.contains("name")) {
        mName = in["name"];
    }
    if (in.contains("merge_smoothness")) {
        mMergeSmoothness = in["merge_smoothness"];
    }

    // Deserialize elements
    mElements.clear();
    if (in.contains("elements")) {
        for (const auto &elementData : in["elements"])
        {
            if (elementData.contains("type")) {
                std::string type = elementData["type"];
                Element* element = nullptr;
                if (type == "ellipse")
                {
                    element = new EllipseElement();
                }
                else if (type == "rectangle")
                {
                    element = new RectangleElement();
                }
                else if (type == "triangle")
                {
                    element = new TriangleElement();
                }
                // Add other element types here as needed

                if (element)
                {
                    element->Deserialize(elementData);
                    AddElement(element);
                }
            }
        }
    }

    // Deserialize effects
    if (in.contains("effects"))
    {
        if (in["effects"].contains("shading"))
        {
            mShadingEffect->Deserialize(in["effects"]["shading"]);
        }
        if (in["effects"].contains("contour"))
        {
            mContourEffect->Deserialize(in["effects"]["contour"]);
        }
    }
}

std::vector<Element*> Layer::GetElements() const
{
    std::vector<Element*> elements;
    for (const auto &e : mElements)
        elements.push_back(e.get());
    return elements;
}

Layer* Shaper::AddLayer()
{
    mLayers.push_back(std::make_unique<Layer>(mWidth, mHeight));
    mLayerOrder.push_back(mLayers.back()->GetID());
    return mLayers.back().get();
}

void Shaper::RemoveLayer(size_t id)
{
    auto layerIt = std::find_if(mLayers.begin(), mLayers.end(),
        [=](const std::unique_ptr<Layer>& layer) { return layer->GetID() == id; });
    if (layerIt != mLayers.end())
    {
        mLayers.erase(layerIt);
    }

    auto orderIt = std::find(mLayerOrder.begin(), mLayerOrder.end(), id);
    if (orderIt != mLayerOrder.end())
    {
        mLayerOrder.erase(orderIt);
    }
}

Layer* Shaper::MoveLayerUp(size_t id)
{
    auto it = std::find_if(mLayerOrder.begin(), mLayerOrder.end(),
        [=](size_t index) { return index == id; });

    if (it != mLayerOrder.end() && it != mLayerOrder.begin())
    {
        std::iter_swap(it, std::prev(it));
    }

    return GetLayer(id);
}

Layer* Shaper::MoveLayerDown(size_t id)
{
    auto it = std::find_if(mLayerOrder.begin(), mLayerOrder.end(),
        [=](size_t index) { return index == id; });

    if (it != mLayerOrder.end() && std::next(it) != mLayerOrder.end())
    {
        std::iter_swap(it, std::next(it));
    }

    return GetLayer(id);
}

void Shaper::ReorderLayer(size_t id, size_t newIndex)
{
    auto it = std::find(mLayerOrder.begin(), mLayerOrder.end(), id);
    if (it != mLayerOrder.end() && newIndex < mLayerOrder.size())
    {
        mLayerOrder.erase(it);
        mLayerOrder.insert(mLayerOrder.begin() + newIndex, id);
    }
}

void Shaper::RenderAll()
{
    for (const auto &layer : mLayers)
    {
        layer->Clear();
        layer->Render();
    }
}

void Shaper::Resize(int width, int height)
{
    for (const auto &layer : mLayers)
    {
        layer->Resize(width, height);
    }
}

void Shaper::Serialize(json &out) const
{
    out["width"] = mWidth;
    out["height"] = mHeight;

    for (const auto &layer : mLayers)
    {
        json layerData;
        layer->Serialize(layerData);
        out["layers"].push_back(layerData);
    }

    out["layer_order"] = mLayerOrder;
}

void Shaper::Deserialize(const json &in)
{
    if (in.contains("width")) {
        mWidth = in["width"];
    }
    if (in.contains("height")) {
        mHeight = in["height"];
    }
    Resize(mWidth, mHeight);

    mLayers.clear();
    if (in.contains("layers"))
    {
        for (const auto &layerData : in["layers"])
        {
            Layer* layer = AddLayer();
            layer->Deserialize(layerData);
        }
    }

    mLayerOrder.clear();
    if (in.contains("layer_order"))
    {
        for (const auto &id : in["layer_order"])
        {
            mLayerOrder.push_back(id);
        }
    }
}

void Shaper::ExportPNG(const std::string &path)
{
    if (mLayers.empty()) return;

    std::unique_ptr<olc::Sprite> out = std::make_unique<olc::Sprite>(mWidth, mHeight);
    RenderAll();

    // compose final image 
    for (const auto& layerID : mLayerOrder)
    {
        Layer* layer = GetLayer(layerID);
        if (!layer) continue;

        for (int y = 0; y < mHeight; y++)
        {
            for (int x = 0; x < mWidth; x++)
            {
                olc::Pixel src = layer->GetSurface()->GetPixel(x, y);
                olc::Pixel dst = out->GetPixel(x, y);

                float alpha = src.a / 255.0f;
                float invAlpha = 1.0f - alpha;

                olc::Pixel result;
                result.r = uint8_t(std::clamp(int(src.r * alpha + dst.r * invAlpha), 0, 255));
                result.g = uint8_t(std::clamp(int(src.g * alpha + dst.g * invAlpha), 0, 255));
                result.b = uint8_t(std::clamp(int(src.b * alpha + dst.b * invAlpha), 0, 255));
                result.a = uint8_t(std::clamp(int(src.a + dst.a * invAlpha), 0, 255));

                out->SetPixel(x, y, result);
            }
        }
    }

    std::vector<uint8_t> imageData;
    imageData.reserve(mWidth * mHeight * 4); // RGBA

    for (int y = 0; y < mHeight; y++)
    {
        for (int x = 0; x < mWidth; x++)
        {
            olc::Pixel pixel = out->GetPixel(x, y);
            imageData.push_back(pixel.r);
            imageData.push_back(pixel.g);
            imageData.push_back(pixel.b);
            imageData.push_back(pixel.a);
        }
    }

    stbi_write_png(path.c_str(), mWidth, mHeight, 4, imageData.data(), mWidth * 4);
}

Layer *Shaper::GetLayer(size_t id) const
{
    auto it = std::find_if(mLayers.begin(), mLayers.end(),
        [=](const std::unique_ptr<Layer>& layer) { return layer->GetID() == id; });
    return (it != mLayers.end()) ? it->get() : nullptr;
}

std::vector<Layer*> Shaper::GetLayers() const
{
    std::vector<Layer*> layers;
    for (const auto& index : mLayerOrder)
        layers.push_back(GetLayer(index));
    return layers;
}

size_t Shaper::GetLayerOrder(size_t id) const
{
    auto it = std::find(mLayerOrder.begin(), mLayerOrder.end(), id);
    return (it != mLayerOrder.end()) ? std::distance(mLayerOrder.begin(), it) : size_t(-1);
}

void ContourEffect::Apply(Layer *target)
{
    if (!target) return;

    // this is a post-fx what applies a pixel art outline around a transparent image
    auto surface = target->GetSurface();
    if (!surface) return;

    // Create a copy of the original surface to avoid modifying it while reading
    auto originalSurface = std::make_unique<olc::Sprite>(surface->width, surface->height);
    for (int y = 0; y < surface->height; y++)
    {
        for (int x = 0; x < surface->width; x++)
        {
            originalSurface->SetPixel(x, y, surface->GetPixel(x, y));
        }
    }

    int neighborsToCheck[] = {
        -1, 0,
        1, 0,
        0, -1,
        0, 1
    };

    for (int y = 0; y < surface->height; y++)
    {
        for (int x = 0; x < surface->width; x++)
        {
            olc::Pixel color = originalSurface->GetPixel(x, y);
            if (color.a == 0) // If the pixel is transparent
            {
                bool shouldDrawContour = false;
                
                // Check surrounding pixels with bounds checking
                for (int i = 0; i < 4 && !shouldDrawContour; i++) {
                    int dx = neighborsToCheck[i * 2];
                    int dy = neighborsToCheck[i * 2 + 1];

                    int nx = x + dx;
                    int ny = y + dy;
                    
                    // Check bounds before accessing pixel
                    if (nx >= 0 && nx < surface->width && ny >= 0 && ny < surface->height)
                    {
                        olc::Pixel neighbor = originalSurface->GetPixel(nx, ny);
                        if (neighbor.a != 0) // If a neighboring pixel is opaque
                        {
                            shouldDrawContour = true;
                        }
                    }
                }
                
                if (shouldDrawContour)
                {
                    surface->SetPixel(x, y, mColor);
                }
            }
        }
    }
}

void ContourEffect::Serialize(json &out) const
{
    Effect::Serialize(out);
    out["color"] = { mColor.r, mColor.g, mColor.b, mColor.a };
}

void ContourEffect::Deserialize(const json &in)
{
    Effect::Deserialize(in);
    if (in.contains("color")) {
        mColor = { in["color"][0], in["color"][1], in["color"][2], in["color"][3] };
    }
}

void Effect::Serialize(json &out) const
{
    out["enabled"] = mEnabled;
}

void Effect::Deserialize(const json &in)
{
    if (in.contains("enabled")) {
        mEnabled = in["enabled"];
    }
}

void ShadingEffect::Apply(Layer *target)
{
    if (!target) return;

    auto surface = target->GetSurface();
    if (!surface) return;

    auto fnStep = [](float a, float b)
    {
        return (a < b) ? 1.0f : 0.0f;
    };

    for (int y = 0; y < surface->height; y++)
    {
        for (int x = 0; x < surface->width; x++)
        {
            olc::Pixel originalColor = surface->GetPixel(x, y);
            
            // Skip transparent pixels
            if (originalColor.a == 0) continue;
            
            // Calculate light direction: from pixel to light source
            vec3 L{ float(mLightPosition.x - x), float(mLightPosition.y - y), float(surface->width) / 2.0f };
            L = L.norm();

            olc::Pixel normalMap = target->GetNormals()->GetPixel(x, y);
            vec3 n{
                (static_cast<float>(normalMap.r) / 255.0f) * 2.0f - 1.0f,
                (static_cast<float>(normalMap.g) / 255.0f) * 2.0f - 1.0f,
                (static_cast<float>(normalMap.b) / 255.0f) * 2.0f - 1.0f
            };

            // Calculate the lighting effect based on the light position
            float lightIntensity = std::max(0.0f, n.dot(L));

            // Apply toon shading threshold
            float intensity = fnStep(lightIntensity, 0.5f);

            // Create shaded color: blend between original color and shadow color
            olc::Pixel litColor = originalColor;
            olc::Pixel shadowedColor = olc::PixelLerp(originalColor, mColor, 0.7f);
            
            // Choose between lit and shadowed based on intensity
            olc::Pixel resultColor = intensity < 0.75f ? litColor : shadowedColor;
            // olc::Pixel resultColor = olc::PixelLerp(litColor, shadowedColor, intensity);

            // Apply final intensity blending
            olc::Pixel finalColor = olc::PixelLerp(originalColor, resultColor, mIntensity);
            finalColor.a = originalColor.a; // Preserve alpha
            
            surface->SetPixel(x, y, finalColor);
        }
    }
}

void ShadingEffect::Serialize(json &out) const
{
    Effect::Serialize(out);
    out["intensity"] = mIntensity;
    out["color"] = { mColor.r, mColor.g, mColor.b, mColor.a };
    out["light_position"] = { mLightPosition.x, mLightPosition.y };
}

void ShadingEffect::Deserialize(const json &in)
{
    Effect::Deserialize(in);
    if (in.contains("intensity")) {
        mIntensity = in["intensity"];
    }
    if (in.contains("color")) {
        mColor = { in["color"][0], in["color"][1], in["color"][2], in["color"][3] };
    }
    if (in.contains("light_position")) {
        mLightPosition = { in["light_position"][0], in["light_position"][1] };
    }
}

float TriangleElement::GetSDF(olc::vf2d p) const
{
    // p is in normalized coordinates where the triangle should be a unit triangle
    // Simple triangle SDF - equilateral triangle with height 2 and base 2
    auto fnSign = [](float v) {
        return (v > 0) - (v < 0);
    };

    olc::vf2d q{ 1.0f, 1.0f }; // Unit triangle extents
    p.x = std::abs(p.x);
    p.y *= 0.5f;
    p.y += q.y * 0.5f;
    olc::vf2d a = p - q * std::clamp(p.dot(q) / q.dot(q), 0.0f, 1.0f);
    olc::vf2d b = p - q * olc::vf2d(std::clamp(p.x / q.x, 0.0f, 1.0f), 1.0f);
    float k = fnSign(q.y);
    float d = std::min(a.dot(a), b.dot(b));
    float s = std::max(k * (p.x * q.y - p.y * q.x), k * (p.y - q.y));
    return std::sqrt(d) * fnSign(s);
}

bool TriangleElement::IsPointInside(const olc::vi2d &point) const
{
    olc::vi2d center = GetPosition();
    olc::vi2d size = GetSize();
    float rotation = GetRotation();
    
    // Translate point to triangle's local coordinate system
    olc::vi2d localPoint = point - center;
    
    // Rotate point by negative rotation to align with triangle's axes
    float cosAngle = ::cos(-rotation);
    float sinAngle = ::sin(-rotation);
    
    float rotatedX = localPoint.x * cosAngle - localPoint.y * sinAngle;
    float rotatedY = localPoint.x * sinAngle + localPoint.y * cosAngle;
    
    // Define triangle vertices in local space (isosceles triangle)
    // Triangle points upward with base at bottom
    float halfWidth = size.x / 2.0f;
    float halfHeight = size.y / 2.0f;
    
    // Triangle vertices: top point and two base points
    olc::vf2d v0{ 0.0f, -halfHeight };        // Top vertex
    olc::vf2d v1{ -halfWidth, halfHeight };   // Bottom left
    olc::vf2d v2{ halfWidth, halfHeight };    // Bottom right
    
    olc::vf2d p{ rotatedX, rotatedY };
    
    // Use barycentric coordinates to test if point is inside triangle
    float denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
    
    if (::abs(denom) < 1e-6f) return false; // Degenerate triangle
    
    float a = ((v1.y - v2.y) * (p.x - v2.x) + (v2.x - v1.x) * (p.y - v2.y)) / denom;
    float b = ((v2.y - v0.y) * (p.x - v2.x) + (v0.x - v2.x) * (p.y - v2.y)) / denom;
    float c = 1.0f - a - b;
    
    return (a >= 0.0f && b >= 0.0f && c >= 0.0f);
}

void TriangleElement::Serialize(json &out) const
{
    out["type"] = "triangle";
    Element::Serialize(out);
}
