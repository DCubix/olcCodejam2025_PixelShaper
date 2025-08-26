#pragma once

#include "olcPixelGameEngine.h"

#include <string>
#include <vector>
#include <memory>

class Element {
public:
    Element() = default;
    virtual ~Element() = default;

    Element(
        const olc::vi2d& position,
        const olc::vi2d& size,
        float rotation,
        const olc::Pixel& color
    ) : mPosition(position), mSize(size), mRotation(rotation), mColor(color) {}

    virtual float GetSDF(olc::vf2d p) const = 0;
    virtual bool IsPointInside(const olc::vi2d& point) const = 0;

    void SetPosition(const olc::vi2d& position) { mPosition = position; }
    olc::vi2d GetPosition() const { return mPosition; }

    olc::vi2d GetSize() const { return mSize; }
    void SetSize(const olc::vi2d& size) { mSize = size; }

    void SetRotation(float rotation) { mRotation = rotation; }
    float GetRotation() const { return mRotation; }

    void SetColor(const olc::Pixel& color) { mColor = color; }
    olc::Pixel GetColor() const { return mColor; }

    void SetSubtractive(bool subtractive) { mSubtractive = subtractive; }
    bool IsSubtractive() const { return mSubtractive; }

    olc::vi2d mPosition{ 0, 0 };
    olc::vi2d mSize{ 1, 1 };
    float mRotation{ 0.0f };
    olc::Pixel mColor{ 255, 255, 255, 255 };
    bool mSubtractive{ false };
};

class EllipseElement : public Element {
public:
    EllipseElement() = default;
    EllipseElement(
        const olc::vi2d& position,
        const olc::vi2d& size,
        float rotation,
        const olc::Pixel& color
    ) : Element(position, size, rotation, color) {}

    float GetSDF(olc::vf2d p) const override;
    bool IsPointInside(const olc::vi2d& point) const override;
};

class RectangleElement : public Element {
public:
    RectangleElement() = default;
    RectangleElement(
        const olc::vi2d& position,
        const olc::vi2d& size,
        float rotation,
        const olc::Pixel& color
    ) : Element(position, size, rotation, color) {}

    float GetSDF(olc::vf2d p) const override;
    bool IsPointInside(const olc::vi2d& point) const override;
};

class Layer;

class Effect {
public:
    Effect() = default;
    virtual ~Effect() = default;

    virtual void Apply(Layer* target) = 0;

    bool mEnabled{ false };
};

class ContourEffect : public Effect {
public:
    void Apply(Layer* target) override;

    olc::Pixel mColor{ 0, 0, 0, 255 };
};

class ShadingEffect : public Effect {
public:
    void Apply(Layer* target) override;

    float mIntensity{ 0.5f };
    olc::Pixel mColor{ 0, 0, 0, 255 };
    olc::vi2d mLightPosition{ 0, 0 };
};

class Layer {
public:
    Layer() = default;
    Layer(int width, int height) {
        Resize(width, height);
        mID = mNextID++;
        mName = "Layer " + std::to_string(mID);
        mContourEffect = std::make_unique<ContourEffect>();
        mShadingEffect = std::make_unique<ShadingEffect>();
    }

    void AddElement(Element* element);
    void RemoveElement(Element* element);

    void Resize(int width, int height);
    void Clear();

    void Render();

    std::string GetName() const { return mName; }
    void SetName(const std::string& name) { mName = name; }

    ShadingEffect* GetShadingEffect() { return mShadingEffect.get(); }
    ContourEffect* GetContourEffect() { return mContourEffect.get(); }

    float GetMergeSmoothness() const { return mMergeSmoothness; }
    void SetMergeSmoothness(float smoothness) { mMergeSmoothness = smoothness; }

    std::vector<Element*> GetElements() const;
    olc::Sprite* GetSurface() const { return mSurface.get(); }
    olc::Sprite* GetNormals() const { return mNormals.get(); }
    size_t GetID() const { return mID; }

private:
    std::vector<std::unique_ptr<Element>> mElements;
    std::unique_ptr<olc::Sprite> mSurface, mNormals;

    std::unique_ptr<ShadingEffect> mShadingEffect;
    std::unique_ptr<ContourEffect> mContourEffect;
    float mMergeSmoothness{ 0.0f };

    size_t mID;
    std::string mName{ "Layer" };

    static size_t mNextID;
};

class Shaper {
public:
    Shaper() = default;
    Shaper(int width, int height)
        : mWidth(width), mHeight(height) {}

    Layer* AddLayer();
    void RemoveLayer(size_t id);

    Layer* MoveLayerUp(size_t id);
    Layer* MoveLayerDown(size_t id);

    void RenderAll();
    void Resize(int width, int height);

    int GetWidth() const { return mWidth; }
    int GetHeight() const { return mHeight; }

    Layer* GetLayer(size_t id) const;
    std::vector<Layer*> GetLayers() const;
    std::vector<size_t> GetLayerOrder() const { return mLayerOrder; }
private:
    std::vector<std::unique_ptr<Layer>> mLayers;
    std::vector<size_t> mLayerOrder;
    int mWidth{ 100 };
    int mHeight{ 100 };
};