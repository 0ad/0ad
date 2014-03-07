// fake QApplication

class QWidget;

class QApplication
{
public:
    QApplication(int &, char **) {}
    void exec() {}
    void setMainWidget(void *) {}
    void processEvents() {}
    static QWidget *desktop() { return 0; }
    void *activeWindow() { return 0; }
};
