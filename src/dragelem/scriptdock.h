#ifndef SCRIPTDOCK_H
#define SCRIPTDOCK_H

#include "dockingarea.h"

#define LOWEROFFSET QPoint(0, 5)
#define UPPEROFFSET QPoint(0, -8)
#define UPPEROFFSETHAT QPoint(0, -21)

class ScriptArea;

class ScriptDock : public DockingArea
{
    void connect(DraggableElement *upper, DraggableElement *lower);

public:
    enum Type
    {
        Upper,
        Lower,
        Inner
    };

    ScriptDock(Type type, Sprite *sprite, DraggableElement *parent);

    inline DraggableElement* getParent() {return _parent;}

    virtual void dock(DraggableElement* elem);
    virtual void undock();

    ~ScriptDock();

protected:
    Type _type;
    DraggableElement* _parent;
};

#endif // SCRIPTDOCK_H
