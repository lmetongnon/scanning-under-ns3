/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"

#include "scan-tools.h"
#include "penetration-tools.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ScanToolsApplication");

NS_OBJECT_ENSURE_REGISTERED (ScanTools);

TypeId
ScanTools::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ScanTools")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<ScanTools> ()
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   // TimeValue (Seconds (1.0)),
                   TimeValue (Time ("2ms")),
                   MakeTimeAccessor (&ScanTools::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("MaxRange", 
                   "The maximum number of hosts to scan per Network",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ScanTools::m_count),
                   MakeUintegerChecker<uint32_t> ())
    // .AddAttribute ("NetworkAddress", 
    //                "The Network list we want to scan",
    //                Ipv6AddressValue (),
    //                MakeIpv6AddressAccessor (&ScanTools::m_networkAddress),
    //                MakeIpv6AddressChecker ())
    // .AddAttribute ("TargetNetwork", 
    //                "The Network list we want to scan",
    //                Ipv6AddressValue (),
    //                MakeIpv6AddressAccessor (&ScanTools::m_targetNetwork),
    //                MakeIpv6AddressChecker ())
    // .AddAttribute ("Prefix", 
    //                "The destination Address of the outbound packets",
    //                Ipv6PrefixValue (),
    //                MakeIpv6PrefixAccessor (&ScanTools::m_prefix),
    //                MakeIpv6PrefixChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (80),
                   MakeUintegerAccessor (&ScanTools::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ScanTools::SetDataSize,
                                         &ScanTools::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&ScanTools::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

ScanTools::ScanTools ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
}

ScanTools::~ScanTools()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_targetedAddresses.clear ();
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void 
ScanTools::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
ScanTools::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
ScanTools::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  GenerateAddresses ();
  // StartScanning ();
}

void
ScanTools::SetTargetedNetworks (
      std::map<Ipv6Address, 
      Ipv6Prefix> &targetedNetworks)
{
  NS_LOG_FUNCTION (this);
  m_targetedNetworks = targetedNetworks;
}

void
ScanTools::Scanning ()
{
  NS_LOG_FUNCTION (this);
  if (!m_targetedAddresses.empty ())
    {
      m_peerAddress = m_targetedAddresses.front ();
      m_targetedAddresses.pop_front ();
      if (m_socket == 0)
        {
          TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
          m_socket = Socket::CreateSocket (GetNode (), tid);
          m_socket->Bind6();
        }
      m_socket->Connect (Inet6SocketAddress (m_peerAddress, m_peerPort));
      m_socket->SetRecvCallback (MakeCallback (&ScanTools::HandleRead, this));

      ScheduleTransmit (Seconds (m_interval));
    }
}

void
ScanTools::GenerateAddresses ()
{
  NS_LOG_FUNCTION (this);
  Ipv6AddressList ipv6AddressList = Ipv6AddressList();
  Ipv6AddressList ipv6AddressList6LoWPAN = Ipv6AddressList();
  for (std::map<Ipv6Address, Ipv6Prefix>::iterator i = m_targetedNetworks.begin(); i != m_targetedNetworks.end(); ++i)
    {
      ipv6AddressList.Init (i->first, i->second, Ipv6Address ("::200:ff:fe00:1"));
      ipv6AddressList6LoWPAN.Init (i->first, i->second, Ipv6Address ("::ff:fe00:1"));
      for (uint32_t k = 0; k < m_count; ++k)
        {
          m_targetedAddresses.push_back (ipv6AddressList6LoWPAN.NextAddress (i->second));
          m_targetedAddresses.push_back (ipv6AddressList.NextAddress (i->second));
        }
    }
    // for (std::list<Ipv6Address>::iterator i = m_targetedAddresses.begin(); i != m_targetedAddresses.end(); ++i)
    //   {
    //     NS_LOG_INFO (*i);
    //   }
    Scanning ();
}

void 
ScanTools::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  // Retrieve the penetration application
  // Ptr<Application> application = GetNode()->GetApplication(1);
  // Ptr<PenetrationTools> penetrationTool = DynamicCast<PenetrationTools>(application);
  for (std::list<Ipv6Address>::iterator i = m_victimAddresses.begin(); i != m_victimAddresses.end(); ++i)
    {
      NS_LOG_INFO (*i);
    }
    Ptr<PenetrationTools> penetrationTool = GetNode()->GetApplication(1)->GetObject <PenetrationTools>();
    penetrationTool->StartPenetration (m_victimAddresses);

  //Create a callback to alert penetration tool a the end of the scanning
  // Callback<void, std::list<Ipv6Address> > callback = MakeCallback (&PenetrationTools::StartPenetration, &penetrationTool);
  // Callback<void, uint32_t > callback = MakeCallback (&PenetrationTools::SetDataSize, &penetrationTool);
  // callback (ipv6AddressList.GetTargetAddressesList ()); 
  // ipv6AddressList.GetTargetAddressesList ();
}

void 
ScanTools::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the attacker is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t 
ScanTools::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
ScanTools::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << "20ms");
  // m_sendEvent = Simulator::Schedule (dt, &ScanTools::Send, this);
  m_sendEvent = Simulator::Schedule (Time ("20ms"), &ScanTools::Send, this);
  // NS_LOG_INFO (Time ("20ms"));
}

void 
ScanTools::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "ScanTools::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "ScanTools::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the attacker has indicated that it doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding attribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  m_socket->Send (p);

  ++m_sent;

  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s attacker sent " << m_size << " bytes to " <<
               m_peerAddress << " port " << m_peerPort);

  if (!m_targetedAddresses.empty ()) 
    {
      Scanning ();
    }
}
void
ScanTools::AddToTargetList (Ipv6Address victimAddress)
{
  NS_LOG_FUNCTION (this << victimAddress);
  m_victimAddresses.push_back (victimAddress);
}

void
ScanTools::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  if ((packet = socket->RecvFrom (from)))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s attacker received " << packet->GetSize () << " bytes from " <<
                   Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                   Inet6SocketAddress::ConvertFrom (from).GetPort ());
      AddToTargetList (Inet6SocketAddress::ConvertFrom (from).GetIpv6 ());
    }
}

} // Namespace ns3