/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 The Boeing Company
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
 * Author: Lionel Metongnon <lionel.metongnon@uclouvain.be>
 */

#include "sixlowpan-network.h"
#include "ns3/core-module.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixlowpanNetwork");


SixlowpanNetwork::SixlowpanNetwork()
: Network(1)
{
  NS_LOG_FUNCTION (this);
}

SixlowpanNetwork::SixlowpanNetwork(uint32_t nbrOfNodes)
: Network(nbrOfNodes)
{
  NS_LOG_FUNCTION (this);
}

void 
SixlowpanNetwork::SetMobility(std::vector<double> xy)
{
  NS_LOG_FUNCTION (this);
  m_mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (xy[0]),
                                 "MinY", DoubleValue (xy[1]),
                                 "DeltaX", DoubleValue (xy[2]),
                                 "DeltaY", DoubleValue (xy[2]),
                                 "GridWidth", UintegerValue (1),
                                 "LayoutType", StringValue ("ColumnFirst"));
  m_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  m_mobility.Install (NodeContainer (m_router));

  m_mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-5000, 5000, -5000, 5000)));
  m_mobility.Install (m_nodes);
}

void 
SixlowpanNetwork::ConfigureL2() 
{
  NS_LOG_FUNCTION (this);
  NetDeviceContainer lrwpanDevices = m_lrWpanHelper.Install(NodeContainer (m_router, m_nodes));

  // Fake PAN association and short address assignment.
  m_lrWpanHelper.AssociateToPan (lrwpanDevices, 0);

  // NS_LOG_INFO ("Add 6LoWPAN support to lrwpanDevices and create sixlowpanDevices");
  SixLowPanHelper sixlowpan;
  m_netDevices = sixlowpan.Install (lrwpanDevices); 
}

void
SixlowpanNetwork::EnablePcap(bool promiscious)
{
  NS_LOG_FUNCTION (this << promiscious);
  m_lrWpanHelper.EnablePcapAll ("6LowPan", promiscious);
} 

SixlowpanNetwork::~SixlowpanNetwork()
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3