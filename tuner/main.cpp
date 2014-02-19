#include <QApplication>
#include <QDebug>
#include <QStringList>

#include "SetupDialog.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);

	SetupDialog win;
	win.show();

	return app.exec();
}
