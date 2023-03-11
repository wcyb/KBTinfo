import sys
from enum import Enum

import serial
from serial.tools import list_ports as lp
import plotly.express as px
import pandas as pd


class PacketType(Enum):
    UNKNOWN = 0
    BATT_INFO = 1
    CHART = 2
    CHART_DISPLAY = 3


class Konnwei:
    fcs16_lookup = []
    initial_fcs = 0xffff
    good_fcs = 0xf0b8

    def __init__(self, com_port, com_port_baud):
        self.port = serial.Serial(com_port, com_port_baud, timeout=0)
        self.received_packet = None
        self.received_packet_type = PacketType.UNKNOWN
        self.received_data = []
        self.generate_fcs16_lookup_table()

    @staticmethod
    def add_commas(string):
        return ','.join(string.split(' '))

    @staticmethod
    def add_0x(string):
        return '0x' + ',0x'.join(string.split(','))

    def generate_fcs16_lookup_table(self):
        P = 0x8408
        for b in range(0, 256):
            v = b
            for _ in range(8, 0, -1):
                v = (v >> 1) ^ P if v & 1 else v >> 1
            self.fcs16_lookup.append(v & 0xffff)

    def calculate_checksum(self):
        packet_len = self.get_packet_len()
        current_fcs = self.initial_fcs
        checksum = 0
        for i in range(0, packet_len):
            last_fcs = current_fcs
            current_fcs = (current_fcs >> 8) ^ self.fcs16_lookup[(current_fcs ^ self.received_packet[i]) & 0xff]
            if i < packet_len - 2:  # omit fcs calculation for their custom checksum
                checksum = (last_fcs ^ self.received_packet[i]) << 16 | current_fcs
        return current_fcs, (checksum ^ 0xffff) & 0xffff

    def check_if_checksum_ok(self):
        packet_len = self.get_packet_len()
        trial_fcs, calculated_checksum = self.calculate_checksum()
        received_checksum = (self.received_packet[packet_len - 1] << 8) | self.received_packet[packet_len - 2]
        if trial_fcs == self.good_fcs and received_checksum == calculated_checksum:
            return True
        return False

    def get_packet_len(self):
        return ((self.received_packet[3] << 8) | self.received_packet[2]) - 2  # omit new line chars in packet len

    def get_packet_encoding(self):
        return (self.received_packet[6] << 8) | self.received_packet[5]

    def determine_packet_type(self):
        if not self.is_packet_batt_info():
            if not self.is_packet_chart():
                if not self.is_packet_chart_display():
                    return False  # unknown packet type
        if not self.check_if_packet_correct():
            return False  # packet is not correct
        return True  # packet type has been determined

    def is_packet_batt_info(self):
        header = [0x0, 0x24, 0x24, 0xff, 0xfe]
        if len(self.received_packet) < 9:
            return False
        for i in range(0, len(header)):
            if self.received_packet[i] != header[i]:
                return False
        self.received_packet_type = PacketType.BATT_INFO
        return True

    def is_packet_chart(self):
        header = [0x24, 0x24, 0, 0, 0xff, 0x01]
        if len(self.received_packet) < 10:
            return False
        for i in range(0, len(header)):
            if i == 2 or i == 3:  # second and third byte are packet length, so ignore them
                continue
            if self.received_packet[i] != header[i]:
                return False
        self.received_packet_type = PacketType.CHART
        return True

    def is_packet_chart_display(self):
        header = [0x24, 0x24, 0x0a, 0x00, 0xff, 0x02]
        if len(self.received_packet) < 10:
            return False
        for i in range(0, len(header)):
            if self.received_packet[i] != header[i]:
                return False
        self.received_packet_type = PacketType.CHART_DISPLAY
        return True

    def check_if_packet_correct(self):
        if self.received_packet_type is PacketType.UNKNOWN:
            return False
        if self.received_packet_type is PacketType.BATT_INFO:
            if not self.check_if_packet_have_trailer():
                return False
        if self.received_packet_type is PacketType.CHART or self.received_packet_type is PacketType.CHART_DISPLAY:
            if not self.check_if_packet_have_trailer() or not self.check_if_checksum_ok():
                return False
        return True

    def check_if_packet_have_trailer(self):
        if self.received_packet[-1] == 0x0a and self.received_packet[-2] == 0x0d:
            return True
        return False

    def get_batt_tester_info(self):
        pass

    def download_batt_tester_update(self):
        pass

    def update_batt_tester(self):
        pass


available_ports = lp.comports()
if len(available_ports) == 0:
    print("No COM ports available!")
    sys.exit()
print("Enter a name of a COM port used for communication with the device:")
for p in available_ports:
    print(p.name + ' (' + p.description + ')')
port = input("Name: ")
k = Konnwei(port, 115200)

times_checked = 0  # if received data doesn't match to any type of packet, then check the buffer again
packet_processed = False  # if data was processed, set to True to clear buffers
chart_data = []  # data to display
check_data = False  # if there is more than 1 packet in the buffer, then check and process the received data again
while True:
    if k.port.in_waiting or check_data:
        k.received_data.extend(k.port.readall())
        k.received_packet = k.received_data
        k.determine_packet_type()
        if k.received_packet_type == PacketType.UNKNOWN:
            times_checked += 1
            if times_checked == 3:
                k.received_data.clear()
                k.received_packet = None
                times_checked = 0

        if k.received_packet_type == PacketType.BATT_INFO:
            print(bytearray(k.received_packet[7:]).decode().replace(chr(0x7f), chr(0x03a9)))
            packet_processed = True
        if k.received_packet_type == PacketType.CHART:
            if len(k.received_data) < 10:
                continue
            for i in range(6, k.get_packet_len() - 2, 2):  # omit header, fcs and newline
                chart_data.append(((k.received_packet[i + 1] << 8) | k.received_packet[i]) / 10)
            k.received_data = k.received_data[k.get_packet_len() + 2:]  # delete data that was read from the buffer
            check_data = True  # check if we have another packet in the buffer
        if k.received_packet_type == PacketType.CHART_DISPLAY:
            time_span = [x * 10 / (len(chart_data) - 1) for x in range(len(chart_data))]  # or +0.0125 for each voltage sample
            df = pd.DataFrame({"time": time_span, "voltage": chart_data})
            fig = px.line(df, x="time", y="voltage", labels={"voltage": "Voltage [V]", "time": "Time [s]"})
            fig.update_layout(title={"text": "Battery voltage during cranking", 'x': 0.5, 'y': 0.96})
            fig.show()
            check_data = False
            packet_processed = True

        if packet_processed:
            k.received_data.clear()
            k.received_packet = None
            k.received_packet_type = PacketType.UNKNOWN
            chart_data.clear()
            packet_processed = False
