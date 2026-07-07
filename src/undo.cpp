/// Undo/redo system implementation.
#include "undo.h"

void UndoHistory::SaveState(const Canvas &canvas, const char *label)
{
    // Discard any redo states beyond current position
    if (m_pos + 1 < (int)m_states.size())
        m_states.erase(m_states.begin() + m_pos + 1, m_states.end());

    Snapshot snap;
    snap.mode = canvas.mode;
    snap.extent = canvas.extent;
    snap.target = canvas.target;
    snap.currentFrame = canvas.currentFrame;
    snap.loopFrame = canvas.loopFrame;
    snap.loopEndFrame = canvas.loopEndFrame;
    snap.frames = canvas.frames;
    snap.label = label ? label : "";
    m_states.push_back(std::move(snap));
    m_pos = (int)m_states.size() - 1;

    // Limit history size. The saved-point marker shifts with the trimmed
    // entry; once the saved state itself is trimmed it becomes unreachable
    // (-1), so the canvas can never falsely compare as saved again.
    if ((int)m_states.size() > m_maxHistory)
    {
        m_states.erase(m_states.begin());
        m_pos--;
        if (m_savedPos >= 0)
            m_savedPos--;
    }
}

bool UndoHistory::Undo(Canvas &canvas)
{
    if (!CanUndo()) return false;
    m_pos--;
    const auto &snap = m_states[m_pos];
    canvas.mode = snap.mode;
    canvas.extent = snap.extent;
    canvas.target = snap.target;
    canvas.currentFrame = snap.currentFrame;
    canvas.loopFrame = snap.loopFrame;
    canvas.loopEndFrame = snap.loopEndFrame;
    canvas.frames = snap.frames;
    return true;
}

bool UndoHistory::Redo(Canvas &canvas)
{
    if (!CanRedo()) return false;
    m_pos++;
    const auto &snap = m_states[m_pos];
    canvas.mode = snap.mode;
    canvas.extent = snap.extent;
    canvas.target = snap.target;
    canvas.currentFrame = snap.currentFrame;
    canvas.loopFrame = snap.loopFrame;
    canvas.loopEndFrame = snap.loopEndFrame;
    canvas.frames = snap.frames;
    return true;
}

void UndoHistory::GoTo(int pos, Canvas &canvas)
{
    if (pos < 0 || pos >= (int)m_states.size()) return;
    m_pos = pos;
    const auto &snap = m_states[m_pos];
    canvas.mode = snap.mode;
    canvas.extent = snap.extent;
    canvas.target = snap.target;
    canvas.currentFrame = snap.currentFrame;
    canvas.loopFrame = snap.loopFrame;
    canvas.loopEndFrame = snap.loopEndFrame;
    canvas.frames = snap.frames;
}

const char *UndoHistory::GetLabel(int idx) const
{
    if (idx < 0 || idx >= (int)m_states.size()) return "";
    return m_states[idx].label.c_str();
}

void UndoHistory::Clear()
{
    m_states.clear();
    m_pos = -1;
    m_savedPos = -1;
}

void UndoHistory::SetMaxHistory(int max)
{
    if (max < 10) max = 10;
    m_maxHistory = max;
    while ((int)m_states.size() > m_maxHistory)
    {
        m_states.erase(m_states.begin());
        m_pos--;
        if (m_savedPos >= 0)
            m_savedPos--;
    }
}
