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

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "fq-codel-queue-disc.h"
#include "codel-queue-disc.h"
#include "ns3/net-device-queue-interface.h"
#include "unordered_set"

uint32_t n_flows = 0;
uint32_t n_collisions = 0;
uint32_t filled_queues = 0;

std::unordered_set<uint32_t> flowSet;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FqCoDelQueueDisc");
uint32_t tags[1024];

NS_OBJECT_ENSURE_REGISTERED (FqCoDelFlow);

TypeId
FqCoDelFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqCoDelFlow")
                          .SetParent<QueueDiscClass> ()
                          .SetGroupName ("TrafficControl")
                          .AddConstructor<FqCoDelFlow> ();
  return tid;
}

FqCoDelFlow::FqCoDelFlow () : m_deficit (0), m_status (INACTIVE)
{
  // NS_LOG_FUNCTION (this);
}

FqCoDelFlow::~FqCoDelFlow ()
{
  // NS_LOG_FUNCTION (this);
}

void
FqCoDelFlow::SetDeficit (uint32_t deficit)
{
  // NS_LOG_FUNCTION (this << deficit);
  m_deficit = deficit;
}

int32_t
FqCoDelFlow::GetDeficit (void) const
{
  // NS_LOG_FUNCTION (this);
  return m_deficit;
}

void
FqCoDelFlow::IncreaseDeficit (int32_t deficit)
{
  // NS_LOG_FUNCTION (this << deficit);
  m_deficit += deficit;
}

void
FqCoDelFlow::SetStatus (FlowStatus status)
{
  // NS_LOG_FUNCTION (this);
  m_status = status;
}

FqCoDelFlow::FlowStatus
FqCoDelFlow::GetStatus (void) const
{
  // NS_LOG_FUNCTION (this);
  return m_status;
}

NS_OBJECT_ENSURE_REGISTERED (FqCoDelQueueDisc);

