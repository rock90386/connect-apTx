  /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */ 
/*  
 * Copyright (c) 2012 University of Washington, 2012 INRIA  
 *  
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License version 2 as  
 * published by the Free Software Foundation; 
 *  
 * This program is distributed in the hope that it will be useful,  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 * GNU General Public License for more details. 
 *  
 * You should have received a copy of the GNU General Public License  
 * along with this program; if not, write to the Free Software  
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  
 *  
 * Author: Alina Quereilhac <alina.quereilhac@inria.fr> 
 *  
 */ 
// +----------------------+     +-----------------------+ 
// |      client host     |     |      server host      | 
// +----------------------+     +-----------------------+ 
// |     ns-3 Node 0      |     |      ns-3 Node 1      | 
// |  +----------------+  |     |   +----------------+  | 
// |  |    ns-3 TCP    |  |     |   |    ns-3 TCP    |  | 
// |  +----------------+  |     |   +----------------+  | 
// |  |    ns-3 IPv4   |  |     |   |    ns-3 IPv4   |  | 
// |  +----------------+  |     |   +----------------+  | 
// |  |   FdNetDevice  |  |     |   |   FdNetDevice  |  | 
// |  |    10.1.1.1    |  |     |   |    10.1.1.2    |  | 
// |  +----------------+  |     |   +----------------+  | 
// |  |   raw socket   |  |     |   |   raw socket   |  | 
// |  +----------------+  |     |   +----------------+  | 
// |       | eth0 |       |     |        | eth0 |       | 
// +-------+------+-------+     +--------+------+-------+ 
//  
//         10.1.1.11                     10.1.1.12  
//  
//             |                            | 
//             +----------------------------+ 
//  
// This example is aimed at measuring the throughput of the FdNetDevice 
// when using the EmuFdNetDeviceHelper. This is achieved by saturating  
// the channel with TCP traffic. Then the throughput can be obtained from   
// the generated .pcap files. 
//  
// To run this example you will need two hosts (client & server). 
// Steps to run the experiment: 
//  
// 1 - Connect the 2 computers with an Ethernet cable.  
// 2 - Set the IP addresses on both Ethernet devices. 
//  
// client machine: $ sudo ip addr add dev eth0 10.1.1.11/24 
// server machine: $ sudo ip addr add dev eth0 10.1.1.12/24 
//  
// 3 - Set both Ethernet devices to promiscuous mode. 
//  
// both machines: $ sudo ip link set eth0 promisc on  
//  
// 4 - Give root suid to the raw socket creator binary. 
//     If the --enable-sudo option was used to configure ns-3 with waf, then the following  
//     step will not be necessary.  
//  
// both hosts: $ sudo chown root.root build/src/fd-net-device/ns3.30.1-raw-sock-creator-debug  
// both hosts: $ sudo chmod 4755 build/src/fd-net-device/ns3.30.1-raw-sock-creator-debug  
//  
// 5 - Run the server side: 
//  
// server host: $ ./waf --run="fd-emu-onoff --serverMode=1" 
//  
// 6 - Run the client side: 
//        
// client host: $ ./waf --run="fd-emu-onoff"  
//  
#include <iostream> 
#include <fstream>  
#include <vector> 
#include <string> 
#include <cassert>  
#include "ns3/core-module.h"  
#include "ns3/network-module.h" 
#include "ns3/internet-module.h"  
#include "ns3/applications-module.h"  
#include "ns3/config-store-module.h"  
#include "ns3/fd-net-device-module.h" 
#include "ns3/ipv4-header.h"



using namespace ns3;  
NS_LOG_COMPONENT_DEFINE ("EmuFdNetDeviceSaturationExample");  
std::vector<Ptr<Packet>> m_buffer;  
bool serverMode = false;  
uint32_t rePacketCount = 0; 
uint32_t *p_rePacketCount = &rePacketCount; 
int i=0;  
  uint8_t buffer[100] = {0};  
  const uint8_t *p_buffer = buffer; 
