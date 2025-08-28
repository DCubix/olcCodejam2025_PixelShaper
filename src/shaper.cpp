#include "shaper.h"

#include <algorithm>
#include <cmath>

#include "stb_image_write.h"

size_t Layer::mNextID = 1;

float EllipseElement::GetSDF(olc::vf2d p) const
{
    // p is now in pixel coordinates relative to the element's center
    // Get half-extents
    olc::vf2d ab{ mSize.x / 2.0f, mSize.y / 2.0f };
    
    // Handle degenerate cases
    if (ab.x <= 0.0f || ab.y <= 0.0f) return 1e30f;
    
    if (mSize.x == mSize.y)
    {
        // Circle case - simpler calculation
        float r = ab.x;
        return p.mag() - r;
    }

    auto fnSign = [](float v) {
        return (v > 0) - (v < 0);
    };

    olc::vf2d ap{ ::abs(p.x), ::abs(p.y) };
    
    if (ap.x > ap.y)
    {
        std::swap(ap.x, ap.y);
        std::swap(ab.x, ab.y);
    }

    float l = ab.y * ab.y - ab.x * ab.x;
    
    // Handle case where ellipse is nearly circular
    if (::abs(l) < 1e-6f)
    {
        float r = (ab.x + ab.y) * 0.5f;
        return ap.mag() - r;
    }
    
    float m = ab.x * ap.x / l, m2 = m * m;
    float n = ab.y * ap.y / l, n2 = n * n;
    float c = (m2 + n2 - 1.0f) / 3.0f, c3 = c * c * c;
    float q = c3 + m2 * n2 * 2.0f;
    float d = c3 + m2 * n2;
    float g = m + m * n2;
    float co;

    if (d < 0.0f)
    {
        float h = ::acosf(std::clamp(q / c3, -1.0f, 1.0f)) / 3.0f;
        float s = ::cosf(h);
        float t = ::sinf(h) * ::sqrtf(3.0f);
        float rx = ::sqrtf(std::max(0.0f, -c * (s + t + 2.0f) + m2));
        float ry = ::sqrtf(std::max(0.0f, -c * (s - t + 2.0f) + m2));
        
        // Avoid division by zero
        float denom = rx * ry;
        if (::abs(denom) < 1e-6f) {
            co = 0.0f;
        } else {
            co = (ry + fnSign(l) * rx + ::abs(g) / denom - m) / 2.0f;
        }
    }
    else
    {
        float h = 2.0f * m * n * ::sqrtf(std::max(0.0f, d));
        float s = fnSign(q + h) * ::powf(::abs(q + h), 1.0f / 3.0f);
        float u = fnSign(q - h) * ::powf(::abs(q - h), 1.0f / 3.0f);
        float rx = -s - u - c * 4.0f + 2.0f * m2;
        float ry = (s - u) * ::sqrtf(3.0f);
        float rm = ::sqrtf(std::max(0.0f, rx * rx + ry * ry));
        
        // Avoid division by zero
        if (rm < 1e-6f) {
            co = 0.0f;
        } else {
            float sqrtArg = std::max(0.0f, rm - rx);
            if (sqrtArg < 1e-6f) {
                co = 0.0f;
            } else {
                co = (ry / ::sqrtf(sqrtArg) + 2.0f * g / rm - m) / 2.0f;
            }
        }
    }
    
    // Clamp co to valid range
    co = std::clamp(co, 0.0f, 1.0f);
    
    olc::vf2d r = ab * olc::vf2d(co, ::sqrtf(std::max(0.0f, 1.0f - co * co)));
    return (r - ap).mag() * fnSign(ap.y - r.y);
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
    out["position"] = { mPosition.x, mPosition.y };
    out["size"] = { mSize.x, mSize.y };
    out["rotation"] = mRotation;
    out["color"] = { mColor.r, mColor.g, mColor.b, mColor.a };
    out["subtractive"] = mSubtractive;
}

void Element::Deserialize(const json &in)
{
    mPosition = { in["position"][0], in["position"][1] };
    mSize = { in["size"][0], in["size"][1] };
    mRotation = in["rotation"];
    mColor = { in["color"][0], in["color"][1], in["color"][2], in["color"][3] };
    mSubtractive = in["subtractive"];
}

