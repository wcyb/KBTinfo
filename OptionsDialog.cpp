#include "OptionsDialog.h"
#include "ui_OptionsDialog.h"

OptionsDialog::OptionsDialog(QWidget* parent) : QDialog(parent), ui(new Ui::OptionsDialog) {
  ui->setupUi(this);

  const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
  if (ports.count()) {
    for (const QSerialPortInfo& inf : ports)
      ui->cbbComPort->addItem(QString("%1 (%2)").arg(inf.portName(), inf.description()));

    ui->cbbComPort->setEnabled(true);
    ui->cbAutoConnect->setEnabled(true);
  }
  readSettingsFile();
}

OptionsDialog::~OptionsDialog() { delete ui; }

bool OptionsDialog::saveSettingsFile() {
  QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

  if (!ui->cbbComPort->count())
    return false; // if we don't have any COM ports, then don't go further

  QString portDesc = ui->cbbComPort->currentText().section('(', 1, 1);
  portDesc.chop(1); // remove last char ')' from port description

  settings.beginGroup(sGroup);
  settings.setValue(sPort, ui->cbbComPort->currentText().section(' ', 0, 0));
  settings.setValue(sPortDesc, portDesc);
  settings.setValue(sAutoConnect, ui->cbAutoConnect->isChecked());
  settings.endGroup();

  return true;
}

bool OptionsDialog::readSettingsFile() {
  QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

  if (!settings.allKeys().size())
    return false; // do nothing if there is no settings file

  if (!ui->cbbComPort->count())
    return false; // if we don't have any COM ports, then don't go further

  settings.beginGroup(sGroup);
  for (ushort i = 0; i < ui->cbbComPort->count(); i++) {
    if (ui->cbbComPort->currentText().section(' ', 0, 0) == settings.value(sPort, QString()).toString()) {
      ui->cbbComPort->setCurrentIndex(i);
      break;
    }
  }
  settings.value(sAutoConnect, bool()).toBool() ? ui->cbAutoConnect->setChecked(true) : ui->cbAutoConnect->setChecked(false);
  settings.endGroup();

  return true;
}

void OptionsDialog::saveSettings() {
  saveSettingsFile();
  accept();
}
