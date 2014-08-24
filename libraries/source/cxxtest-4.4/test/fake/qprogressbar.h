// fake qprogressbar.h

class QColorGroup
{
public:
    enum { Highlight };
};

class QColor
{
public:
    QColor(int, int, int) {}
};

class QPalette
{
public:
    void setColor(int, const QColor &) {}
};

class QProgressBar
{
public:
    QProgressBar(int, void *) {}
    void setProgress(int)  {}
    int progress()  { return 0; }
    QPalette palette() { return QPalette(); }
    void setPalette(const QPalette &) {}
};