template<typename T>  
void pop_front(std::vector<T>& vec) 
{ 
    assert(!vec.empty()); 
    vec.front() = std::move(vec.back());  
    vec.pop_back(); 
    //std::cout<<"don't pop?!!!"<<vec.size()<<std::endl;  
} 
static void SendPacket (Ptr<Socket> socket, uint32_t pktSize,   
                        uint32_t pktCount, Time pktInterval )  
{ 
  //std::cout<<buffer[0]<<std::endl;  
  if (pktCount > 0) 
    { 
      if(serverMode == 1) 
      { 
        if(m_buffer.size()!=0){ 
        //std::cout<<"start transmit back"<<std::endl;  
        socket->Send( m_buffer.front());  
        pop_front(m_buffer);  
      Simulator::Schedule (pktInterval, &SendPacket,  
                           socket, pktSize,rePacketCount, pktInterval);  
      } 
      else  
      std::cout<<"m_buffer empty"<<std::endl; 
      } 
      else  
      { 
        std::cout<<"transmit gogo"<<std::endl;  
        socket->Send (Create<Packet> (p_buffer,pktSize)); 
        Simulator::Schedule (pktInterval, &SendPacket,  
                           socket, pktSize,pktCount - 1, pktInterval);  
      } 
    } 
  else  
    { 
      socket->Close (); 
    } 
} 

static void SendPacketToNextHost (Ptr<Socket> socket, uint32_t pktSize,   
                        uint32_t pktCount, Time pktInterval )  
{ 
  //std::cout<<buffer[0]<<std::endl; 
  
  if (pktCount > 0) 
    { 
         Ipv4Header ipv4hd;
        if(m_buffer.size()!=0){ 
        std::cout<<"start transmit to next host"<<std::endl;
          Ptr<Packet> packet=m_buffer.front();
           packet->RemoveHeader(ipv4hd);

        socket->Send( packet);  
        pop_front(m_buffer);  
      Simulator::Schedule (pktInterval, &SendPacketToNextHost,  
                           socket, pktSize,m_buffer.size(), pktInterval);  
      } 
      else  
      std::cout<<"m_buffer empty"<<std::endl; 
      
    } 
  else  
    { 
      socket->Close (); 
    } 
} 

