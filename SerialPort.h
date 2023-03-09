#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class SerialPort : public QObject {
  Q_OBJECT
public:
  explicit SerialPort(const QString& portName, QObject* parent = nullptr);
  ~SerialPort();

public:
  bool openSerialPort(); // opens selected serial port and returns true or false if port can't be opened
  bool writeDataToSerialPort(const QByteArray& dataToWrite);
  // Removes the desired number of bytes from the buffer and, if successful, returns the number of bytes removed.
  // If more data is requested to be deleted than is currently in the buffer, nothing is deleted and the current amount of data is returned.
  ushort removeDataFromBufferStart(const ushort& howManyBytes);
  void clearDataBuffer();                                                                          // clears our internal data buffer
  bool clearSerialPortDataBuffer(const QSerialPort::Directions& dir = QSerialPort::AllDirections); // clears Qt's data buffer
  void closeSerialPort();

signals:
  void serialPortDataReceived(const QByteArray& dataBuff);
  void serialPortError(const QSerialPort::SerialPortError& error);

private slots:
  void spDataReceived();
  void spError(const QSerialPort::SerialPortError& error);

private:
  QString spName; // name of the selected serial port
  QSerialPort* sp = nullptr;
  QSerialPortInfo spInfo;
  QByteArray spData; // internal buffer for the received data
};

#endif // SERIALPORT_H
