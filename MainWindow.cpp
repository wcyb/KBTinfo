#include "MainWindow.h"
#include "./ui_MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow), waveformChart(QChart()) {
  ui->setupUi(this);

  tabCrankingGrid = new QGridLayout(ui->tabCranking);
  waveformData = new QLineSeries(&waveformChart);
  axisX = new QValueAxis(waveformData);
  axisY = new QValueAxis(waveformData);

  QStyle* pbStyle = QStyleFactory::create("Fusion");
  pbStyle->setParent(this);
  ui->pbSoh->setStyle(pbStyle);
  ui->pbSoc->setStyle(pbStyle);

  waveformChart.legend()->hide();
  waveformChart.setTitle(tr("Battery voltage during cranking"));
  axisX->setLabelFormat("%.1f");
  axisX->setTitleText(tr("Time [s]"));
  axisX->setMinorTickCount(2);
  axisY->setLabelFormat("%.2f");
  axisY->setTitleText(tr("Voltage [V]"));
  axisY->setMinorTickCount(5);
  waveformChart.addAxis(axisX, Qt::AlignBottom);
  waveformChart.addAxis(axisY, Qt::AlignLeft);
  waveformChart.addSeries(waveformData);
  waveformData->attachAxis(axisX);
  waveformData->attachAxis(axisY);
  waveformChartView.setChart(&waveformChart);
  waveformChartView.setRenderHint(QPainter::Antialiasing);
  tabCrankingGrid->setContentsMargins(0, 0, 0, 0);
  tabCrankingGrid->addWidget(&waveformChartView);

  statusMsg = new QLabel(tr("Not connected."), ui->statusbar);
  ui->statusbar->addWidget(statusMsg);

  generateFcs16LookupTable(fcs16Lookup);

  if (readSettings()) { // if we have initialized settings, then COM port can be opened
    if (getSettingsValue(sAutoConnect, bool()).toBool())
      connectToDevice();
    else
      ui->connectToDevice->setEnabled(true);
  }
}

MainWindow::~MainWindow() {
  if (sp != nullptr)
    delete sp;
  delete tabCrankingGrid;
  delete waveformData;
  delete axisX;
  delete axisY;
  delete ui;
}

void MainWindow::saveState() {
  const QString fileName = QFileDialog::getSaveFileName(this, tr("Save battery state"), QDir::homePath(), tr("Text file (*.txt);;CSV file (*.csv)"));

  if (fileName.isEmpty()) {
    QMessageBox::warning(this, tr("Warning"), tr("The file name cannot be empty.\nNo file operations were performed."));
    return;
  }

  const QString fileType = fileName.section('.', -1);
  if (fileType != "txt" && fileType != "csv") {
    QMessageBox::warning(this, tr("Warning"), tr("Only txt and csv formats are supported.\nNo file operations were performed."));
    return;
  }

  QFile file(fileName);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(this, tr("Warning"), tr("Unable to create file.\nNo file operations were performed.\n") + file.errorString());
    return;
  }

  QTextStream data(&file);
  if (fileType == "txt") {
    data << ui->lSoh->text() << ' ' << ui->pbSoh->value() << '%' << '\n';
    data << ui->lSoc->text() << ' ' << ui->pbSoc->value() << '%' << '\n';
    data << removeTextFormatting(ui->lIntRes->text()) << ' ' << ui->lRes->text() << '\n';
    data << removeTextFormatting(ui->lTestNorm->text()) << ' ' << ui->lTNorm->text() << '\n';
    data << removeTextFormatting(ui->lTestRes->text()) << ' ' << ui->lTRes->text() << '\n';
    data << removeTextFormatting(ui->lVoltage->text()) << ' ' << ui->lVol->text() << '\n';
    data << removeTextFormatting(ui->lOverallCond->text()) << ' ' << ui->lCond->text() << '\n';
  } else if (fileType == "csv") {
    data << ui->lSoh->text() << ',' << ui->lSoc->text() << ',' << removeTextFormatting(ui->lIntRes->text()) << ',' << removeTextFormatting(ui->lTestNorm->text()) << ','
         << removeTextFormatting(ui->lTestRes->text()) << ',' << removeTextFormatting(ui->lVoltage->text()) << ',' << removeTextFormatting(ui->lOverallCond->text()) << '\n';
    data << ui->pbSoh->value() << '%' << ',' << ui->pbSoc->value() << '%' << ',' << ui->lRes->text() << ',' << ui->lTNorm->text() << ',' << ui->lTRes->text() << ',' << ui->lVol->text() << ','
         << ui->lCond->text() << '\n';
  }
  file.close();
}

