/*  
 * Copyright (c) 2011 University of Kansas
 * Author: Justin Rohrer <rohrej@ittc.ku.edu>
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

/*
 * Dự án mô phỏng mạng MANET sử dụng ns-3 với giao thức định tuyến AODV, sử dụng Flow Monitor để phân tích hiệu suất của giao thức và 
 * NetAnim để mô phỏng và quan sát quá trình giao tiếp giữa các node trong mạng MANET.
 */

/*Thêm các thư viện cần thiết cho dự án*/
#include "ns3/aodv-module.h"/*Thư viện hỗ trợ mô hình mạng AODV*/
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"/*Thư viện hỗ trợ flow monitor*/
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"  /*Thư viện hỗ trợ netanim*/
#include <fstream>
#include <iostream>

/* giúp mã nguồn ngắn gọn và dễ đọc hơn mà không cần
 * phải chỉ định không gian tên (namespace) của chúng mỗi lần*/
using namespace ns3;
using namespace dsr;

/*Khởi tạo ghi nhật ký (logging)*/
NS_LOG_COMPONENT_DEFINE("manet-routing-compare");

/*Khởi tạo lớp RoutingExperiment: chứa các phương thức và thuộc tính cần thiết để chạy mô phỏng,
 bao gồm cấu hình giao thức, theo dõi lưu lượng và ghi nhật ký hiệu suất.*/
class RoutingExperiment
{
public:
    RoutingExperiment();//Hàm khởi tạo
    void Run();//chạy chương trình
    void CommandSetup(int argc, char** argv);//thiết lập tham số đầu vào
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);//thiết lập một socket để nhận gói tin từ một địa chỉ IPv4 cụ thể trên một node.
    void ReceivePacket(Ptr<Socket> socket);//được gọi khi socket nhận được một gói tin.
    void CheckThroughput();//kiểm tra thông lượng (throughput) của mạng
private:
    uint32_t port{9};// Cổng để gửi nhận gói tin
    uint32_t bytesTotal{0};// Tổng số byte đã nhận
    uint32_t packetsReceived{0};// Tổng số gói tin đã nhận
    std::string m_CSVfileName{"manet-routing.output.DSDV.csv"};// Tên tệp CSV để lưu kết quả
    int m_nSinks{10};// Số lượng node nhận
    std::string m_protocolName{"AODV"};// Tên giao thức định tuyến đang sử dụng (AODV)
    double m_txp{7.5};// Công suất phát (Transmission Power)
    bool m_traceMobility{true};// Biến kiểm soát việc ghi lại thông tin di động
    bool m_flowMonitor{true};// Biến kiểm soát việc sử dụng FlowMonitor
};

RoutingExperiment::RoutingExperiment()
{
// Thiết lập mô phỏng, cấu hình các tham số mạng, và khởi động mô phỏng
}

static inline std::string

PrintReceivedPacket(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)

{
// Tạo một đối tượng ostringstream để xây dựng chuỗi đầu ra
    std::ostringstream oss;
    
// Thêm thời gian hiện tại của mô phỏng (tính bằng giây) và ID của node vào chuỗi
    oss << Simulator::Now().GetSeconds() << " " << socket->GetNode()->GetId();

 // Kiểm tra xem địa chỉ gửi có phải là địa chỉ kiểu InetSocketAddress không   
    if (InetSocketAddress::IsMatchingType(senderAddress))

    {
// Chuyển đổi địa chỉ gửi thành InetSocketAddress
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);

 // Thêm thông tin về địa chỉ IP của node gửi vào chuỗi       
        oss << " received one packet from " << addr.GetIpv4();

    }
    else
    {
// Nếu không phải địa chỉ kiểu InetSocketAddress, chỉ thông báo nhận gói tin mà không có địa chỉ cụ thể
        oss << " received one packet!";
    }
// Trả về chuỗi đã được xây dựng
    return oss.str();

}

void RoutingExperiment::ReceivePacket(Ptr<Socket> socket)// Hàm xử lý gói tin nhận từ socket

{

    Ptr<Packet> packet;// Khai báo con trỏ gói tin để lưu trữ gói tin nhận được

    Address senderAddress;// Địa chỉ của người gửi gói tin

    // Vòng lặp nhận gói tin từ socket cho đến khi không còn gói tin nào
    while ((packet = socket->RecvFrom(senderAddress)))// Nhận gói tin từ socket và lưu địa chỉ người gửi

    {

        bytesTotal += packet->GetSize();// Cập nhật tổng số byte đã nhận bằng kích thước gói tin

        packetsReceived += 1;// Tăng số lượng gói tin đã nhận lên 1
 // In ra thông tin về gói tin nhận được, bao gồm thời gian và địa chỉ người gửi
        NS_LOG_UNCOND(PrintReceivedPacket(socket, packet, senderAddress));

    }

}

