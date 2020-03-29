/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/log.h"
#include "fifo-queue-disc.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"
#include "ack-filter.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (FifoQueueDisc);

TypeId FifoQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FifoQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FifoQueueDisc> ()
    .AddAttribute ("MaxSize",
                   "The max queue size",
                   QueueSizeValue (QueueSize ("1000p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
  ;
  return tid;
}

FifoQueueDisc::FifoQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
  NS_LOG_FUNCTION (this);
}

FifoQueueDisc::~FifoQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
FifoQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  if (GetCurrentSize () + item > GetMaxSize ())
    {
      NS_LOG_LOGIC ("Queue full -- dropping pkt");
      DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
      return false;
    }

  bool retval = GetInternalQueue (0)->Enqueue (item);
  Ptr<Queue<QueueDiscItem>> queue = GetInternalQueue (0);
  AckFilter ack;
  ack.AckFilterMain(queue);
  // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
  // internal queue because QueueDisc::AddInternalQueue sets the trace callback

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return retval;
}

Ptr<QueueDiscItem>
FifoQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = GetInternalQueue (0)->Dequeue ();

  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  return item;
}

Ptr<const QueueDiscItem>
FifoQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item = GetInternalQueue (0)->Peek ();
  
  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  return item;
}

bool
FifoQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("FifoQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("FifoQueueDisc needs no packet filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // add a DropTail queue
      AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
                          ("MaxSize", QueueSizeValue (GetMaxSize ())));
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("FifoQueueDisc needs 1 internal queue");
      return false;
    }

  return true;
}

void
FifoQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

bool
FifoQueueDisc::AckFilterMayDrop (Ptr<QueueDiscItem> item, uint32_t tstamp,uint32_t tsecr) const
{
  uint8_t flags;
  if ((((item->GetUint8Value (QueueItem::TCP_FLAGS,flags)) & uint32_t (0x0F3F0000)) != TcpHeader::ACK) || item->HasTcpOption (TcpOption::SACKPERMITTED) || item->HasTcpOption (TcpOption::WINSCALE) || item->HasTcpOption (TcpOption::UNKNOWN))
    {
      return false;
    }
  else if (item->HasTcpOption (TcpOption::TS))
    {
      uint32_t tstamp_check,tsecr_check;
      item->TcpGetTimestamp (tstamp_check,tsecr_check);
      if ((tstamp_check < tstamp) || (tsecr_check < tsecr))
        {
          return false;
        }
      else
        {
          return true;
        }
    }
  else
    {
      return true;
    }
}

int
FifoQueueDisc::AckFilterSackCompare (Ptr<QueueDiscItem> item_a, Ptr<QueueDiscItem> item_b) const
{
  if (item_a->HasTcpOption (TcpOption::SACK) && !(item_b->HasTcpOption (TcpOption::SACK)))
    {
      return -1;
    }
  else if (!(item_a->HasTcpOption (TcpOption::SACK)) && (item_b->HasTcpOption (TcpOption::SACK)))
    {
      return 1;
    }
  else if (!(item_a->HasTcpOption (TcpOption::SACK)) && !(item_b->HasTcpOption (TcpOption::SACK)))
    {
      return 0;
    }
  typedef std::list<std::pair<SequenceNumber32,SequenceNumber32> > sack;
  sack sack_a, sack_b;
  sack_a = item_a->TcpGetSackList ();
  sack_b = item_b->TcpGetSackList ();
  SequenceNumber32 ack_seq_a = item_a->GetAckSeqHeader ();
  uint32_t bytes_a = 0, bytes_b = 0;
  while (true)
    {
      sack sack_temp = sack_b;
      sack::iterator it_a = sack_a.begin ();
      sack::iterator it_b = sack_b.begin ();
      SequenceNumber32 start_a,end_a;
      start_a = it_a->first;
      end_a = it_a->second;
      bool found = false;
      bool first = true;

      if (start_a < ack_seq_a)
        {
          return -1;
        }
      bytes_a += end_a - start_a;
      while (true)
        {
          SequenceNumber32 start_b, end_b;
          start_b = it_b->first;
          end_b = it_b->second;
          if (first)
            {
            }
          bytes_b += end_b - start_b;
          if (!(start_b > start_a) && !(end_b < end_a))
            {
              found = true;
              if (!first)
                {
                  break;
                }
            }
          it_b++;
        }
      if (!found)
        {
          return -1;
        }
      else
        {
          it_a++;
          first = false;
        }
    }
  return bytes_b > bytes_a ? 1 : 0;
}

} // namespace ns3
