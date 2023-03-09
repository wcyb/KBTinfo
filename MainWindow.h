#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts>

#include "OptionsDialog.h"
#include "SerialPort.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow* ui;

private slots:
  void saveState();
  void saveWaveform();
  void saveBoth();
  void printState();
  void printWaveform();
  void printBoth();
  void showOptions();
  void showAbout();

  void connectToDevice();
  void disconnectFromDevice();
  void updateFirmware();

  void onSerialPortDataReceived(const QByteArray& dataBuff);
  void onSerialPortError(const QSerialPort::SerialPortError& error);

private:
  void generateFcs16LookupTable(ushort (&arrayToPopulate)[256]);

  QPair<ushort, ushort> calculatePacketChecksums(const QVector<uchar>& packetToCheck) const;
  bool checkIfPacketChecksumsOk(const QVector<uchar>& packetToCheck) const;
  bool checkIfPacketCorrect(const QVector<uchar>& packetToCheck) const;
  bool checkIfPacketHaveTrailer(const QVector<uchar>& packetToCheck) const;

  ushort getPacketLength(const QVector<uchar>& packetToCheck) const;
  ushort getPacketEncoding(const QVector<uchar>& packetToCheck) const;

  bool isPacketBattInfo(const QVector<uchar>& packetToCheck);
  bool isPacketChart(const QVector<uchar>& packetToCheck);
  bool isPacketChartDisplay(const QVector<uchar>& packetToCheck);
  bool determinePacketType(const QVector<uchar>& packetToCheck);

  bool readSettings();
  const QVariant getSettingsValue(const QString& settingName, const QVariant& valueType);
  ushort prepareChart();
  void displayChart();
  void displayBattInfo();
  void cleanupAfterPacketProcessing();

  QString removeTextFormatting(const QString& richText) const;

private:
  enum class PacketType : uchar { Unknown, BattInfo, Chart, ChartDisplay };

  ushort fcs16Lookup[256];
  QVector<uchar> receivedPacket;
  PacketType receivedPacketType = PacketType::Unknown;
  QVector<uchar> receivedData;

  QGridLayout* tabCrankingGrid = nullptr;
  QLineSeries* waveformData = nullptr;
  QChart waveformChart;
  QValueAxis* axisX = nullptr;
  QValueAxis* axisY = nullptr;
  QChartView waveformChartView;
  bool chartReceivedAndDisplayed = false;

  SerialPort* sp = nullptr;
  QLabel* statusMsg = nullptr;

  ushort receivedDataCheckedTimes = 0; // if received data doesn't match to any type of packet, then check the buffer again
  bool packetProcessed = false;        // if data was processed, set to true to clear buffers
};
#endif // MAINWINDOW_H
