#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QSerialPortInfo>
#include <QSettings>

#include "SettingsNames.h"

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog {
  Q_OBJECT

public:
  explicit OptionsDialog(QWidget* parent = nullptr);
  ~OptionsDialog();

private:
  Ui::OptionsDialog* ui;

private:
  bool saveSettingsFile(void);
  bool readSettingsFile(void);

private slots:
  void saveSettings(void);

private:
  // send by original software but no response implemented in firmware
  const QByteArray checkIfAliveDataPacket = QByteArray::fromHex("5E5E0A000401D1300D0A");
};

#endif // OPTIONSDIALOG_H
