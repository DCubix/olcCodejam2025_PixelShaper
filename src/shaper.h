#pragma once

#include "olcPixelGameEngine.h"

#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct vec3 {
    float x, y, z;

    vec3 operator+(const vec3& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }

    vec3 operator-(const vec3& other) const {
        return { x - other.x, y - other.y, z - other.z };
    }

    vec3 operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    float dot(const vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    vec3 cross(const vec3& other) const {
        return { y * other.z - z * other.y,
                 z * other.x - x * other.z,
                 x * other.y - y * other.x };
    }

    vec3 norm() const {
        float len = mag();
        return (len > 0) ? (*this * (1.0f / len)) : vec3{ 0, 0, 0 };
    }

    float mag() const {
        return std::sqrt(dot(*this));
    }
};

class ISerializable {
public:
    virtual void Serialize(json& out) const = 0;
    virtual void Deserialize(const json& in) = 0;
};

enum class JoinOperation : int {
    Union = 0,
    Intersection,
    Subtraction
};

struct ElementParams {
    olc::vi2d position{ 0, 0 };
    olc::vi2d size{ 1, 1 };
    float rotation{ 0.0f };
    olc::Pixel color{ 255, 255, 255, 255 };
    JoinOperation joinOperation{ JoinOperation::Union };
};

class Element : public ISerializable {
public:
    Element() = default;
    virtual ~Element() = default;

    Element(
        const olc::vi2d& position,
        const olc::vi2d& size,
        float rotation,
        const olc::Pixel& color
    ) : mPosition(position), mSize(size), mRotation(rotation), mColor(color) {
        mID = mNextID++;
    }

    virtual float GetSDF(olc::vf2d p) const = 0;
    virtual bool IsPointInside(const olc::vi2d& point) const = 0;

    virtual void Serialize(json& out) const override;
    virtual void Deserialize(const json& in) override;

    void SetPosition(const olc::vi2d& position) { mPosition = position; }
    olc::vi2d GetPosition() const { return mPosition; }

    olc::vi2d GetSize() const { return mSize; }
    void SetSize(const olc::vi2d& size) { mSize = size; }

    void SetRotation(float rotation) { mRotation = rotation; }
    float GetRotation() const { return mRotation; }

    void SetColor(const olc::Pixel& color) { mColor = color; }
    olc::Pixel GetColor() const { return mColor; }

    bool IsSubtractive() const { return mJoinOp == JoinOperation::Subtraction; }

    JoinOperation GetJoinOperation() const { return mJoinOp; }
    void SetJoinOperation(JoinOperation op) { mJoinOp = op; }

    ElementParams GetParams() const {
        return ElementParams{ mPosition, mSize, mRotation, mColor, mJoinOp };
    }

    void SetParams(const ElementParams& params) {
        mPosition = params.position;
        mSize = params.size;
        mRotation = params.rotation;
        mColor = params.color;
        mJoinOp = params.joinOperation;
    }

    size_t GetID() const { return mID; }

    olc::vi2d mPosition{ 0, 0 };
    olc::vi2d mSize{ 1, 1 };
    float mRotation{ 0.0f };
    olc::Pixel mColor{ 255, 255, 255, 255 };
    JoinOperation mJoinOp{ JoinOperation::Union };
    
private:
    size_t mID;
    static size_t mNextID;
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

    virtual void Serialize(json& out) const override;
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

    virtual void Serialize(json& out) const override;
};

// Isosceles triangle
class TriangleElement : public Element {
public:
    TriangleElement() = default;
    TriangleElement(
        const olc::vi2d& position,
        const olc::vi2d& size,
        float rotation,
        const olc::Pixel& color
    ) : Element(position, size, rotation, color) {}

    float GetSDF(olc::vf2d p) const override;
    bool IsPointInside(const olc::vi2d& point) const override;

    virtual void Serialize(json& out) const override;
};

class Layer;

class Effect : public ISerializable {
public:
    Effect() = default;
    virtual ~Effect() = default;

    virtual void Apply(Layer* target) = 0;
    virtual void Serialize(json& out) const override;
    virtual void Deserialize(const json& in) override;

    bool mEnabled{ false };
};

class ContourEffect : public Effect {
public:
    void Apply(Layer* target) override;
    void Serialize(json& out) const override;
    void Deserialize(const json& in) override;

    olc::Pixel mColor{ 0, 0, 0, 255 };
};

class ShadingEffect : public Effect {
public:
    void Apply(Layer* target) override;
    void Serialize(json& out) const override;
    void Deserialize(const json& in) override;

    float mIntensity{ 0.5f };
    olc::Pixel mColor{ 0, 0, 0, 255 };
    olc::vi2d mLightPosition{ 0, 0 };
};

enum class LayerEffectType {
    ShadingEffect,
    ContourEffect
};

class Layer : public ISerializable {
public:
    Layer() = default;
    Layer(int width, int height) {
        Resize(width, height);
        mID = mNextID++;
        mName = "Layer " + std::to_string(mID);
        mContourEffect = std::make_unique<ContourEffect>();
        mShadingEffect = std::make_unique<ShadingEffect>();
    }

    Element* AddElement(Element* element);
    void RemoveElement(Element* element);

    Element* GetElement(size_t id) const;

    void Resize(int width, int height);
    void Clear();

    void Render();

    void Serialize(json& out) const override;
    void Deserialize(const json& in) override;

    std::string GetName() const { return mName; }
    void SetName(const std::string& name) { mName = name; }

    ShadingEffect* GetShadingEffect() { return mShadingEffect.get(); }
    ContourEffect* GetContourEffect() { return mContourEffect.get(); }

    Effect* GetEffect(LayerEffectType type) {
        switch (type) {
            case LayerEffectType::ShadingEffect:
                return GetShadingEffect();
            case LayerEffectType::ContourEffect:
                return GetContourEffect();
            default:
                return nullptr;
        }
    }

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

class Shaper : public ISerializable {
public:
    Shaper() = default;
    Shaper(int width, int height)
        : mWidth(width), mHeight(height) {}

    Layer* AddLayer();
    void RemoveLayer(size_t id);

    Layer* MoveLayerUp(size_t id);
    Layer* MoveLayerDown(size_t id);
    void ReorderLayer(size_t id, size_t newIndex);

    void RenderAll();
    void Resize(int width, int height);

    void Serialize(json& out) const override;
    void Deserialize(const json& in) override;

    void ExportPNG(const std::string& path);

    int GetWidth() const { return mWidth; }
    int GetHeight() const { return mHeight; }

    Layer* GetLayer(size_t id) const;
    size_t GetLayerOrder(size_t id) const;
    std::vector<Layer*> GetLayers() const;
    std::vector<size_t> GetLayerOrder() const { return mLayerOrder; }
private:
    std::vector<std::unique_ptr<Layer>> mLayers;
    std::vector<size_t> mLayerOrder;
    int mWidth{ 100 };
    int mHeight{ 100 };
};