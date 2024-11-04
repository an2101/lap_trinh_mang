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

NS_LOG_COMPONENT_DEFINE("manet-routing-compare");

class RoutingExperiment

{

public:

    RoutingExperiment();

    void Run();

    void CommandSetup(int argc, char** argv);

    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);

    void ReceivePacket(Ptr<Socket> socket);

    void CheckThroughput();



private:

    uint32_t port{9};

    uint32_t bytesTotal{0};

    uint32_t packetsReceived{0};

    std::string m_CSVfileName{"manet-routing.output.DSDV.csv"};

    int m_nSinks{10};

    std::string m_protocolName{"DSDV"};

    double m_txp{7.5};

    bool m_traceMobility{true};

    bool m_flowMonitor{true};

};



RoutingExperiment::RoutingExperiment()

{

}



static inline std::string

PrintReceivedPacket(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)

{

    std::ostringstream oss;

    oss << Simulator::Now().GetSeconds() << " " << socket->GetNode()->GetId();

    if (InetSocketAddress::IsMatchingType(senderAddress))

    {

        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);

        oss << " received one packet from " << addr.GetIpv4();

    }

    else

    {

        oss << " received one packet!";

    }

    return oss.str();

}



void RoutingExperiment::ReceivePacket(Ptr<Socket> socket)

{

    Ptr<Packet> packet;

    Address senderAddress;

    while ((packet = socket->RecvFrom(senderAddress)))

    {

        bytesTotal += packet->GetSize();

        packetsReceived += 1;

        NS_LOG_UNCOND(PrintReceivedPacket(socket, packet, senderAddress));

    }

}



void RoutingExperiment::CheckThroughput()

{

    double kbs = (bytesTotal * 8.0) / 1000;

    bytesTotal = 0;



    std::ofstream out(m_CSVfileName, std::ios::app);

    out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","

        << m_nSinks << "," << m_protocolName << "," << m_txp << std::endl;

    out.close();

    packetsReceived = 0;

    Simulator::Schedule(Seconds(1.0), &RoutingExperiment::CheckThroughput, this);

}



Ptr<Socket> RoutingExperiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node)

{

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

    Ptr<Socket> sink = Socket::CreateSocket(node, tid);

    InetSocketAddress local = InetSocketAddress(addr, port);

    sink->Bind(local);

    sink->SetRecvCallback(MakeCallback(&RoutingExperiment::ReceivePacket, this));

    return sink;

}



void RoutingExperiment::CommandSetup(int argc, char** argv)

{

    CommandLine cmd(__FILE__);

    cmd.AddValue("CSVfileName", "The name of the CSV output file name", m_CSVfileName);

    cmd.AddValue("traceMobility", "Enable mobility tracing", m_traceMobility);

    cmd.AddValue("protocol", "Routing protocol (OLSR, AODV, DSDV, DSR)", m_protocolName);

    cmd.AddValue("flowMonitor", "Enable FlowMonitor", m_flowMonitor);

    cmd.Parse(argc, argv);



    std::vector<std::string> allowedProtocols{"OLSR", "AODV", "DSDV", "DSR"};

    if (std::find(allowedProtocols.begin(), allowedProtocols.end(), m_protocolName) == allowedProtocols.end())

    {

        NS_FATAL_ERROR("No such protocol:" << m_protocolName);

    }

}



int main(int argc, char* argv[])

{

    RoutingExperiment experiment;

    experiment.CommandSetup(argc, argv);

    experiment.Run();

    return 0;

}



void RoutingExperiment::Run()

{

    Packet::EnablePrinting();



    std::ofstream out(m_CSVfileName);

    out << "SimulationSecond,ReceiveRate,PacketsReceived,NumberOfSinks,RoutingProtocol,TransmissionPower" << std::endl;

    out.close();



    int nWifis = 50;

    double TotalTime = 200.0;

    std::string rate("2048bps");

    std::string phyMode("DsssRate11Mbps");

    int nodeSpeed = 20;

    int nodePause = 0;



    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));

    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));



    NodeContainer adhocNodes;

    adhocNodes.Create(nWifis);



    WifiHelper wifi;

    wifi.SetStandard(WIFI_STANDARD_80211b);



    YansWifiPhyHelper wifiPhy;

    YansWifiChannelHelper wifiChannel;

    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    wifiPhy.SetChannel(wifiChannel.Create());



    WifiMacHelper wifiMac;

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode), "ControlMode", StringValue(phyMode));

    wifiPhy.Set("TxPowerStart", DoubleValue(m_txp));

    wifiPhy.Set("TxPowerEnd", DoubleValue(m_txp));



    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);



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



    AodvHelper aodv;

    OlsrHelper olsr;

    DsdvHelper dsdv;

    DsrHelper dsr;

    DsrMainHelper dsrMain;

    Ipv4ListRoutingHelper list;

    InternetStackHelper internet;



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



    Ipv4AddressHelper addressAdhoc;

    addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer adhocInterfaces = addressAdhoc.Assign(adhocDevices);



    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());

    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));

    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));



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



    AnimationInterface anim("manet-routing-compare.xml");



    FlowMonitorHelper flowmonHelper;

    Ptr<FlowMonitor> flowmon;

    if (m_flowMonitor)

    {

        flowmon = flowmonHelper.InstallAll();

    }



    CheckThroughput();

    Simulator::Stop(Seconds(TotalTime));

    Simulator::Run();



    if (m_flowMonitor)

    {

        flowmon->SerializeToXmlFile("manet-routing-compare.flowmon", false, false);

    }



    Simulator::Destroy();

}