TypeId
FqCoDelQueueDisc::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::FqCoDelQueueDisc")
          .SetParent<QueueDisc> ()
          .SetGroupName ("TrafficControl")
          .AddConstructor<FqCoDelQueueDisc> ()
          .AddAttribute ("Interval", "The CoDel algorithm interval for each FQCoDel queue",
                         StringValue ("100ms"), MakeStringAccessor (&FqCoDelQueueDisc::m_interval),
                         MakeStringChecker ())
          .AddAttribute ("Target", "The CoDel algorithm target queue delay for each FQCoDel queue",
                         StringValue ("5ms"), MakeStringAccessor (&FqCoDelQueueDisc::m_target),
                         MakeStringChecker ())
          .AddAttribute ("MaxSize", "The maximum number of packets accepted by this queue disc",
                         QueueSizeValue (QueueSize ("10240p")),
                         MakeQueueSizeAccessor (&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                         MakeQueueSizeChecker ())
          .AddAttribute ("Flows",
                         "The number of queues into which the incoming packets are classified",
                         UintegerValue (1024), MakeUintegerAccessor (&FqCoDelQueueDisc::m_flows),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("DropBatchSize", "The maximum number of packets dropped from the fat flow",
                         UintegerValue (64),
                         MakeUintegerAccessor (&FqCoDelQueueDisc::m_dropBatchSize),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute (
              "Perturbation",
              "The salt used as an additional input to the hash function used to classify packets",
              UintegerValue (0), MakeUintegerAccessor (&FqCoDelQueueDisc::m_perturbation),
              MakeUintegerChecker<uint32_t> ())
          .AddAttribute (
              "SetAssociativity", "Variable to use setassociativity hash", BooleanValue (false),
              MakeBooleanAccessor (&FqCoDelQueueDisc::m_setAssociativity), MakeBooleanChecker ());
  return tid;
}

FqCoDelQueueDisc::FqCoDelQueueDisc ()
    : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS), m_quantum (0)
{
  // NS_LOG_FUNCTION (this);
}

FqCoDelQueueDisc::~FqCoDelQueueDisc ()
{
  // NS_LOG_FUNCTION (this);
}

void
FqCoDelQueueDisc::SetQuantum (uint32_t quantum)
{
  // NS_LOG_FUNCTION (this << quantum);
  m_quantum = quantum;
}

uint32_t
FqCoDelQueueDisc::GetQuantum (void) const
{
  return m_quantum;
}
/*
* For set associative when the set is filled a new flow arrives to the same set then it is a collision. 
* For the regular hash if a flow hashes into a queue which already contains a flow then it is a collision. 
* For set associative a new flow is determined if the flow does not match with any of the tags in the set
* For the regular hash a new flow is determined if the flow does not match with the tag of the queue it hashes to
* For the regular hash if a flow hashes into a queue which already contains a flow then it is a collision. 
* n_collision variable holds the values of collision
* n_flows variable holds the value for the number of new flows 
*/

bool
FqCoDelQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  // NS_LOG_FUNCTION (this << item);
  if (n_flows % 100 == 0)
    {
      std::cout << n_flows << " " << n_collisions << " " << filled_queues << "\n" ;
    }
  if (n_flows >= 2000)
    {
      exit (0);
    }
  if (m_setAssociativity)
    {
      uint32_t h = 0;
      uint32_t innerHash, outerHash;
      uint32_t flowHash;
      uint32_t set_ways = 8;

      if (GetNPacketFilters () == 0)
        {
          // NS_LOG_DEBUG("InsideFirstIF");
          flowHash = item->Hash (m_perturbation);
          h = (flowHash % m_flows);
          innerHash = h % set_ways;
          outerHash = h - innerHash;
        }
      else
        {

          int32_t ret = Classify (item);

          if (ret != PacketFilter::PF_NO_MATCH)
            {
              // NS_LOG_DEBUG("InsideIF");
              flowHash = ret;
              h = ret % m_flows;
              innerHash = h % set_ways;
              outerHash = h - innerHash;
            }
          else
            {
              // NS_LOG_DEBUG("InsideElse");
              NS_LOG_ERROR ("No filter has been able to classify this packet, drop it.");
              DropBeforeEnqueue (item, UNCLASSIFIED_DROP);
              return false;
            }
        }

      Ptr<FqCoDelFlow> flow;
      if (m_flowsIndices.find (outerHash) == m_flowsIndices.end ())
        {
          // NS_LOG_DEBUG ("Creating a new flow queue with index " << h << "; flow index "
          //                                                    << m_flowsIndices[outerHash] << " index of queue " << i - m_flowsIndices[outerHash]);
          for (uint32_t i = 0; i < 8; i++)
            {
              flow = m_flowFactory.Create<FqCoDelFlow> ();
              Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
              qd->Initialize ();
              flow->SetQueueDisc (qd);
              AddQueueDiscClass (flow);
            }
          m_flowsIndices[outerHash] = GetNQueueDiscClasses () - 8;
          flow = StaticCast<FqCoDelFlow> (GetQueueDiscClass (m_flowsIndices[outerHash]));
          flow->SetStatus (FqCoDelFlow::NEW_FLOW);
          flow->SetDeficit (m_quantum);
          m_newFlows.push_back (flow);
          tags[outerHash] = flowHash;
          flow->GetQueueDisc ()->Enqueue (item);

          // collision code added here
          flowSet.insert (flowHash);
          n_flows++;
          filled_queues++;

          // NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index "
          // << m_flowsIndices[outerHash] << " index of queue " << 0);
          if (GetCurrentSize () > GetMaxSize ())
            {
              FqCoDelDrop ();
            }
        }
      else
        {
          uint32_t i;
          bool flag = false;
          for (i = m_flowsIndices[outerHash]; i < m_flowsIndices[outerHash] + 8; i++)
            {
              // NS_LOG_DEBUG ("i inside " << i << " flowHash " << flowHash << " something " <<tags[outerHash + i - m_flowsIndices[outerHash]]);
              flow = StaticCast<FqCoDelFlow> (GetQueueDiscClass (i));

              if (tags[outerHash + i - m_flowsIndices[outerHash]] == flowHash ||
                  flow->GetStatus () == FqCoDelFlow::INACTIVE)
                {
                  if (flow->GetStatus () == FqCoDelFlow::INACTIVE)
                    {
                      flow->SetStatus (FqCoDelFlow::NEW_FLOW);
                      flow->SetDeficit (m_quantum);
                      m_newFlows.push_back (flow);
                      
                      // collision code added here
                      flowSet.insert (flowHash);
                      n_flows++;
                      filled_queues++;
                    }
                  flow->GetQueueDisc ()->Enqueue (item);
                  tags[outerHash + i - m_flowsIndices[outerHash]] = flowHash;
                  flag = true;
                  // NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index "
                  // << m_flowsIndices[outerHash] << " index of queue " << i - m_flowsIndices[outerHash]);

                  if (GetCurrentSize () > GetMaxSize ())
                    {
                      FqCoDelDrop ();
                    }
                  break;
                }
            }

          if (flag == false)
            {
              flow = StaticCast<FqCoDelFlow> (GetQueueDiscClass (m_flowsIndices[outerHash]));
              flow->GetQueueDisc ()->Enqueue (item);
              n_collisions+=1;
              if(flowSet.find(flowHash)==flowSet.end()){
                n_flows++;
                flowSet.insert(flowHash);
              }
              tags[outerHash + i] = flowHash;
              // NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index "
              // << m_flowsIndices[outerHash]);

              // collision code added here

              if (GetCurrentSize () > GetMaxSize ())
                {
                  FqCoDelDrop ();
                }
            }
        }
    }
  else
    {
      uint32_t h = 0;
      uint32_t flowHash;
      uint32_t tags[1024];
      if (GetNPacketFilters () == 0)
        {
          flowHash = item->Hash (m_perturbation);
          h = flowHash % m_flows;
        }
      else
        {
          int32_t ret = Classify (item);
          // NS_LOG_DEBUG ("DoClassify " << ret);

          if (ret != PacketFilter::PF_NO_MATCH)
            {
              flowHash = ret;
              h = flowHash % m_flows;
              // NS_LOG_DEBUG ("Inside IF PF_NO_MATCH " << h);
            }
          else
            {
              // NS_LOG_ERROR ("No filter has been able to classify this packet, drop it.");
              DropBeforeEnqueue (item, UNCLASSIFIED_DROP);
              return false;
            }
        }
      Ptr<FqCoDelFlow> flow;
      if (m_flowsIndices.find (h) == m_flowsIndices.end ())
        {
          // NS_LOG_DEBUG ("Creating a new flow queue with index " << h);
          flow = m_flowFactory.Create<FqCoDelFlow> ();
          Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
          qd->Initialize ();
          flow->SetQueueDisc (qd);
          AddQueueDiscClass (flow);
          flowSet.insert (flowHash);
          m_flowsIndices[h] = GetNQueueDiscClasses () - 1;
          tags[h] = flowHash;
          n_flows++;
          filled_queues++;
        }
      else
        {
          if (tags[h] != flowHash)
            {
              n_collisions++;
              if(flowSet.find(flowHash)==flowSet.end()){
                n_flows++;
                flowSet.insert(flowHash);
              }
              tags[h] = flowHash;
            }
          flow = StaticCast<FqCoDelFlow> (GetQueueDiscClass (m_flowsIndices[h]));
        }

      if (flow->GetStatus () == FqCoDelFlow::INACTIVE)
        {
          flow->SetStatus (FqCoDelFlow::NEW_FLOW);
          flow->SetDeficit (m_quantum);
          m_newFlows.push_back (flow);
          flowSet.insert (flowHash);
        }

      flow->GetQueueDisc ()->Enqueue (item);

      // NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index " << m_flowsIndices[h]);

      if (GetCurrentSize () > GetMaxSize ())
        {
          FqCoDelDrop ();
        }
    }

  return true;
}


