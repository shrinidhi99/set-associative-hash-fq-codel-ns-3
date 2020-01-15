# Implementation of FQ-CoDel using set associative hash
---
**Brief**: Flow Queue Controlled Delay (FQ-CoDel) is a popular queue discipline to address the
problem of bufferbloat. FQ-CoDel is implemented in the mainline of ns-3. However, the
hashing technique of FQ-CoDel suffers from the classic birthday problem, where the chance
of collision of flows into the same bucket rises to 50% when the number of flows approaches
the square root of the maximum number of buckets. The main aim of this project is to use
set-associative hash in the ns-3 implementation of FQ-CoDel.

---

You can find more details about our work at the [wiki](https://github.com/shrinidhi99/set-associative-hash-fq-codel-ns-3/wiki).

**Introduction**:

The concept of set associativity we use here is similar to its contemporary application to caches in computer architecture, where the idea is used to minimize cache misses and increase overall performance. In the context of managing network flows in queues, this technique is claimed to reduce collisions, as compared to direct hashing. A flow is determined uniquely by the five-tuple (source IP address, destination IP address, source port number, destination port number, protocol number). Linux implementation uses 1024 queues, and 8 queues in each set. We consider the same parameters here. For each packet, a hash function is used to obtain a value (hash) corresponding to this flow (call this the ```flow_hash```). To identify the queue number to which this packet should be assigned to, we take the remainder modulo 1024 to get a ```reduced_hash```. Starting from the first queue in this set, we iteratively check if the associated queue (indexed by ```reduced_hash```) stores the corresponding ```flow_hash```. If such a match is found, the packet is added to this queue. If no such queue is found that stores packets of this flow, we accept the fact that there is a collision and add the packet to the first queue of the set, changing the associated flow of this queue accordingly.

**Expected Advantages**: FQ-Codel currently uses traditional hashing to hash flows into the queues, and is suboptimal. This works well for a moderate number of flows, such as seen in a home gateway. As the number of flows increase, the number of hash collisions also increase. The birthday problem provides a more concrete view of why this occurs. There is a 50% chance of collision occurring as the number of flows reaches sqrt(number of buckets). The main aim of this project is, of course, to avoid this and reduce the chances of hash collisions. As mentioned in RFC 8290, with 1024 buckets and perfect hashing, the probability of no collision occurring with 100 flows is 90.78% - this probability can be further improved by using set-associative hashing. According to the RFC's analytical equations, the probability of no collision with an 8-way set associative hash is around 100%, which is a significant improvement over the normal hashing technique.

Furthermore, set associative hashing is a prominent feature of the comprehensive queue management system CAKE (Common Applications Kept Enhanced). Our implementation of set-associative hashing can be resued when CAKE is implemented in the future in ns3.

**Class Relationships, coupling and implementation details**: 

**References**:
* [**The Flow Queue CoDel Packet Scheduler and Active Queue Management Algorithm (RFC 8290)**](https://tools.ietf.org/html/rfc8290) 
  
  _This memo presents the FQ-CoDel hybrid packet scheduler and Active Queue Management (AQM) algorithm, a powerful tool for fighting bufferbloat and reducing latency._

* [**ns-3 code of FQ-CoDel**](https://gitlab.com/nsnam/ns-3-dev/blob/master/src/traffic-control/model/fq-codel-queue-disc.h) 
  
  _A flow queue used by the FqCoDel queue disc in ns3-dev_

* [**CAKE Technical Information**](https://www.bufferbloat.net/projects/codel/wiki/CakeTechnical/)
  
  _Details of set-associative hash_
 
* [**Piece of CAKE: A Comprehensive Queue Management Solution for Home Gateways**](https://arxiv.org/pdf/1804.07617.pdf)

  _This paper presents Common Applications Kept Enhanced (CAKE), a comprehensive network queue management system designed specifically for home Internet gateways_
  
**Testing**: 

> Commands to Run:
```shell
NS_LOG="FqCoDelQueueDisc" ./test.py -s fq-codel-queue-disc --text=results

NS_LOG="FqCoDelQueueDisc" ./waf --run "test-runner --suite=fq-codel-queue-disc"

NS_LOG="FqCoDelQueueDisc:FqCoDelQueueDisc2:PacketFilter:QueueDisc" ./waf --run "test-runner --suite=fq-codel-queue-disc"

NS_LOG="FqCoDelQueueDisc" ./waf --run "test-runner --suite=fq-codel-queue-disc" > data26

```
