#include "SerialPort.h"

SerialPort::SerialPort(const QString& portName, QObject* parent) : QObject{parent}, spName(portName) {}

SerialPort::~SerialPort() {
  if (sp->isOpen())
    closeSerialPort();
  delete sp;
}

bool SerialPort::openSerialPort() {
  if (sp != nullptr) {
    if (sp->isOpen())
      closeSerialPort();
    delete sp;
  }

  const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
  if (ports.count()) {
    for (const QSerialPortInfo& inf : ports) { // find serial port selected by the user
      if (inf.portName() == spName) {
        spInfo = inf;
        break;
      }
    }
  }

  if (spInfo.isNull())
    return false; // if we can't find the serial port selected by the user then we can't open it
  else {
    sp = new QSerialPort(spInfo, this);
    connect(sp, &QSerialPort::readyRead, this, &SerialPort::spDataReceived);
    connect(sp, &QSerialPort::errorOccurred, this, &SerialPort::spError);
    sp->setBaudRate(QSerialPort::Baud115200);
  }
  return sp->open(QIODeviceBase::ReadWrite);
}

bool SerialPort::writeDataToSerialPort(const QByteArray& dataToWrite) {
  const qint64 bytesWritten = sp->write(dataToWrite);
  if (bytesWritten != dataToWrite.length())
    return false; // if an error occured or not all bytes were sent to the serial port
  return true;
}

ushort SerialPort::removeDataFromBufferStart(const ushort& howManyBytes) {
  if (howManyBytes > spData.length())
    return spData.length(); // if we want to remove more than buffer holds, then do nothing

  spData.erase(spData.cbegin(), spData.cbegin() + howManyBytes);

  return spData.length();
}

void SerialPort::clearDataBuffer() { spData.clear(); }

bool SerialPort::clearSerialPortDataBuffer(const QSerialPort::Directions& dir) { return sp->clear(dir); }

void SerialPort::closeSerialPort() { sp->close(); }

void SerialPort::spDataReceived() {
  spData.append(sp->readAll());
  emit serialPortDataReceived(spData);
}

void SerialPort::spError(const QSerialPort::SerialPortError& error) {
  if (error == QSerialPort::ResourceError) {
    spData.clear(); // in case a device was disconnected during program operation, clean the data buffer
    clearSerialPortDataBuffer();
  }
  emit serialPortError(error);
}