void ReceivePacket (Ptr<Socket> socket) 
{ 
  NS_LOG_INFO ("Received one packet!"); 
  Ptr<Packet> packet = socket->Recv (); 
  // std::cout<<packet->GetSize()<<std::endl; 
  m_buffer.push_back(packet); 
  rePacketCount+=1; 
   std::cout<<"size:"<<m_buffer.size()<<std::endl; 
  SocketIpTosTag tosTag;  
  if (packet->RemovePacketTag (tosTag)) 
    { 
      NS_LOG_INFO (" TOS = " << (uint32_t)tosTag.GetTos ());  
    } 
  SocketIpTtlTag ttlTag;  
  if (packet->RemovePacketTag (ttlTag)) 
    { 
      NS_LOG_INFO (" TTL = " << (uint32_t)ttlTag.GetTtl ());  
    } 
} 
int   
main (int argc, char *argv[]) 
{ 
  uint16_t sinkPort = 8000; 
  uint16_t sumePort = 7000; 
  uint16_t TxPort = 8800; 
  uint32_t packetSize = 1024; // bytes 
  std::string dataRate("1Gb/s");  
  //bool serverMode = false;  
  uint32_t ipTos = 0; 
  bool ipRecvTos = true;  
  uint32_t ipTtl = 0; 
  bool ipRecvTtl = true;  
  uint32_t packetCount = 3;  
  // uint32_t packetCount = 117261;  
  //   double packetInterval = 8.528e-6; 
double packetInterval = 0.1; 
 // uint32_t *p_packetCount = &packetCount; 

  std::string deviceName_1 ("enxd037453b9273"); 
    std::string deviceName_2 ("eno1");

  std::string client ("10.1.1.1");  
  std::string sume ("10.1.1.3"); //net fpga
  std::string tosume ("10.1.1.4"); //from ns3 to net fpga
  std::string server ("10.1.1.2");  
  std::string netmask ("255.255.255.0");  
  std::string macSume ("00:18:3e:02:36:c2");//sume port 0 
  std::string macgetfromSume ("1c:69:7a:03:a4:ca");//eno1 mac 
  std::string macServer ("d0:37:45:3b:93:04");  
  std::string macClient ("d0:37:45:3b:92:73");  //enxd037453b9273 mac
  //std::string macfpga ("54:b2:03:08:19:91"); 
  CommandLine cmd;  
  cmd.AddValue ("deviceName_1", "Device name", deviceName_1); 
  cmd.AddValue ("deviceName_2", "Device name", deviceName_2); 
  cmd.AddValue ("client", "Local IP address (dotted decimal only please)", client); 
  cmd.AddValue ("tosume", "Local IP address (dotted decimal only please)", tosume);   
  cmd.AddValue ("sume", "Local IP address (dotted decimal only please)", sume); 
  cmd.AddValue ("server", "Remote IP address (dotted decimal only please)", server);  
  cmd.AddValue ("localmask", "Local mask address (dotted decimal only please)", netmask); 
  cmd.AddValue ("serverMode", "1:true, 0:false, default client", serverMode); 
  cmd.AddValue ("mac-sume", "Mac Address for Server Client : 00:18:3e:02:36:c2", macSume); 
  cmd.AddValue ("mac-getfromsume", "Mac Address for Server Client : 54:b2:03:08:19:90", macgetfromSume);
  cmd.AddValue ("mac-client", "Mac Address for Server Client : 00:00:00:00:00:02", macClient);
  cmd.AddValue ("mac-server", "Mac Address for Server Default : 00:00:00:00:00:01", macServer); 
  cmd.AddValue ("data-rate", "Data rate defaults to 1000Mb/s", dataRate); 
    cmd.AddValue ("PacketSize", "Packet size in bytes", packetSize);  
  cmd.AddValue ("PacketCount", "Number of packets to send", packetCount); 
    cmd.AddValue ("rePacketCount", "Number of packets to send", rePacketCount); 
  cmd.AddValue ("Interval", "Interval between packets", packetInterval);  
  cmd.AddValue ("IP_TOS", "IP_TOS", ipTos); 
  cmd.AddValue ("IP_RECVTOS", "IP_RECVTOS", ipRecvTos); 
  cmd.AddValue ("IP_TTL", "IP_TTL", ipTtl); 
  cmd.AddValue ("IP_RECVTTL", "IP_RECVTTL", ipRecvTtl); 
  cmd.Parse (argc, argv); 
  Ipv4Address remoteIp; 
  Ipv4Address localIp;  
  Ipv4Address sendtosumeIp;  
  Ipv4Address sumeIp;
  //Ipv4Address localIprecvback;  
  Mac48AddressValue localMac; 
  Mac48AddressValue localtosumeMac;
  //Mac48AddressValue localMacrecvback; 
    
  for(int i=0;i<100;i++)  
  { 
    buffer[i]=120;  
  } 
   // LogComponentEnable ("FdNetDevice", LOG_FUNCTION); 
   // LogComponentEnable ("FdNetDevice", LOG_DEBUG);  
   // LogComponentEnable ("FdNetDevice", LOG_PREFIX_TIME);  
   //    LogComponentEnable ("FdNetDevice", LOG_LOGIC); 
   //    LogComponentEnable ("Ipv4RawSocketImpl", LOG_FUNCTION); 
   // LogComponentEnable ("Ipv4RawSocketImpl", LOG_DEBUG);  
   // LogComponentEnable ("Ipv4RawSocketImpl", LOG_LOGIC); 
   //  LogComponentEnable ("ArpCache", LOG_FUNCTION); 
   // LogComponentEnable ("ArpCache", LOG_DEBUG);  
    LogComponentEnable ("EmuFdNetDeviceSaturationExample", LOG_INFO); 
  if (serverMode) 
  { 
     remoteIp = Ipv4Address (client.c_str ());  
     localIp = Ipv4Address (server.c_str ()); 
     localMac = Mac48AddressValue (macServer.c_str ()); 
  } 
  else  
  { 
     remoteIp = Ipv4Address (server.c_str ());  
     localIp = Ipv4Address (client.c_str ()); 
     sumeIp = Ipv4Address (sume.c_str ()); 
     sendtosumeIp = Ipv4Address (tosume.c_str ());  
     localMac =  Mac48AddressValue (macClient.c_str ());  
     localtosumeMac =  Mac48AddressValue (macgetfromSume.c_str ()); 
  } 
  Ipv4Mask localMask (netmask.c_str ());  
    
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));  
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true)); 
  

  NS_LOG_INFO ("Create Node");  
  Ptr<Node> node1 = CreateObject<Node> (); 
  Ptr<Node> node2 = CreateObject<Node> (); 

  NS_LOG_INFO ("Create Device");
  EmuFdNetDeviceHelper emu1;//to next ns3
  emu1.SetDeviceName (deviceName_1);
  NetDeviceContainer devices1 = emu1.Install (node1);
  Ptr<NetDevice> device1 = devices1.Get (0);
  device1->SetAttribute ("Address", localMac);

  EmuFdNetDeviceHelper emu2;//to sume
  emu2.SetDeviceName (deviceName_2);
  NetDeviceContainer devices2 = emu2.Install (node2);
  Ptr<NetDevice> device2 = devices2.Get (0);
  device2->SetAttribute ("Address", localtosumeMac);

  NS_LOG_INFO ("Add Internet Stack");
  InternetStackHelper internetStackHelper;
  internetStackHelper.SetIpv4StackInstall(true);
  internetStackHelper.Install (node1);
  internetStackHelper.Install (node2);

  NS_LOG_INFO ("Create IPv4 Interface");
  Ptr<Ipv4> ipv41 = node1->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv42 = node2->GetObject<Ipv4> ();
  uint32_t interface1 = ipv41->AddInterface (device1);
  uint32_t interface2 = ipv42->AddInterface (device2);
  Ipv4InterfaceAddress address1 = Ipv4InterfaceAddress (localIp, localMask);
  Ipv4InterfaceAddress address2 = Ipv4InterfaceAddress (sendtosumeIp, localMask);
  ipv41->AddAddress (interface1, address1);
  ipv42->AddAddress (interface2, address2);
  ipv41->SetMetric (interface1, 1);
  ipv42->SetMetric (interface2, 1);
  ipv41->SetUp (interface1);
  ipv42->SetUp (interface2);
 
  if(serverMode)  
  { 
    // Address sinkLocalAddress (InetSocketAddress (localIp, sinkPort));  
    // PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress); 
    // ApplicationContainer sinkApp = sinkHelper.Install (node);  
    // sinkApp.Start (Seconds (1.0)); 
    // sinkApp.Stop (Seconds (60.0)); 
      
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");  
  Ptr<Socket> source = Socket::CreateSocket (node1, tid);  
  InetSocketAddress remote = InetSocketAddress (remoteIp, TxPort);  
  source->Connect (remote); 
  Time interPacketInterval = Seconds (packetInterval);  
  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), 
                                  Seconds (3.0), &SendPacket,   
                                  source, packetSize, rePacketCount, interPacketInterval); 
  Ptr<Socket> recvSink = Socket::CreateSocket (node1, tid);  
  InetSocketAddress local =InetSocketAddress (localIp, sinkPort); 
  recvSink->SetIpRecvTos (ipRecvTos); 
  recvSink->SetIpRecvTtl (ipRecvTtl); 
  recvSink->Bind (local); 
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));  
    //std::cout<<recvSink<<std::endl; 
    // emu1.EnablePcap ("fd-server", device1); 
  } 
  else  
  { 
    // AddressValue remoteAddress (InetSocketAddress (remoteIp, sinkPort)); 
    // OnOffHelper onoff ("ns3::TcpSocketFactory", Address ()); 
    // onoff.SetAttribute ("Remote", remoteAddress);  
    // onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));  
    // onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]")); 
    // onoff.SetAttribute ("DataRate", DataRateValue (dataRate)); 
    // onoff.SetAttribute ("PacketSize", UintegerValue (packetSize)); 
    // ApplicationContainer clientApps = onoff.Install (node);  
     TypeId tid = TypeId::LookupByName ("ns3::Ipv4RawSocketFactory"); 
  Ptr<Socket> sourcetosume = Socket::CreateSocket (node2, tid);   //to sume
  InetSocketAddress remotetosume = InetSocketAddress (sumeIp, sumePort);  
  sourcetosume->Connect (remotetosume); 
   Time interPacketInterval = Seconds (packetInterval);  
  Simulator::ScheduleWithContext (sourcetosume->GetNode ()->GetId (), 
                                  Seconds (1.0), &SendPacket,   
                                  sourcetosume, packetSize, packetCount, interPacketInterval);  


  
  Ptr<Socket> source = Socket::CreateSocket (node1, tid);   //to next ns3
  InetSocketAddress remote = InetSocketAddress (remoteIp, sinkPort);  
  source->Connect (remote); 
  
  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), 
                                  Seconds (1.5), &SendPacketToNextHost,   
                                  source, packetSize, packetCount, interPacketInterval);  

  std::cout<<"Send Packet"<<std::endl;  
   Ptr<Socket> recvSink = Socket::CreateSocket (node2, tid);  
  InetSocketAddress local =InetSocketAddress (sumeIp, sumePort);  
  recvSink->SetIpRecvTos (ipRecvTos);  
  recvSink->SetIpRecvTtl (ipRecvTtl);  
  recvSink->Bind (local);  
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket)); 
    // clientApps.Start (Seconds (4.0));  
    // clientApps.Stop (Seconds (58.0));  
    // emu1.EnablePcap ("fd-client", device1); 
    emu2.EnablePcap ("fd-tosume", device2); 
  } 
// LogComponentEnable("OnOffApplication", LOG_LEVEL_ALL); 
//       LogComponentEnable("PacketSink", LOG_LEVEL_ALL); 
      // LogComponentEnable("ArpL3Protocol", LOG_FUNCTION);  
      //  LogComponentEnable("ArpL3Protocol", LOG_DEBUG); 
  Simulator::Stop (Seconds (50)); 
  Simulator::Run ();  
  Simulator::Destroy ();  
  return 0; 
}