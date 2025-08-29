#pragma once

#include <vector>
#include <memory>

#include "shaper.h"

struct LayerRef {
    Shaper* drawing;
    size_t layerId;
};

struct ElementRef {
    Shaper* drawing;
    size_t layerId;
    size_t elementId;
};

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

class History {
public:
    void Push(ICommand* command);
    void Undo();
    void Redo();
    void Reset();

    bool CanUndo() const;
    bool CanRedo() const;
private:
    std::vector<std::unique_ptr<ICommand>> mUndoStack;
    std::vector<std::unique_ptr<ICommand>> mRedoStack;
};

// Pre-defined commands
class CmdAddElement : public ICommand {
public:
    CmdAddElement(ElementRef ref, const json& params)
        : mRef(ref), mParams(params) {}

    void Execute() override;
    void Undo() override;

private:
    ElementRef mRef;
    json mParams;
};

class CmdDeleteElement : public ICommand {
public:
    CmdDeleteElement(ElementRef ref)
        : mRef(ref) {}

    void Execute() override;
    void Undo() override;

private:
    ElementRef mRef;
    json mParams;
};

class CmdChangeProperty : public ICommand {
public:
    CmdChangeProperty(ElementRef ref, const json& newParams)
        : mRef(ref), mNewParams(newParams) {}

    void Execute() override;
    void Undo() override;

private:
    ElementRef mRef;
    json mOldParams;
    json mNewParams;
};

class CmdChangeDrawingSize : public ICommand {
public:
    CmdChangeDrawingSize(Shaper* target, const olc::vi2d& newSize)
        : mTarget(target), mNewSize(newSize) {}

    void Execute() override;
    void Undo() override;

private:
    olc::vi2d mOldSize;
    olc::vi2d mNewSize;
    Shaper* mTarget;
};

class CmdChangeMergeSmoothness : public ICommand {
public:
    CmdChangeMergeSmoothness(LayerRef ref, float newSmoothness)
        : mRef(ref), mNewSmoothness(newSmoothness) {}

    void Execute() override;
    void Undo() override;

private:
    float mOldSmoothness;
    float mNewSmoothness;
    LayerRef mRef;
};

class CmdEffectEnable : public ICommand {
public:
    CmdEffectEnable(LayerRef ref, LayerEffectType type, bool enable)
        : mRef(ref), mType(type), mEnable(enable) {}

    void Execute() override;
    void Undo() override;

private:
    LayerRef mRef;
    LayerEffectType mType;
    bool mEnable;
    bool mOldState;
};

class CmdChangeEffectProperty : public ICommand {
public:
    CmdChangeEffectProperty(LayerRef ref, LayerEffectType type, const json& newParams)
        : mRef(ref), mType(type), mNewParams(newParams) {}

    void Execute() override;
    void Undo() override;

private:
    LayerRef mRef;
    LayerEffectType mType;
    json mOldParams;
    json mNewParams;
};

class CmdAddLayer : public ICommand {
public:
    CmdAddLayer(LayerRef ref, const json& params)
        : mRef(ref), mParams(params) {}

    void Execute() override;
    void Undo() override;

private:
    LayerRef mRef;
    json mParams;
};

class CmdRemoveLayer : public ICommand {
public:
    CmdRemoveLayer(LayerRef ref)
        : mRef(ref) {}

    void Execute() override;
    void Undo() override;

private:
    LayerRef mRef;
    json mParams;
};

class CmdMoveLayerUp : public ICommand {
public:
    CmdMoveLayerUp(LayerRef ref)
        : mRef(ref) {}

    void Execute() override;
    void Undo() override;

private:
    LayerRef mRef;
};

class CmdMoveLayerDown : public ICommand {
public:
    CmdMoveLayerDown(LayerRef ref)
        : mRef(ref) {}

    void Execute() override;
    void Undo() override;

private:
    LayerRef mRef;
};