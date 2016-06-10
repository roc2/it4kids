#ifndef COMMANDDE_H
#define COMMANDDE_H

#include "draggableelement.h"

class ScriptDock;

class CommandDE : public DraggableElement
{
    Q_OBJECT
public:
    CommandDE(const QString& identifier, const QString& text, const QColor& color, Sprite* sprite = 0, QWidget* parent = 0);

    void rearrangeUpperElems();
    void rearrangeLowerElems();

    void resize();

    virtual void removeChildDragElems();

    virtual ScriptDock *getDock(ScriptDock::Type type);

    ~CommandDE();
protected:
    DraggableElement* getCurrentElement(Sprite *sprite, QWidget* parent);

    void moveEvent(QMoveEvent*);

    ScriptDock _upperDock;
    ScriptDock _lowerDock;
};

#endif // COMMANDDE_H