float RectangleElement::GetSDF(olc::vf2d p) const
{
    // p is now in pixel coordinates relative to the element's center
    olc::vf2d ap{ ::abs(p.x), ::abs(p.y) };
    olc::vf2d b{ mSize.x / 2.0f, mSize.y / 2.0f };
    olc::vf2d d = ap - b;
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

void Layer::AddElement(Element *element)
{
    mElements.push_back(std::unique_ptr<Element>(element));
}

void Layer::RemoveElement(Element *element)
{
    auto it = std::remove_if(mElements.begin(), mElements.end(),
        [element](const std::unique_ptr<Element>& e) { return e.get() == element; });
    mElements.erase(it, mElements.end());
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

struct v3 {
    float x, y, z;

    v3 operator+(const v3& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }

    v3 operator-(const v3& other) const {
        return { x - other.x, y - other.y, z - other.z };
    }

    v3 operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    float dot(const v3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    v3 cross(const v3& other) const {
        return { y * other.z - z * other.y,
                 z * other.x - x * other.z,
                 x * other.y - y * other.x };
    }

    v3 norm() const {
        float len = mag();
        return (len > 0) ? (*this * (1.0f / len)) : v3{ 0, 0, 0 };
    }

    float mag() const {
        return std::sqrt(dot(*this));
    }
};

void Layer::Render()
{
    if (!mSurface) return;

    auto fnStep = [](float a, float b)
    {
        return (a < b) ? 1.0f : 0.0f;
    };

    auto fnSmoothUnion = [](float d1, float d2, float k)
    {
        float h = std::clamp(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
        return std::lerp(d2, d1, h) - k * h * (1.0f - h);
    };

    auto fnSubtract = [](float d1, float d2)
    {
        return std::max(d1, -d2);
    };

    std::vector<float> sdfMap;
    sdfMap.resize(mSurface->width * mSurface->height);

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
                
                // Transform pixel coordinates to element's local coordinate system
                olc::vf2d worldPos{ float(x), float(y) };
                olc::vf2d localPos = worldPos - olc::vf2d(el->GetPosition().x, el->GetPosition().y);
                
                float cosAngle = ::cos(-el->GetRotation());
                float sinAngle = ::sin(-el->GetRotation());
                olc::vf2d rotatedPos{
                    localPos.x * cosAngle - localPos.y * sinAngle,
                    localPos.x * sinAngle + localPos.y * cosAngle
                };

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
                // Transform pixel coordinates to element's local coordinate system
                olc::vf2d worldPos{ float(x), float(y) };
                olc::vf2d localPos = worldPos - olc::vf2d(el->GetPosition().x, el->GetPosition().y);
                
                float cosAngle = ::cos(-el->GetRotation());
                float sinAngle = ::sin(-el->GetRotation());
                olc::vf2d rotatedPos{
                    localPos.x * cosAngle - localPos.y * sinAngle,
                    localPos.x * sinAngle + localPos.y * cosAngle
                };

                float sdf = el->GetSDF(rotatedPos);
                
                if (!el->IsSubtractive())
                {
                    if (firstElement) {
                        sdfAccum = sdf;
                        firstElement = false;
                    } else {
                        sdfAccum = fnSmoothUnion(sdfAccum, sdf, mMergeSmoothness + 0.2f);
                    }
                }
                else
                {
                    sdfAccum = fnSubtract(sdfAccum, sdf);
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
            return 1.0f;
        return sdfMap[y * mSurface->width + x];
    };

    const float e = 10.0f / mSurface->width;
    for (int y = 0; y < mSurface->height; y++)
    {
        for (int x = 0; x < mSurface->width; x++)
        {
            float sdf = fnSampleSDF(x, y);
            float dx = fnSampleSDF(x + 1, y) - fnSampleSDF(x - 1, y);
            float dy = fnSampleSDF(x, y + 1) - fnSampleSDF(x, y - 1);
            v3 n{ -dx, -dy, 2.0f * e };
            n = n.norm();
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
    mID = in["id"];
    mName = in["name"];
    mMergeSmoothness = in["merge_smoothness"];

    Layer::mNextID = std::max(Layer::mNextID, mID + 1);

    // Deserialize elements
    mElements.clear();
    for (const auto &elementData : in["elements"])
    {
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
        // Add other element types here as needed

        if (element)
        {
            element->Deserialize(elementData);
            AddElement(element);
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
    mWidth = in["width"];
    mHeight = in["height"];
    Resize(mWidth, mHeight);

    mLayers.clear();
    for (const auto &layerData : in["layers"])
    {
        Layer* layer = AddLayer();
        layer->Deserialize(layerData);
    }

    mLayerOrder.clear();
    for (const auto &id : in["layer_order"])
    {
        mLayerOrder.push_back(id);
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
    mColor = { in["color"][0], in["color"][1], in["color"][2], in["color"][3] };
}

void Effect::Serialize(json &out) const
{
    out["enabled"] = mEnabled;
}

void Effect::Deserialize(const json &in)
{
    mEnabled = in["enabled"];
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
            v3 L{ float(mLightPosition.x - x), float(mLightPosition.y - y), float(surface->width) / 2.0f };
            L = L.norm();

            olc::Pixel normalMap = target->GetNormals()->GetPixel(x, y);
            v3 n{
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
            olc::Pixel resultColor = intensity > 0.5f ? litColor : shadowedColor;
            
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
    mIntensity = in["intensity"];
    mColor = { in["color"][0], in["color"][1], in["color"][2], in["color"][3] };
    mLightPosition = { in["light_position"][0], in["light_position"][1] };
}