Ptr<QueueDiscItem>
FqCoDelQueueDisc::DoDequeue (void)
{
  // NS_LOG_FUNCTION (this);

  Ptr<FqCoDelFlow> flow;
  Ptr<QueueDiscItem> item;

  do
    {
      bool found = false;

      while (!found && !m_newFlows.empty ())
        {
          flow = m_newFlows.front ();

          if (flow->GetDeficit () <= 0)
            {
              flow->IncreaseDeficit (m_quantum);
              flow->SetStatus (FqCoDelFlow::OLD_FLOW);
              m_oldFlows.push_back (flow);
              m_newFlows.pop_front ();
            }
          else
            {
              //NS_LOG_DEBUG ("Found a new flow with positive deficit");
              found = true;
            }
        }

      while (!found && !m_oldFlows.empty ())
        {
          flow = m_oldFlows.front ();

          if (flow->GetDeficit () <= 0)
            {
              flow->IncreaseDeficit (m_quantum);
              m_oldFlows.push_back (flow);
              m_oldFlows.pop_front ();
            }
          else
            {
              // NS_LOG_DEBUG ("Found an old flow with positive deficit");
              found = true;
            }
        }

      if (!found)
        {
          // NS_LOG_DEBUG ("No flow found to dequeue a packet");
          return 0;
        }

      item = flow->GetQueueDisc ()->Dequeue ();

      if (!item)
        {
          // NS_LOG_DEBUG ("Could not get a packet from the selected flow queue");
          if (!m_newFlows.empty ())
            {
              flow->SetStatus (FqCoDelFlow::OLD_FLOW);
              m_oldFlows.push_back (flow);
              m_newFlows.pop_front ();
            }
          else
            {
              flow->SetStatus (FqCoDelFlow::INACTIVE);
              m_oldFlows.pop_front ();
            }
        }
      else
        {
          // NS_LOG_DEBUG ("Dequeued packet " << item->GetPacket ());
        }
    }
  while (item == 0);

  flow->IncreaseDeficit (item->GetSize () * -1);

  return item;
}