void RoutingExperiment::CheckThroughput()// Hàm kiểm tra thông lượng

{

    double kbs = (bytesTotal * 8.0) / 1000;// Tính thông lượng (kilo bits per second) từ tổng số byte đã nhận

    bytesTotal = 0;// Đặt lại tổng số byte đã nhận về 0 sau khi ghi vào file


// Mở file CSV để ghi dữ liệu, sử dụng chế độ append để thêm dữ liệu vào cuối file
    std::ofstream out(m_CSVfileName, std::ios::app);
// Ghi thời gian hiện tại, thông lượng, số gói đã nhận, số lượng sinks, tên giao thức, và công suất truyền
    out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","

        << m_nSinks << "," << m_protocolName << "," << m_txp << std::endl;

    out.close();

    packetsReceived = 0;// Đặt lại số gói đã nhận về 0 sau khi ghi
// Lập lịch để gọi lại hàm CheckThroughput sau 1 giây
    Simulator::Schedule(Seconds(1.0), &RoutingExperiment::CheckThroughput, this);

}



Ptr<Socket> RoutingExperiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node)// Hàm thiết lập nhận gói tin

{
// Tìm kiếm TypeId của UdpSocketFactory để sử dụng cho socket
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
// Tạo một socket cho node sử dụng loại socket đã tìm thấy
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
// Tạo địa chỉ socket, kết hợp địa chỉ IPv4 với cổng đã xác định
    InetSocketAddress local = InetSocketAddress(addr, port);
// Ràng buộc socket với địa chỉ cục bộ
    sink->Bind(local);
// Thiết lập callback để xử lý khi có gói tin được nhận, liên kết với hàm ReceivePacket
    sink->SetRecvCallback(MakeCallback(&RoutingExperiment::ReceivePacket, this));
// Trả về socket đã tạo
    return sink;

}



void RoutingExperiment::CommandSetup(int argc, char** argv)// Hàm thiết lập các tham số đầu vào cho mô phỏng

{
// Tạo một đối tượng CommandLine, sử dụng tên file hiện tại
    CommandLine cmd(__FILE__);
// Thêm tham số đầu vào cho tên file CSV đầu ra
    cmd.AddValue("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
// Thêm tham số đầu vào để kích hoạt theo dõi di động
    cmd.AddValue("traceMobility", "Enable mobility tracing", m_traceMobility);
// Thêm tham số đầu vào cho giao thức định tuyến
    cmd.AddValue("protocol", "Routing protocol (OLSR, AODV, DSDV, DSR)", m_protocolName);
// Thêm tham số đầu vào để kích hoạt FlowMonitor
    cmd.AddValue("flowMonitor", "Enable FlowMonitor", m_flowMonitor);
// Phân tích các tham số đầu vào
    cmd.Parse(argc, argv);


 // Tạo một vector chứa các giao thức cho phép
    std::vector<std::string> allowedProtocols{"OLSR", "AODV", "DSDV", "DSR"};
// Kiểm tra xem giao thức đã cho có nằm trong danh sách cho phép hay không
    if (std::find(allowedProtocols.begin(), allowedProtocols.end(), m_protocolName) == allowedProtocols.end())

    {
// Nếu không có, thì dừng chương trình và thông báo lỗi

        NS_FATAL_ERROR("No such protocol:" << m_protocolName);

    }

}



int main(int argc, char* argv[])

{
// Tạo một đối tượng của lớp RoutingExperiment
    RoutingExperiment experiment;
// Gọi hàm CommandSetup để thiết lập các tham số đầu vào
    experiment.CommandSetup(argc, argv);
// Chạy mô phỏng với các tham số đã thiết lập
    experiment.Run();

    return 0;

}


//hàm chạy mô phỏng mạng
void RoutingExperiment::Run()

{
// Bật chế độ in thông tin gói dữ liệu
    Packet::EnablePrinting();


// Mở file CSV và ghi tiêu đề cho các cột
    std::ofstream out(m_CSVfileName);

    out << "SimulationSecond,ReceiveRate,PacketsReceived,NumberOfSinks,RoutingProtocol,TransmissionPower" << std::endl;

    out.close();

// Khai báo số lượng node WiFi, thời gian tổng mô phỏng, tốc độ truyền dữ liệu, v.v.

    int nWifis = 50;

    double TotalTime = 200.0;

    std::string rate("2048bps");

    std::string phyMode("DsssRate11Mbps");

    int nodeSpeed = 20;

    int nodePause = 0;


// Thiết lập kích thước gói và tốc độ dữ liệu cho ứng dụng OnOff
    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));

    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));


 // Tạo các node ad-hoc
    NodeContainer adhocNodes;

    adhocNodes.Create(nWifis);


