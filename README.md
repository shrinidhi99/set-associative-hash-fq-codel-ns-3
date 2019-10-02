# Implementation of FQ-CoDel using set associative hash
---
**Brief**: Flow Queue Controlled Delay (FQ-CoDel) is a popular queue discipline to address the
problem of bufferbloat. FQ-CoDel is implemented in the mainline of ns-3. However, the
hashing technique of FQ-CoDel suffers from the classic birthday problem, where the chance
of collision of flows into the same bucket rises to 50% when the number of flows approaches
the square root of the maximum number of buckets. The main aim of this project is to use
set-associative hash in the ns-3 implementation of FQ-CoDel.

**Required experience**: C, C++ and Fundamentals of Linux kernel

**Bonus experience**: Knowledge of FQ and CoDel is a plus.

**Difficulty**: High
---
**Introduction**: 
**Expected Advantages**: FQ-Codel currently uses traditioal hashing to hash the flows into the queues and it is suboptimal. This works well for modernate number of flows such as seen in a home gateway. As the number of flows increase, the number of hash collision also increase. The birthday problem provides a more concrete view on why this occurs. There is a 50% chance of collision occuring as the number of flows reach the sqrt(number of buckets). The main aim of this project is, of course, to avoid this and reduce the chances of hash collisions. As mentioned in RFC 8290, with 1024 buckets and perfect hashing the probability of no collision occuring with 100 flows is 90.78%. This probability can be further improved by using set-associative hashing as it is again mentioned in the RFC. According to the author's analyitical equations, the probability of no collision with 8 way set associative hash is around 100% which is a significant improvement over the normal hashing technique. 
Furthermore, the recent development of Comprehensive queue management system(CAKE) there is a need for set associative hashing as it is one of the main features develeoped in CAKE. This development can be further quickened by our project as it will involve the implementation of set associative hashing which can then be reused when CAKE is implemented in the future in ns3.  

**Class Relationships, coupling and implementation details**: 

**References**: 
---
**Recommended reading**:
* (RFC 8290) The Flow Queue CoDel Packet Scheduler and Active Queue Management Algorithm (Link: [https://tools.ietf.org/html/rfc8290](https://tools.ietf.org/html/rfc8290))
* ns-3 code of FQ-CoDel (Link: [https://gitlab.com/nsnam/ns-3-dev/blob/master/src/traffic-control/model/fq-codel-queue-disc.h](https://gitlab.com/nsnam/ns-3-dev/blob/master/src/traffic-control/model/fq-codel-queue-disc.h))
* Details of set associative hash (Link: [https://www.bufferbloat.net/projects/codel/wiki/CakeTechnical/](https://www.bufferbloat.net/projects/codel/wiki/CakeTechnical/))
