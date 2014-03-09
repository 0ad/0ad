// fake qmessagebox.h

class QMessageBox
{
public:
    enum Icon { Information, Warning, Critical };
    static void *standardIcon(Icon) { return 0; }
};
