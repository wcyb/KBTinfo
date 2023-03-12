# KBTinfo
Software for reading and visualizing data from Konnwei battery testers, created using Qt 6.  
Allows you to completely replace the "UPLINK" software from the manufacturer.  
Operation was tested on the KW650 tester, but the program should be compatible with all testers that are supported by "UPLINK", which are:

- KW210
- KW600
- KW650
- KW710
- KW720

The data received from the tester can be saved to a graphic or csv file, in the case of voltage waveforms, and to a text or csv file in the case of a battery parameters test.

![Voltage waveform](/doc/img/waveform.png)

![Battery state](/doc/img/state.png)

## Packet format
1. Type 1 packets

Packets containing data with battery parameters have the following format:
|      Header      | Packet type |       Code page       | Data | Trailer |
|:----------------:|:-----------:|:---------------------:|:----:|:-------:|
| 0x00, 0x24, 0x24 |  0xFF, 0xFE | 0xBD, 0x6F[^codepage] | Text |   \r\n  |
[^codepage]: [iso-8859-15](https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers)

2. Type 2 packets

Data packets have the following format:
|   Header   | Packet length |   Packet type   |         Data        |       FCS       | Trailer |
|:----------:|:-------------:|:---------------:|:-------------------:|:---------------:|:-------:|
| 0x24, 0x24 |    2 bytes    | 2 bytes[^ptype] | Binary data[^dtype] | CRC-CCITT[^crc] |   \r\n  |
[^ptype]: Voltage waveform: 0xFF, 0x01  
  Chart display: 0xFF, 0x02
[^dtype]: For voltage waveforms, 2 bytes for each sample.  
  For the waveform display command, this section is empty.
[^crc]: [CRC-CCITT](https://github.com/torvalds/linux/blob/master/lib/crc-ccitt.c)  
  [FCS16](https://github.com/lobaro/util-slip/blob/master/fcs16.c)

## Other important information
- Most of the transmitted data is 16 bits. This data is divided into 2 bytes, first the lower one is transmitted, then the higher one.
- Type 1 packet text data will be transmitted in the language selected in the tester. Despite this, the value of the code page field is fixed.
- The data can be split by the tester and sent in more than 1 packet. This is not indicated anywhere in the packet, so it should be assumed that when a packet of the same type is received again, it is a continuation of the previous one.
- The manufacturer's software, when the tester is connected, sends a packet with the following content at fixed intervals: 0x5E,0x5E,0x0A,0x00,0x04,0x01,0xD1,0x30,0x0D,0x0A. Its format corresponds to a type 2 packet, but the tester does not send a response to it. It seems that this was an (unsuccessful) attempt to implement a heartbeat check.

## TODO
- [ ] Firmware update support.
- [ ] Saving the voltage waveform and battery parameters to a single PDF file.
- [ ] Voltage waveform printing.
- [ ] Battery parameters printing.
- [ ] Voltage waveform and battery parameters printing.

### For non-commercial use only.
