#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("Wojciech Cybowski");
  QCoreApplication::setOrganizationDomain("https://github.com/wcyb");
  QCoreApplication::setApplicationName("KBTinfo");
  QCoreApplication::setApplicationVersion("v1.0.0.0");

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString& locale : uiLanguages) {
    const QString baseName = "KBTinfo_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }
  MainWindow w;
  w.setWindowTitle(w.windowTitle() + ' ' + QCoreApplication::applicationVersion());
  w.show();
  return a.exec();
}
