/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
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
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
*/

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/fq-codel-queue-disc.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-packet-filter.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-packet-filter.h"
#include "ns3/ipv6-queue-disc-item.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

using namespace ns3;

// Variable to assign hash to a new packet's flow
int32_t hash;

/**
 * Simple test packet filter able to classify IPv4 packets
 *
 */
class Ipv4TestPacketFilter : public Ipv4PacketFilter {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  Ipv4TestPacketFilter ();
  virtual ~Ipv4TestPacketFilter ();

private:
  virtual int32_t DoClassify (Ptr<QueueDiscItem> item) const;
  virtual bool CheckProtocol (Ptr<QueueDiscItem> item) const;
};

TypeId
Ipv4TestPacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4TestPacketFilter")
    .SetParent<Ipv4PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<Ipv4TestPacketFilter> ()
  ;
  return tid;
}

Ipv4TestPacketFilter::Ipv4TestPacketFilter ()
{
}

Ipv4TestPacketFilter::~Ipv4TestPacketFilter ()
{
}

int32_t
Ipv4TestPacketFilter::DoClassify (Ptr<QueueDiscItem> item) const
{
  return hash;
}

bool
Ipv4TestPacketFilter::CheckProtocol (Ptr<QueueDiscItem> item) const
{
  return true;
}

/**
 * This class tests packets for which there is no suitable filter
 */
class FqCoDelQueueDiscNoSuitableFilter : public TestCase
{
public:
  FqCoDelQueueDiscNoSuitableFilter ();
  virtual ~FqCoDelQueueDiscNoSuitableFilter ();

private:
  virtual void DoRun (void);
};

FqCoDelQueueDiscNoSuitableFilter::FqCoDelQueueDiscNoSuitableFilter ()
  : TestCase ("Test packets that are not classified by any filter")
{
}

FqCoDelQueueDiscNoSuitableFilter::~FqCoDelQueueDiscNoSuitableFilter ()
{
}

void
FqCoDelQueueDiscNoSuitableFilter::DoRun (void)
{
  // Packets that cannot be classified by the available filters should be dropped
  Ptr<FqCoDelQueueDisc> queueDisc = CreateObjectWithAttributes<FqCoDelQueueDisc> ("MaxSize", StringValue ("4p"));
  Ptr<Ipv4TestPacketFilter> filter = CreateObject<Ipv4TestPacketFilter> ();
  queueDisc->AddPacketFilter (filter);

  hash = -1;
  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ptr<Packet> p;
  p = Create<Packet> ();
  Ptr<Ipv6QueueDiscItem> item;
  Ipv6Header ipv6Header;
  Address dest;
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 0, "no flow queue should have been created");

  p = Create<Packet> (reinterpret_cast<const uint8_t*> ("hello, world"), 12);
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 0, "no flow queue should have been created");

  Simulator::Destroy ();
}

/**
 * This class tests the IP flows separation and the packet limit
 */
class FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit : public TestCase
{
public:
  FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit ();
  virtual ~FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header hdr);
};

FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit::FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit ()
  : TestCase ("Test IP flows separation and packet limit")
{
}

FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit::~FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit ()
{
}

void
FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit::AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit::DoRun (void)
{
  Ptr<FqCoDelQueueDisc> queueDisc = CreateObjectWithAttributes<FqCoDelQueueDisc> ("MaxSize", StringValue ("4p"));

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");

  // Add two packets from the second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.7"));
  // Add the first packet
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the flow queue");
  // Add the second packet that causes two packets to be dropped from the fat flow (max backlog = 300, threshold = 150)
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the deficit per flow
 */
class FqCoDelQueueDiscDeficit : public TestCase
{
public:
  FqCoDelQueueDiscDeficit ();
  virtual ~FqCoDelQueueDiscDeficit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header hdr);
};

FqCoDelQueueDiscDeficit::FqCoDelQueueDiscDeficit ()
  : TestCase ("Test credits and flows status")
{
}

FqCoDelQueueDiscDeficit::~FqCoDelQueueDiscDeficit ()
{
}

