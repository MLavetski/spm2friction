#include "s2f.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    S2F w;
    w.show();

    return a.exec();
}
