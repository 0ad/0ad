// fake qwidget.h
#ifndef __FAKE__QWIDGET_H
#define __FAKE__QWIDGET_H

class QString;

class QWidget
{
public:
    bool isMinimized() { return false; }
    void close(bool) {}
    void showMinimized() {}
    void showNormal() {}
    void setCaption(const QString &) {}
    void setIcon(void *) {}
    int x() { return 0; }
    int y() { return 0; }
    int width() { return 0; }
    int height() { return 0; }
    void setGeometry(int, int, int, int) {}
};

#endif // __FAKE__QWIDGET_H