void
FqCoDelQueueDiscDeficit::AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
FqCoDelQueueDiscDeficit::DoRun (void)
{
  Ptr<FqCoDelQueueDisc> queueDisc = CreateObjectWithAttributes<FqCoDelQueueDisc> ();

  queueDisc->SetQuantum (90);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add a packet from the first flow
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  Ptr<FqCoDelFlow> flow1 = StaticCast<FqCoDelFlow> (queueDisc->GetQueueDiscClass (0));
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the first flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCoDelFlow::NEW_FLOW, "the first flow must be in the list of new queues");
  // Dequeue a packet
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  // the deficit for the first flow becomes 90 - (100+20) = -30
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), -30, "unexpected deficit for the first flow");

  // Add two packets from the first flow
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCoDelFlow::NEW_FLOW, "the first flow must still be in the list of new queues");

  // Add two packets from the second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the second flow queue");
  Ptr<FqCoDelFlow> flow2 = StaticCast<FqCoDelFlow> (queueDisc->GetQueueDiscClass (1));
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the second flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCoDelFlow::NEW_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the second flow, as the first flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  // the first flow got a quantum of deficit (-30+90=60) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 60, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCoDelFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow has a negative deficit (-30) and is still in the list of new queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), -30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCoDelFlow::NEW_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the first flow, as the second flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  // the first flow has a negative deficit (60-(100+20)= -60) and stays in the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), -60, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCoDelFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow got a quantum of deficit (-30+90=60) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), 60, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCoDelFlow::OLD_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the second flow, as the first flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the second flow queue");
  // the first flow got a quantum of deficit (-60+90=30) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 30, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCoDelFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow has a negative deficit (60-(100+20)= -60)
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), -60, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCoDelFlow::OLD_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the first flow, as the second flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the second flow queue");
  // the first flow has a negative deficit (30-(100+20)= -90)
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), -90, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCoDelFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow got a quantum of deficit (-60+90=30) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), 30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCoDelFlow::OLD_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet
  queueDisc->Dequeue ();
  // the first flow is at the head of the list of old queues but has a negative deficit, thus it gets a quantun
  // of deficit (-90+90=0) and is moved to the end of the list of old queues. Then, the second flow (which has a
  // positive deficit) is selected, but the second flow is empty and thus it is set to inactive. The first flow is
  // reconsidered, but it has a null deficit, hence it gets another quantum of deficit (0+90=90). Then, the first
  // flow is reconsidered again, now it has a positive deficit and hence it is selected. But, it is empty and
  // therefore is set to inactive, too.
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 90, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCoDelFlow::INACTIVE, "the first flow must be inactive");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), 30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCoDelFlow::INACTIVE, "the second flow must be inactive");

  Simulator::Destroy ();
}

/**
 * This class tests the TCP flows separation
 */
class FqCoDelQueueDiscTCPFlowsSeparation : public TestCase
{
public:
  FqCoDelQueueDiscTCPFlowsSeparation ();
  virtual ~FqCoDelQueueDiscTCPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr);
};

FqCoDelQueueDiscTCPFlowsSeparation::FqCoDelQueueDiscTCPFlowsSeparation ()
  : TestCase ("Test TCP flows separation")
{
}

FqCoDelQueueDiscTCPFlowsSeparation::~FqCoDelQueueDiscTCPFlowsSeparation ()
{
}

