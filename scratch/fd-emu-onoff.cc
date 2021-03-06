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
// both hosts: $ sudo chown root.root build/src/fd-net-device/ns3-dev-raw-sock-creator
// both hosts: $ sudo chmod 4755 build/src/fd-net-device/ns3-dev-raw-sock-creator
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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/fd-net-device-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("EmuFdNetDeviceSaturationExample");

int 
main (int argc, char *argv[])
{
  uint16_t sinkPort1 = 8000;
  uint16_t sinkPort2 = 8080;
  uint16_t sinkPort3 = 8081;
  //uint16_t sinkPort4 = 80812

  uint32_t packetSize = 10000; // bytes
  std::string dataRate("1000Mb/s");
  bool ap1Mode = false;
  bool ap2Mode = false;
  bool userMode = false;

  std::string deviceName ("veth-ns5");
  std::string gatewway ("10.1.1.1");
  std::string ap1 ("10.1.1.2");
  std::string ap2 ("10.1.1.3");
  std::string user ("10.1.1.4");
  std::string netmask ("255.255.255.0");
  std::string macgatewway ("00:00:00:00:00:01");
  std::string macap1 ("00:00:00:00:00:02");
  std::string macap2 ("00:00:00:00:00:03");
  std::string macUser ("00:00:00:00:00:04");


  CommandLine cmd;
  cmd.AddValue ("deviceName", "Device name", deviceName);
  cmd.AddValue ("gatewway", "Local IP address (dotted decimal only please)", gatewway);
  cmd.AddValue ("ap1", "Remote IP address (dotted decimal only please)", ap1);
  cmd.AddValue ("ap2", "Remote IP address (dotted decimal only please)", ap2);
  cmd.AddValue ("user", "Remote IP address (dotted decimal only please)", user);
  cmd.AddValue ("localmask", "Local mask address (dotted decimal only please)", netmask);
  cmd.AddValue ("ap1Mode", "1:true, 0:false, default gatewway", ap1Mode);
  cmd.AddValue ("ap2Mode", "1:true, 0:false, default gatewway", ap2Mode);
  cmd.AddValue ("userMode", "1:true, 0:false, default gatewway", userMode);
  cmd.AddValue ("mac-gatewway", "Mac Address for ap gatewway : 00:00:00:00:00:01", macgatewway);
  cmd.AddValue ("mac-ap1", "Mac Address for ap Default : 00:00:00:00:00:02", macap1);
  cmd.AddValue ("mac-ap2", "Mac Address for ap Default : 00:00:00:00:00:03", macap2);
  cmd.AddValue ("mac-user", "Mac Address for ap Default : 00:00:00:00:00:04", macUser);

  cmd.AddValue ("data-rate", "Data rate defaults to 1000Mb/s", dataRate);
  cmd.Parse (argc, argv);

  Ipv4Address remoteIp;
  Ipv4Address remoteIp1;
  Ipv4Address remoteIp2;
  Ipv4Address remoteIp4;
  Ipv4Address localIp;
  Mac48AddressValue localMac;
  
  if (ap1Mode)
  {
     remoteIp = Ipv4Address (gatewway.c_str ());
     remoteIp4 = Ipv4Address (user.c_str ());
     localIp = Ipv4Address (ap1.c_str ());
     localMac = Mac48AddressValue (macap1.c_str ());
  }
  else if (ap2Mode)
  {
     remoteIp = Ipv4Address (gatewway.c_str ());
     remoteIp4 = Ipv4Address (user.c_str ());
     localIp = Ipv4Address (ap2.c_str ());
     localMac = Mac48AddressValue (macap2.c_str ());
  }
  else if(userMode)
  {
     remoteIp = Ipv4Address (ap1.c_str ());
     localIp = Ipv4Address (user.c_str ());
     localMac = Mac48AddressValue (macUser.c_str ());
  }

  else
  {
     remoteIp1 = Ipv4Address (ap1.c_str ());
     remoteIp2 = Ipv4Address (ap2.c_str ());
     localIp = Ipv4Address (gatewway.c_str ());
     localMac =  Mac48AddressValue (macgatewway.c_str ());
  }

  Ipv4Mask localMask (netmask.c_str ());
  
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NS_LOG_INFO ("Create Node");
  Ptr<Node> node = CreateObject<Node> ();

  NS_LOG_INFO ("Create Device");
  EmuFdNetDeviceHelper emu;
  emu.SetDeviceName (deviceName);
  NetDeviceContainer devices = emu.Install (node);
  Ptr<NetDevice> device = devices.Get (0);
  device->SetAttribute ("Address", localMac);

  NS_LOG_INFO ("Add Internet Stack");
  InternetStackHelper internetStackHelper;
  internetStackHelper.SetIpv4StackInstall(true);
  internetStackHelper.Install (node);

  NS_LOG_INFO ("Create IPv4 Interface");
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  uint32_t interface = ipv4->AddInterface (device);
  Ipv4InterfaceAddress address = Ipv4InterfaceAddress (localIp, localMask);
  ipv4->AddAddress (interface, address);
  ipv4->SetMetric (interface, 1);
  ipv4->SetUp (interface);

  if(ap1Mode)
  {
    Address sinkLocalAddress (InetSocketAddress (localIp, sinkPort1));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install (node);
    sinkApp.Start (Seconds (1.0));
    sinkApp.Stop (Seconds (12.0));

    AddressValue remoteAddress1 (InetSocketAddress (remoteIp4, sinkPort3));
    OnOffHelper onoff1 ("ns3::TcpSocketFactory", Address ());
    onoff1.SetAttribute ("Remote", remoteAddress1);
    onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff1.SetAttribute ("DataRate", DataRateValue (dataRate));
    onoff1.SetAttribute ("PacketSize", UintegerValue (packetSize));

    ApplicationContainer gatewwayApps1 = onoff1.Install (node);
    gatewwayApps1.Start (Seconds (2.0));
    gatewwayApps1.Stop (Seconds (12.0));
    
    emu.EnablePcap ("fd-ap1", device);
  }
   else if(ap2Mode)
  {
    Address sinkLocalAddress (InetSocketAddress (localIp, sinkPort2));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install (node);
    sinkApp.Start (Seconds (1.0));
    sinkApp.Stop (Seconds (12.0));


    // ptr<socket> sk = TcpFactory->CreateSocket();
    // sk ->Bind (InetSocketAddress(sinkPort3));
    // sk ->SendTo (InetSocketAddress (Ipv4Address ("10.0.0.1"),80),)
    AddressValue remoteAddress1 (InetSocketAddress (remoteIp4, sinkPort3));
    OnOffHelper onoff1 ("ns3::TcpSocketFactory", Address ());
    onoff1.SetAttribute ("Remote", remoteAddress1);
    onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff1.SetAttribute ("DataRate", DataRateValue (dataRate));
    onoff1.SetAttribute ("PacketSize", UintegerValue (packetSize));

    ApplicationContainer gatewwayApps1 = onoff1.Install (node);
    gatewwayApps1.Start (Seconds (2.0));
    gatewwayApps1.Stop (Seconds (12.0));
    
    emu.EnablePcap ("fd-ap2", device);
  }
  else if(userMode)
  {
    Address sinkLocalAddress (InetSocketAddress (localIp, sinkPort3));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install (node);
    sinkApp.Start (Seconds (1.0));
    sinkApp.Stop (Seconds (12.0));
    
    emu.EnablePcap ("fd-user", device);
  }
  else
  {
    AddressValue remoteAddress1 (InetSocketAddress (remoteIp1, sinkPort1));
    OnOffHelper onoff1 ("ns3::TcpSocketFactory", Address ());
    onoff1.SetAttribute ("Remote", remoteAddress1);
    onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff1.SetAttribute ("DataRate", DataRateValue (dataRate));
    onoff1.SetAttribute ("PacketSize", UintegerValue (packetSize));

    ApplicationContainer gatewayApps1 = onoff1.Install (node);

    AddressValue remoteAddress2 (InetSocketAddress (remoteIp2, sinkPort2));
    OnOffHelper onoff2 ("ns3::TcpSocketFactory", Address ());
    onoff2.SetAttribute ("Remote", remoteAddress2);
    onoff2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff2.SetAttribute ("DataRate", DataRateValue (dataRate));
    onoff2.SetAttribute ("PacketSize", UintegerValue (packetSize));

    ApplicationContainer gatewayApps2 = onoff2.Install (node);
     gatewayApps2.Start (Seconds (2.0));
     gatewayApps1.Start (Seconds (2.0));
     gatewayApps2.Stop (Seconds (6.0));
     gatewayApps1.Stop (Seconds (6.0));

    emu.EnablePcap ("fd-gatewway", device);
  }

   LogComponentEnable("OnOffApplication", LOG_LEVEL_ALL);
      LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
   


  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

