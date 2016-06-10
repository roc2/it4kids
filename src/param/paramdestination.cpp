#include <QApplication>

#include "paramdestination.h"

#include "mainwindow.h"
#include "sprite.h"
#include "spriteselect.h"

extern MainWindow* sMainWindow;

ParamDestination::ParamDestination(QWidget *parent) : QComboBox(parent)
{
    addItem("mouse");
    SpriteVector* sV = sMainWindow->getSpriteVector();
    for(uint i = 0; i < sV->size(); i++)
    {
        addItem(sV->at(i)->getName());
    }
}

QString ParamDestination::getString() const
{
    return currentText();
}

ParamDestination::~ParamDestination()
{

}
