// fake QLabel
#include <qstring.h>
#include <qwidget.h>

class QLabel
{
public:
    QLabel(void *) {}
    void setText(const QString &) {}
};
