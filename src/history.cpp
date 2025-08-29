#include "history.h"

void History::Push(ICommand* command)
{
    command->Execute();
    mUndoStack.push_back(std::unique_ptr<ICommand>(command));
    mRedoStack.clear();
}

void History::Undo()
{
    if (!mUndoStack.empty()) {
        mUndoStack.back()->Undo();
        mRedoStack.push_back(std::move(mUndoStack.back()));
        mUndoStack.pop_back();
    }
}

void History::Redo()
{
    if (!mRedoStack.empty()) {
        mRedoStack.back()->Execute();
        mUndoStack.push_back(std::move(mRedoStack.back()));
        mRedoStack.pop_back();
    }
}

void History::Reset()
{
    mUndoStack.clear();
    mRedoStack.clear();
}

bool History::CanUndo() const
{
    return !mUndoStack.empty();
}

bool History::CanRedo() const
{
    return !mRedoStack.empty();
}

void CmdChangeProperty::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Element* element = layer->GetElement(mRef.elementId);
    if (!element) return;

    // Store current properties for undo
    element->Serialize(mOldParams);

    // Apply new properties
    element->Deserialize(mNewParams);
}

void CmdChangeProperty::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Element* element = layer->GetElement(mRef.elementId);
    if (!element) return;

    element->Deserialize(mOldParams);
}

void CmdAddElement::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    std::string type = mParams["type"];
    Element* element = nullptr;
    if (type == "ellipse") {
        element = new EllipseElement();
    } else if (type == "rectangle") {
        element = new RectangleElement();
    }

    if (element) {
        element->Deserialize(mParams);
        mRef.elementId = layer->AddElement(element)->GetID();
    }
}

void CmdAddElement::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Element* element = layer->GetElement(mRef.elementId);
    if (element) {
        element->Serialize(mParams);
        layer->RemoveElement(element);
    }
}

void CmdDeleteElement::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Element* element = layer->GetElement(mRef.elementId);
    if (element) {
        element->Serialize(mParams);
        layer->RemoveElement(element);
    }
}

void CmdDeleteElement::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Element* element = nullptr;
    std::string type = mParams["type"];
    if (type == "ellipse") {
        element = new EllipseElement();
    } else if (type == "rectangle") {
        element = new RectangleElement();
    }

    if (element) {
        element->Deserialize(mParams);
        mRef.elementId = layer->AddElement(element)->GetID();
    }
}

void CmdChangeDrawingSize::Execute()
{
    if (!mTarget) return;
    mOldSize = { mTarget->GetWidth(), mTarget->GetHeight() };
    mTarget->Resize(mNewSize.x, mNewSize.y);
}

void CmdChangeDrawingSize::Undo()
{
    if (!mTarget) return;
    mTarget->Resize(mOldSize.x, mOldSize.y);
}

void CmdEffectEnable::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Effect* effect = layer->GetEffect(mType);
    if (!effect) return;

    mOldState = effect->mEnabled;
    effect->mEnabled = mEnable;
}

void CmdEffectEnable::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Effect* effect = layer->GetEffect(mType);
    if (!effect) return;

    effect->mEnabled = mOldState;
}

void CmdChangeMergeSmoothness::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    mOldSmoothness = layer->GetMergeSmoothness();
    layer->SetMergeSmoothness(mNewSmoothness);
}

void CmdChangeMergeSmoothness::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    layer->SetMergeSmoothness(mOldSmoothness);
}

void CmdChangeEffectProperty::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Effect* effect = layer->GetEffect(mType);
    if (!effect) return;

    // Apply new properties
    effect->Deserialize(mNewParams);
}

void CmdChangeEffectProperty::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    Effect* effect = layer->GetEffect(mType);
    if (!effect) return;

    effect->Deserialize(mOldParams);
}

void CmdAddLayer::Execute()
{
    Layer* layer = mRef.drawing->AddLayer();
    if (!layer) return;

    layer->Deserialize(mParams);
    mRef.layerId = layer->GetID();
}

void CmdAddLayer::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    layer->Serialize(mParams);
    mRef.drawing->RemoveLayer(mRef.layerId);
}

void CmdRemoveLayer::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    layer->Serialize(mParams);

    // remove id
    mParams.erase("id");
    mParams["index"] = mRef.drawing->GetLayerOrder(mRef.layerId);

    mRef.drawing->RemoveLayer(mRef.layerId);
}

void CmdRemoveLayer::Undo()
{
    Layer* layer = mRef.drawing->AddLayer();
    if (!layer) return;

    layer->Deserialize(mParams);
    mRef.layerId = layer->GetID();

    mRef.drawing->ReorderLayer(mRef.layerId, mParams["index"]);
}

void CmdMoveLayerUp::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    mRef.drawing->MoveLayerUp(mRef.layerId);
}

void CmdMoveLayerUp::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    mRef.drawing->MoveLayerDown(mRef.layerId);
}

void CmdMoveLayerDown::Execute()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    mRef.drawing->MoveLayerDown(mRef.layerId);
}

void CmdMoveLayerDown::Undo()
{
    Layer* layer = mRef.drawing->GetLayer(mRef.layerId);
    if (!layer) return;

    mRef.drawing->MoveLayerUp(mRef.layerId);
}
