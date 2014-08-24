// fake qstring.h
#ifndef __FAKE__QSTRING_H
#define __FAKE__QSTRING_H

class QString
{
public:
    QString() {}
    QString(const char *) {}
    bool operator==(const QString &) { return false; }

    static QString number(int) { return QString(); }
};

inline QString operator+(const QString &, const QString &) { return QString(); }

#endif // __FAKE__QSTRING_H
