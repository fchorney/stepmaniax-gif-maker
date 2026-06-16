#pragma once
/// Snapshot-based undo/redo system for canvas state.

#include "canvas.h"
#include <string>
#include <vector>

class UndoHistory
{
public:
    // Call after any modifying operation to save current state.
    void SaveState(const Canvas &canvas, const char *label = "");

    // Undo: restores previous state. Returns true if undo was performed.
    bool Undo(Canvas &canvas);

    // Redo: restores next state. Returns true if redo was performed.
    bool Redo(Canvas &canvas);

    // Jump to a specific position in history.
    void GoTo(int pos, Canvas &canvas);

    bool CanUndo() const { return m_pos > 0; }
    bool CanRedo() const { return m_pos < (int)m_states.size() - 1; }
    bool HasUnsavedChanges() const { return m_pos != m_savedPos; }

    void MarkSaved() { m_savedPos = m_pos; }

    int GetPos() const { return m_pos; }
    int GetCount() const { return (int)m_states.size(); }
    const char *GetLabel(int idx) const;

    int GetMaxHistory() const { return m_maxHistory; }
    void SetMaxHistory(int max);

    // Clear all history (e.g., on New/Import).
    void Clear();

private:
    struct Snapshot
    {
        CanvasMode mode;
        int currentFrame;
        int loopFrame;
        int loopEndFrame;
        std::vector<CanvasFrame> frames;
        std::string label;
    };

    std::vector<Snapshot> m_states;
    int m_pos = -1;
    int m_savedPos = 0;
    int m_maxHistory = 100;
};
