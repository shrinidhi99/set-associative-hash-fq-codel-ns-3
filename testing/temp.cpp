    if (m_setAssociativity)
    {
      uint32_t h = 0;
      uint32_t innerHash, outerHash;
      uint32_t flowHash;
      uint32_t set_ways = 8;

      if (GetNPacketFilters () == 0)
        {
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
              flowHash = ret;
              h = ret % m_flows;
              innerHash = h % set_ways;
              outerHash = h - innerHash;
            }
          else
            {
              NS_LOG_ERROR ("No filter has been able to classify this packet, drop it.");
              DropBeforeEnqueue (item, UNCLASSIFIED_DROP);
              return false;
            }
        }

      Ptr<FqCoDelFlow> flow;
      if (m_flowsIndices.find (outerHash) == m_flowsIndices.end ())
        {
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
              flow = StaticCast<FqCoDelFlow> (GetQueueDiscClass (i));

              if (tags[outerHash + i - m_flowsIndices[outerHash]] == flowHash ||
                  flow->GetStatus () == FqCoDelFlow::INACTIVE)
                {
                  if (flow->GetStatus () == FqCoDelFlow::INACTIVE)
                    {
                  
                      flow->SetStatus (FqCoDelFlow::NEW_FLOW);
                      flow->SetDeficit (m_quantum);
                      m_newFlows.push_back (flow);
                      
                    }
                  flow->GetQueueDisc ()->Enqueue (item);
                  tags[outerHash + i - m_flowsIndices[outerHash]] = flowHash;
                  flag = true;
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
              tags[outerHash] = flowHash;
              if (GetCurrentSize () > GetMaxSize ())
                {
                  FqCoDelDrop ();
                }
            }
        }
    }