void
FqCoDelQueueDiscTCPFlowsSeparation::AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (tcpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
FqCoDelQueueDiscTCPFlowsSeparation::DoRun (void)
{
  Ptr<FqCoDelQueueDisc> queueDisc = CreateObjectWithAttributes<FqCoDelQueueDisc> ("MaxSize", StringValue ("10p"));

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (6);

  TcpHeader tcpHdr;
  tcpHdr.SetSourcePort (7);
  tcpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  tcpHdr.SetSourcePort (8);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  tcpHdr.SetDestinationPort (28);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  tcpHdr.SetSourcePort (7);
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the UDP flows separation
 */
class FqCoDelQueueDiscUDPFlowsSeparation : public TestCase
{
public:
  FqCoDelQueueDiscUDPFlowsSeparation ();
  virtual ~FqCoDelQueueDiscUDPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr);
};

FqCoDelQueueDiscUDPFlowsSeparation::FqCoDelQueueDiscUDPFlowsSeparation ()
  : TestCase ("Test UDP flows separation")
{
}

FqCoDelQueueDiscUDPFlowsSeparation::~FqCoDelQueueDiscUDPFlowsSeparation ()
{
}

void
FqCoDelQueueDiscUDPFlowsSeparation::AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (udpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
FqCoDelQueueDiscUDPFlowsSeparation::DoRun (void)
{
  Ptr<FqCoDelQueueDisc> queueDisc = CreateObjectWithAttributes<FqCoDelQueueDisc> ("MaxSize", StringValue ("10p"));

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (17);

  UdpHeader udpHdr;
  udpHdr.SetSourcePort (7);
  udpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  udpHdr.SetSourcePort (8);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  udpHdr.SetDestinationPort (28);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  udpHdr.SetSourcePort (7);
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

/*
 * This class tests linear probing capability, collision response, and set
 * creation capability of SetAssociative hashing in FqCodel SetAssociative
 * hash. We modified DoClassify and CheckProtocol so that we could control
 * the hash returned for each packet. In the beginning, we use flow hashes
 * ranging from 0 to 7. These must go into different queues in the same set. 
 * The set number is obtained by m_flowsIndices[0] which is 0. When a new 
 * packet comes in with flow hash 1024, because 1024 % 1024 = 0, 
 * m_flowsIndices[0] = 0 is obtained and the first set is iteratively searched.
 * The packet is added to queue 0 since the tag of the queues in the set 
 * doesn't match with the hash of the flow, and the tag of the queue is 
 * updated. When a packet with hash 1025 arrives, m_flowsIndices[0] = 0
 * is obtained (because 1025 % 1024 = 1) and the first set is iteratively
 * searched. Since there is no match, it is added to queue 0 and the tag is
 * updated.
 *
 * The variable outerHash stores the nearest multiple of 8 that is lesser than
 * the hash. When a flow hash of 20 arrives, the outerHash corresponding to 20
 * is 16, and since m_flowIndices[16] wasn’t previously allotted, a new set of
 * eight queues are created, and m_flowsIndices[16] is set to be 8 (since there
 * are queues 0-7 previously set). After creating eight queues 8-15, insert the
 * packet into the first queue in this set.
*/

class FqCoDelQueueDiscSetLinearProbing : public TestCase
{
public:
  FqCoDelQueueDiscSetLinearProbing ();
  virtual ~FqCoDelQueueDiscSetLinearProbing ();
private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header hdr);
};

FqCoDelQueueDiscSetLinearProbing::FqCoDelQueueDiscSetLinearProbing ()
    : TestCase ("Test credits and flows status")
{
}

FqCoDelQueueDiscSetLinearProbing::~FqCoDelQueueDiscSetLinearProbing ()
{
}

void
FqCoDelQueueDiscSetLinearProbing::AddPacket (Ptr<FqCoDelQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
FqCoDelQueueDiscSetLinearProbing::DoRun (void)
{
  Ptr<FqCoDelQueueDisc> queueDisc = CreateObjectWithAttributes<FqCoDelQueueDisc> ("SetAssociativeHash", BooleanValue (true));
  queueDisc->SetQuantum (90);
  queueDisc->Initialize ();

  Ptr<Ipv4TestPacketFilter> filter = CreateObject<Ipv4TestPacketFilter> ();
  queueDisc->AddPacketFilter (filter);

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  hash = 0;
  AddPacket (queueDisc, hdr);
  hash = 1;
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  hash = 2;
  AddPacket (queueDisc, hdr);
  hash = 3;
  AddPacket (queueDisc, hdr);
  hash = 4;
  AddPacket (queueDisc, hdr);
  hash = 4;
  AddPacket (queueDisc, hdr);
  hash = 5;
  AddPacket (queueDisc, hdr);
  hash = 6;
  AddPacket (queueDisc, hdr);
  hash = 7;
  AddPacket (queueDisc, hdr);
  hash = 1024;
  AddPacket (queueDisc, hdr);

  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 11,
                         "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2,
                         "unexpected number of packets in the first flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2,
                         "unexpected number of packets in the second flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the third flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the fourth flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetNPackets (), 2,
                         "unexpected number of packets in the fifth flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (5)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the sixth flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (6)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the seventh flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (7)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the eighth flow queue of set one");
  hash = 1025;
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3,
                         "unexpected number of packets in the first flow of set one");
  hash = 10;
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (8)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the first flow of set two");
  Simulator::Destroy ();
}

class FqCoDelQueueDiscTestSuite : public TestSuite
{
public:
  FqCoDelQueueDiscTestSuite ();
};

FqCoDelQueueDiscTestSuite::FqCoDelQueueDiscTestSuite ()
  : TestSuite ("fq-codel-queue-disc", UNIT)
{
  AddTestCase (new FqCoDelQueueDiscNoSuitableFilter, TestCase::QUICK);
  AddTestCase (new FqCoDelQueueDiscIPFlowsSeparationAndPacketLimit, TestCase::QUICK);
  AddTestCase (new FqCoDelQueueDiscDeficit, TestCase::QUICK);
  AddTestCase (new FqCoDelQueueDiscTCPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new FqCoDelQueueDiscUDPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new FqCoDelQueueDiscSetLinearProbing, TestCase::QUICK);
}

static FqCoDelQueueDiscTestSuite fqCoDelQueueDiscTestSuite;
