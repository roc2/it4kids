#include "paramstring.h"

ParamString::ParamString(QWidget *parent) : QLineEdit(parent)
{
    setFixedSize(50, 15);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setFont(QFont("Courier", 7));
    #pragma message("Fix GetValue")
}

QString ParamString::getValue()
{
    return text();
}

bool ParamString::setValue(const QString &value)
{
    if(value == "0") setText("Hello!");
    else setText(value);
    return 1;
}

ParamString::~ParamString()
{

}
