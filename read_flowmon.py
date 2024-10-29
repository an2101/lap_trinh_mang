import xml.etree.ElementTree as ET



def read_flowmon_file(file_path):

    # Phân tích file XML

    tree = ET.parse(file_path)

    root = tree.getroot()



    print("Flow Stats:")

    # Duyệt qua các flow trong FlowStats

    for flow in root.find('FlowStats').findall('Flow'):

        flow_id = flow.get('flowId')

        time_first_tx = flow.get('timeFirstTxPacket')

        time_first_rx = flow.get('timeFirstRxPacket')

        time_last_tx = flow.get('timeLastTxPacket')

        time_last_rx = flow.get('timeLastRxPacket')

        tx_packets = flow.get('txPackets')

        rx_packets = flow.get('rxPackets')

        lost_packets = flow.get('lostPackets')



        print(f"\nFlow ID: {flow_id}")

        print(f"Time First Tx Packet: {time_first_tx}")

        print(f"Time First Rx Packet: {time_first_rx}")

        print(f"Time Last Tx Packet: {time_last_tx}")

        print(f"Time Last Rx Packet: {time_last_rx}")

        print(f"Transmitted Packets: {tx_packets}")

        print(f"Received Packets: {rx_packets}")

        print(f"Lost Packets: {lost_packets}")



    print("\nIPv4 Flow Classifier:")

    # Duyệt qua các flow trong Ipv4FlowClassifier

    for flow in root.find('Ipv4FlowClassifier').findall('Flow'):

        flow_id = flow.get('flowId')

        src_address = flow.get('sourceAddress')

        dst_address = flow.get('destinationAddress')

        protocol = flow.get('protocol')



        print(f"\nFlow ID: {flow_id}")

        print(f"Source Address: {src_address}")

        print(f"Destination Address: {dst_address}")

        print(f"Protocol: {protocol}")



if __name__ == "__main__":

    file_path = "manet-routing-compare.flowmon"  # Thay đổi đường dẫn nếu cần

    read_flowmon_file(file_path)