bool
FqCoDelQueueDisc::CheckConfig (void)
{
  // NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("FqCoDelQueueDisc cannot have classes");
      return false;
    }

  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("FqCoDelQueueDisc cannot have internal queues");
      return false;
    }

  // we are at initialization time. If the user has not set a quantum value,
  // set the quantum to the MTU of the device (if any)
  if (!m_quantum)
    {
      Ptr<NetDeviceQueueInterface> ndqi = GetNetDeviceQueueInterface ();
      Ptr<NetDevice> dev;
      // if the NetDeviceQueueInterface object is aggregated to a
      // NetDevice, get the MTU of such NetDevice
      if (ndqi && (dev = ndqi->GetObject<NetDevice> ()))
        {
          m_quantum = dev->GetMtu ();
          // NS_LOG_DEBUG ("Setting the quantum to the MTU of the device: " << m_quantum);
        }

      if (!m_quantum)
        {
          NS_LOG_ERROR ("The quantum parameter cannot be null");
          return false;
        }
    }

  return true;
}

void
FqCoDelQueueDisc::InitializeParams (void)
{
  // NS_LOG_FUNCTION (this);

  m_flowFactory.SetTypeId ("ns3::FqCoDelFlow");
  m_queueDiscFactory.SetTypeId ("ns3::CoDelQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (GetMaxSize ()));
  m_queueDiscFactory.Set ("Interval", StringValue (m_interval));
  m_queueDiscFactory.Set ("Target", StringValue (m_target));
}

uint32_t
FqCoDelQueueDisc::FqCoDelDrop (void)
{
  // NS_LOG_FUNCTION (this);

  uint32_t maxBacklog = 0, index = 0;
  Ptr<QueueDisc> qd;

  /* Queue is full! Find the fat flow and drop packet(s) from it */
  for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
    {
      qd = GetQueueDiscClass (i)->GetQueueDisc ();
      uint32_t bytes = qd->GetNBytes ();
      if (bytes > maxBacklog)
        {
          maxBacklog = bytes;
          index = i;
        }
    }

  /* Our goal is to drop half of this fat flow backlog */
  uint32_t len = 0, count = 0, threshold = maxBacklog >> 1;
  qd = GetQueueDiscClass (index)->GetQueueDisc ();
  Ptr<QueueDiscItem> item;

  do
    {
      item = qd->GetInternalQueue (0)->Dequeue ();
      DropAfterDequeue (item, OVERLIMIT_DROP);
      len += item->GetSize ();
    }
  while (++count < m_dropBatchSize && len < threshold);

  return index;
}

} // namespace ns3