void MainWindow::saveWaveform() {
  const QString fileName =
      QFileDialog::getSaveFileName(this, tr("Save waveform data"), QDir::homePath(), tr("PNG file (*.png);;JPG file (*.jpg);;JPEG file (*.jpeg);;BMP file (*.bmp);;CSV file (*.csv)"));

  if (fileName.isEmpty()) {
    QMessageBox::warning(this, tr("Warning"), tr("The file name cannot be empty.\nNo file operations were performed."));
    return;
  }

  const QString fileType = fileName.section('.', -1);
  const QVector<QString> allowedFileTypes{"png", "jpg", "jpeg", "bmp", "csv"};
  if (!allowedFileTypes.contains(fileType)) {
    QMessageBox::warning(this, tr("Warning"), tr("Only png, jpg, jpeg, bmp and csv formats are supported.\nNo file operations were performed."));
    return;
  }

  QFile file(fileName);
  bool fileOpenSuccess = false;
  if (fileType != "csv")
    fileOpenSuccess = file.open(QFile::WriteOnly);
  else if (fileType == "csv")
    fileOpenSuccess = file.open(QFile::WriteOnly | QFile::Text);
  if (!fileOpenSuccess) {
    QMessageBox::warning(this, tr("Warning"), tr("Unable to create file.\nNo file operations were performed.\n") + file.errorString());
    return;
  }

  if (fileType != "csv") {
    if (!waveformChartView.grab().save(&file)) {
      QMessageBox::warning(this, tr("Warning"), tr("Unable to save file.\nNo file operations were performed.\n"));
      return;
    }
  } else if (fileType == "csv") {
    QTextStream data(&file);

    data << axisX->titleText() << ',' << axisY->titleText() << '\n';
    for (const QPointF& chartPoint : waveformData->points())
      data << chartPoint.x() << ',' << chartPoint.y() << '\n';
  }
  file.close();
}

void MainWindow::saveBoth() {}

void MainWindow::printState() {}

void MainWindow::printWaveform() {}

void MainWindow::printBoth() {}

void MainWindow::showOptions() {
  OptionsDialog* dlg = new OptionsDialog(this);
  int res = dlg->exec();
  delete dlg;

  if (res == QDialog::Accepted && !ui->disconnectFromDevice->isEnabled())
    ui->connectToDevice->setEnabled(true);
}