// Khởi tạo WiFi với chuẩn 802.11b
    WifiHelper wifi;

    wifi.SetStandard(WIFI_STANDARD_80211b);

// Cấu hình kênh WiFi và thiết lập suy hao

    YansWifiPhyHelper wifiPhy;

    YansWifiChannelHelper wifiChannel;

    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    wifiPhy.SetChannel(wifiChannel.Create());

// Cấu hình MAC và thiết lập công suất truyền cho WiFi

    WifiMacHelper wifiMac;

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode), "ControlMode", StringValue(phyMode));

    wifiPhy.Set("TxPowerStart", DoubleValue(m_txp));

    wifiPhy.Set("TxPowerEnd", DoubleValue(m_txp));


// Thiết lập WiFi trong chế độ ad-hoc
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);


// Thiết lập mô hình di chuyển cho các node ad-hoc
    MobilityHelper mobilityAdhoc;

    ObjectFactory pos;

    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");

    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));

    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));



    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();

    std::stringstream ssSpeed;

    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";

    std::stringstream ssPause;

    ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";

    mobilityAdhoc.SetMobilityModel("ns3::RandomWaypointMobilityModel",

                                   "Speed", StringValue(ssSpeed.str()),

                                   "Pause", StringValue(ssPause.str()),

                                   "PositionAllocator", PointerValue(taPositionAlloc));

    mobilityAdhoc.SetPositionAllocator(taPositionAlloc);

    mobilityAdhoc.Install(adhocNodes);

// Cấu hình các giao thức định tuyến

    AodvHelper aodv;

    OlsrHelper olsr;

    DsdvHelper dsdv;

    DsrHelper dsr;

    DsrMainHelper dsrMain;

    Ipv4ListRoutingHelper list;

    InternetStackHelper internet;

// Thiết lập giao thức định tuyến theo tham số đầu vào

    if (m_protocolName == "OLSR")

    {

        list.Add(olsr, 100);

        internet.SetRoutingHelper(list);

        internet.Install(adhocNodes);

    }

    else if (m_protocolName == "AODV")

    {

        list.Add(aodv, 100);

        internet.SetRoutingHelper(list);

        internet.Install(adhocNodes);

    }

    else if (m_protocolName == "DSDV")

    {

        list.Add(dsdv, 100);

        internet.SetRoutingHelper(list);

        internet.Install(adhocNodes);

    }

    else if (m_protocolName == "DSR")

    {

        internet.Install(adhocNodes);

        dsrMain.Install(dsr, adhocNodes);

    }

// Thiết lập địa chỉ IP cho các node

    Ipv4AddressHelper addressAdhoc;

    addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer adhocInterfaces = addressAdhoc.Assign(adhocDevices);

// Cấu hình và cài đặt ứng dụng OnOff để tạo gói dữ liệu gửi đến các node nhận

    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());

    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));

    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));


// Tạo các ứng dụng OnOff cho mỗi node nhận
    for (int i = 0; i < m_nSinks; i++)

    {

        Ptr<Socket> sink = SetupPacketReceive(adhocInterfaces.GetAddress(i), adhocNodes.Get(i));

        AddressValue remoteAddress(InetSocketAddress(adhocInterfaces.GetAddress(i), port));

        onoff1.SetAttribute("Remote", remoteAddress);



        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();

        ApplicationContainer temp = onoff1.Install(adhocNodes.Get(i + m_nSinks));

        temp.Start(Seconds(var->GetValue(100.0, 101.0)));

        temp.Stop(Seconds(TotalTime));

    }


// Thiết lập mô phỏng đồ họa với NetAnim
    AnimationInterface anim("manet-routing-compare.xml");

// Cấu hình FlowMonitor nếu được yêu cầu

    FlowMonitorHelper flowmonHelper;

    Ptr<FlowMonitor> flowmon;

    if (m_flowMonitor)

    {

        flowmon = flowmonHelper.InstallAll();

    }


// Gọi hàm kiểm tra thông lượng theo chu kỳ
    CheckThroughput();

    Simulator::Stop(Seconds(TotalTime));

    Simulator::Run();


// Lưu kết quả FlowMonitor vào file
    if (m_flowMonitor)

    {

        flowmon->SerializeToXmlFile("manet-routing-compare.flowmon", false, false);

    }


// Kết thúc mô phỏng
    Simulator::Destroy();

}