void MainWindow::showAbout() {
  QString aboutCaption = QMessageBox::tr("<h2>About KBTinfo</h2>"
                                         "<p>Software for reading and visualization of data "
                                         "from Konnwei battery testers. "
                                         "It was made using Qt version %1.</p>")
                             .arg(QLatin1String(QT_VERSION_STR));
  QString aboutText = QMessageBox::tr("<p>Main program icon (car-battery-solid) and options icon "
                                      "(cog-solid), both "
                                      "without changes, were used under CC BY 4.0 license from Font "
                                      "Awesome icons set. "
                                      "License for these icons can be found <a href=\"%1\">here</a>.</p>"
                                      "<p>Program version: %2</p>"
                                      "<p>For updates and source code check <a href=\"%3\">my Github "
                                      "page</a>.</p>"
                                      "<p>Copyright (C) 2023 Wojciech Cybowski</p>"
                                      "<p></p>")
                          .arg(QStringLiteral("https://fontawesome.com/license"), QCoreApplication::applicationVersion(), QStringLiteral("https://github.com/wcyb"));
  QMessageBox msg(this);
  msg.setWindowTitle(tr("About KBTinfo"));
  msg.setText(aboutCaption);
  msg.setInformativeText(aboutText);
  msg.setIconPixmap(QPixmap(":/images/car-battery-solid.png").scaled(128, 128, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
  msg.exec();
}

void MainWindow::connectToDevice() {
  if (getSettingsValue(sPort, QString()).toString().isEmpty()) {
    QMessageBox::information(this, tr("Information"),
                             tr("Before connecting to the tester, you must first specify the port for communication.\nConnect the tester to your computer, then choose File->Options menu."));
    statusMsg->setText(tr("Connection unsuccessful."));
    return;
  }

  sp = new SerialPort(getSettingsValue(sPort, QString()).toString(), this);
  connect(sp, &SerialPort::serialPortDataReceived, this, &MainWindow::onSerialPortDataReceived);
  connect(sp, &SerialPort::serialPortError, this, &MainWindow::onSerialPortError);

  if (sp->openSerialPort()) {
    ui->connectToDevice->setEnabled(false);
    ui->disconnectFromDevice->setEnabled(true);
    cleanupAfterPacketProcessing();
    statusMsg->setText(tr("Connected successfully."));
  } else {
    sp->closeSerialPort();
    delete sp;
    statusMsg->setText(tr("Connection unsuccessfull."));
  }
}

void MainWindow::disconnectFromDevice() {
  ui->connectToDevice->setEnabled(true);
  ui->disconnectFromDevice->setEnabled(false);

  cleanupAfterPacketProcessing();
  sp->closeSerialPort();
  delete sp;
  statusMsg->setText(tr("Disconnected from the device."));
}

void MainWindow::updateFirmware() {}

void MainWindow::onSerialPortDataReceived(const QByteArray& dataBuff) {
  for (byte b : dataBuff)
    receivedPacket.append(static_cast<uchar>(b));

  if (!determinePacketType(receivedPacket)) { // if received data is not a full packet or if it is unknown, then we expect receiving the rest of data later
    if (++receivedDataCheckedTimes == 3)      // if after third time, data that we have is not a valid packet, then discard all data and clear serial port buffer
      cleanupAfterPacketProcessing();
  } else {
    do {
      switch (receivedPacketType) {
      case PacketType::BattInfo:
        displayBattInfo();
        cleanupAfterPacketProcessing();
        packetProcessed = true;
        break;
      case PacketType::Chart:
        if (receivedPacket.length() < getPacketLength(receivedPacket)) {
          receivedPacket.clear();
          packetProcessed = true;
        } else {
          ushort processedBytes = prepareChart();
          receivedPacket.erase(receivedPacket.cbegin(), receivedPacket.cbegin() + processedBytes);
          sp->removeDataFromBufferStart(processedBytes);
          if (!receivedPacket.length()) {
            cleanupAfterPacketProcessing();
            packetProcessed = true;
          } else
            receivedPacketType = PacketType::Unknown;
        }
        break;
      case PacketType::ChartDisplay:
        displayChart();
        cleanupAfterPacketProcessing();
        packetProcessed = true;
        break;
      case PacketType::Unknown:
        if (!determinePacketType(receivedPacket))
          packetProcessed = true;
        break;
      }
    } while (!packetProcessed);
    packetProcessed = false;
  }
}

void MainWindow::onSerialPortError(const QSerialPort::SerialPortError& error) {
  if (error == QSerialPort::NoError) // if for some reason we get this error, then we can safely ignore it
    return;

  disconnectFromDevice();

  QString message;
  switch (error) {
  case QSerialPort::DeviceNotFoundError:
    message = tr("The selected port cannot be opened because it does not exist.");
    break;
  case QSerialPort::PermissionError:
    message = tr("The selected port cannot be opened due to lack of permissions to perform this operation.");
    break;
  case QSerialPort::OpenError:
    message = tr("An already open port cannot be reopened.");
    break;
  case QSerialPort::NotOpenError:
    message = tr("The operation cannot be performed because the selected port is not open.");
    break;
  case QSerialPort::WriteError:
    message = tr("An error occurred while writing the data.");
    break;
  case QSerialPort::ReadError:
    message = tr("An error occurred while reading the data.");
    break;
  case QSerialPort::ResourceError:
    message = tr("A resource access error has occurred.");
    break;
  case QSerialPort::UnsupportedOperationError:
    message = tr("The requested operation is unsupported or prohibited.");
    break;
  case QSerialPort::TimeoutError:
    message = tr("The data timeout limit has been exceeded.");
    break;
  case QSerialPort::UnknownError:
    message = tr("An unidentified error has occurred.");
    break;
  case QSerialPort::NoError:
    break;
  }

  QMessageBox::critical(this, tr("Error - serial port"), message);
}

void MainWindow::generateFcs16LookupTable(ushort (&arrayToPopulate)[256]) {
  const ushort P = 0x8408;

  for (ushort b = 0; b < 256; b++) {
    ushort v = b;
    for (uchar i = 8; i > 0; i--) {
      v = (v & 1) ? (v >> 1) ^ P : v >> 1;
    }
    arrayToPopulate[b] = v & 0xFFFF;
  }
}

QPair<ushort, ushort> MainWindow::calculatePacketChecksums(const QVector<uchar>& packetToCheck) const {
  uint packetLen = getPacketLength(packetToCheck);
  ushort currentFcs = 0xFFFF;
  uint checksumKonnwei = 0;

  for (uint i = 0; i < packetLen; i++) {
    ushort lastFcs = currentFcs;
    currentFcs = (currentFcs >> 8) ^ fcs16Lookup[(currentFcs ^ packetToCheck[i]) & 0xFF];
    if (i < packetLen - 2)
      checksumKonnwei = (lastFcs ^ packetToCheck[i]) << 16 | currentFcs;
  }
  return QPair<ushort, ushort>{currentFcs, (checksumKonnwei ^ 0xFFFF) & 0xFFFF};
}

bool MainWindow::checkIfPacketChecksumsOk(const QVector<uchar>& packetToCheck) const {
  const ushort correctFcs = 0xF0B8;
  uint packetLen = getPacketLength(packetToCheck);
  QPair<ushort, ushort> checksums = calculatePacketChecksums(packetToCheck);
  ushort receivedChecksum = packetToCheck[packetLen - 1] << 8 | packetToCheck[packetLen - 2];
  // check if standard data checksum (first) is correct and if konnwei checksum (second) is correct
  if (checksums.first == correctFcs && receivedChecksum == checksums.second)
    return true;
  return false;
}

bool MainWindow::checkIfPacketCorrect(const QVector<uchar>& packetToCheck) const {
  if (this->receivedPacketType == PacketType::Unknown)
    return false;
  if (this->receivedPacketType == PacketType::BattInfo)
    if (!checkIfPacketHaveTrailer(packetToCheck))
      return false;
  if (this->receivedPacketType == PacketType::Chart || this->receivedPacketType == PacketType::ChartDisplay)
    if (!checkIfPacketHaveTrailer(packetToCheck) || !checkIfPacketChecksumsOk(packetToCheck))
      return false;
  return true;
}

bool MainWindow::checkIfPacketHaveTrailer(const QVector<uchar>& packetToCheck) const {
  if (packetToCheck.end()[-1] == 0x0A && packetToCheck.end()[-2] == 0x0D)
    return true;
  return false;
}

ushort MainWindow::getPacketLength(const QVector<uchar>& packetToCheck) const {
  return ((packetToCheck[3] << 8) | packetToCheck[2]) - 2; // omit new line chars in packet len
}

ushort MainWindow::getPacketEncoding(const QVector<uchar>& packetToCheck) const { return (packetToCheck[6] << 8) | packetToCheck[5]; }

bool MainWindow::isPacketBattInfo(const QVector<uchar>& packetToCheck) {
  const QVector<uchar> header{0x0, 0x24, 0x24, 0xFF, 0xFE};

  if (packetToCheck.length() < 9)
    return false;

  for (uchar i = 0; i < header.length(); i++) {
    if (packetToCheck[i] != header[i])
      return false;
  }
  this->receivedPacketType = PacketType::BattInfo;
  return true;
}

bool MainWindow::isPacketChart(const QVector<uchar>& packetToCheck) {
  const QVector<uchar> header{0x24, 0x24, 0, 0, 0xFF, 0x01};

  if (packetToCheck.length() < 10)
    return false;

  for (uchar i = 0; i < header.length(); i++) {
    if (i == 2 || i == 3) // second and third byte are packet length, so ignore them
      continue;
    if (packetToCheck[i] != header[i])
      return false;
  }
  this->receivedPacketType = PacketType::Chart;
  return true;
}

bool MainWindow::isPacketChartDisplay(const QVector<uchar>& packetToCheck) {
  const QVector<uchar> header{0x24, 0x24, 0x0A, 0x00, 0xFF, 0x02};

  if (packetToCheck.length() < 10)
    return false;

  for (uchar i = 0; i < header.length(); i++) {
    if (packetToCheck[i] != header[i])
      return false;
  }
  this->receivedPacketType = PacketType::ChartDisplay;
  return true;
}

bool MainWindow::determinePacketType(const QVector<uchar>& packetToCheck) {
  if (!isPacketBattInfo(packetToCheck))
    if (!isPacketChart(packetToCheck))
      if (!isPacketChartDisplay(packetToCheck))
        return false; // unknown packet type
  if (!checkIfPacketCorrect(packetToCheck))
    return false; // packet is not correct
  return true;    // packet type has been determined
}

bool MainWindow::readSettings() {
  QSettings settings(sSettingsFileName, QSettings::IniFormat, this);
  bool res = false;

  if (!settings.allKeys().size()) {
    QMessageBox::information(this, tr("Information"),
                             tr("Before connecting to the tester, you must first specify the port for communication.\nConnect the tester to your computer, then choose File->Options menu."));
    return false;
  }

  settings.beginGroup(sGroup);
  if (!settings.value(sPort, QString()).toString().isEmpty())
    res = true; // we have a COM port specified in settings
  settings.endGroup();
  return res;
}

const QVariant MainWindow::getSettingsValue(const QString& settingName, const QVariant& valueType) {
  QSettings settings(sSettingsFileName, QSettings::IniFormat, this);
  settings.beginGroup(sGroup);
  return settings.value(settingName, valueType);
}

ushort MainWindow::prepareChart() {
  receivedData = {receivedPacket.begin(), receivedPacket.begin() + getPacketLength(receivedPacket) + 2};
  static float timeVal = 0.0;

  if (chartReceivedAndDisplayed) { // if we are adding data to the current chart, then don't reset current time stamp, otherwise set timestamp to 0
    chartReceivedAndDisplayed = false;
    timeVal = 0.0;
    waveformData->clear();
  }

  for (int i = 6; i < getPacketLength(receivedData) - 2; i += 2) {
    waveformData->append(timeVal, ((receivedData[i + 1] << 8) | receivedData[i]) / 10.0);
    timeVal += 0.0125;
  }

  return receivedData.length();
}

void MainWindow::displayChart() {
  const QList<QPointF>& chartPoints = waveformData->points();
  const auto minMaxX = std::minmax_element(chartPoints.cbegin(), chartPoints.cend(), [](const QPointF& l, const QPointF& r) { return l.x() < r.x(); });
  const auto minMaxY = std::minmax_element(chartPoints.cbegin(), chartPoints.cend(), [](const QPointF& l, const QPointF& r) { return l.y() < r.y(); });

  axisX->setRange(minMaxX.first->x(), minMaxX.second->x());
  axisY->setRange(std::floor(minMaxY.first->y()), std::ceil(minMaxY.second->y()));

  chartReceivedAndDisplayed = true;

  ui->saveWaveform->setEnabled(true);
  ui->printWaveform->setEnabled(true);
  if (ui->saveState->isEnabled()) {
    ui->saveBoth->setEnabled(true);
    ui->printBoth->setEnabled(true);
  }
}

void MainWindow::displayBattInfo() {
  receivedData = {receivedPacket.begin() + 7, receivedPacket.end() - 2}; // discard header and codepage info from the begining and \r\n from the end
  QString battInfo = QString::fromUtf8(receivedData);
  QStringList splittedInfo = battInfo.split("\r\n");

  QString sohValue(splittedInfo[0].section('=', 1, 1).trimmed());
  sohValue.chop(1); // remove % sign
  ui->pbSoh->setValue(sohValue.toInt());

  QString socValue(splittedInfo[1].section('=', 1, 1).trimmed());
  socValue.chop(1); // remove % sign
  ui->pbSoc->setValue(socValue.toInt());

  QString testNorm(splittedInfo[2].section('=', 0, 0).trimmed() + '-' + splittedInfo[3].section('=', 1, 1).trimmed());
  ui->lTNorm->setText(testNorm);

  QString testRes(splittedInfo[2].section('=', 1, 1).trimmed());
  ui->lTRes->setText(testRes);

  QString resValue(splittedInfo[4].section('=', 1, 1).trimmed());
  resValue.chop(1);     // remove placeholder for omega sign
  resValue += "\u03A9"; // add omega sign
  ui->lRes->setText(resValue);

  QString battVoltage(splittedInfo[5].section('=', 1, 1).trimmed());
  ui->lVol->setText(battVoltage);

  QString battCond(splittedInfo[6].trimmed());
  ui->lCond->setText(battCond);

  ui->saveState->setEnabled(true);
  ui->printState->setEnabled(true);
  if (chartReceivedAndDisplayed) {
    ui->saveBoth->setEnabled(true);
    ui->printBoth->setEnabled(true);
  }
}

void MainWindow::cleanupAfterPacketProcessing() {
  receivedPacketType = PacketType::Unknown;
  receivedDataCheckedTimes = 0;
  receivedPacket.clear();
  receivedData.clear();
  sp->clearDataBuffer();
  sp->clearSerialPortDataBuffer();
}

QString MainWindow::removeTextFormatting(const QString& richText) const { return QTextDocumentFragment::fromHtml(richText).toPlainText().simplified(); }
