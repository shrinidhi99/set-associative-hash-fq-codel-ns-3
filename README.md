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
**Recommended reading**:
* (RFC 8290) The Flow Queue CoDel Packet Scheduler and Active Queue Management Algorithm (Link: [https://tools.ietf.org/html/rfc8290](https://tools.ietf.org/html/rfc8290))
* ns-3 code of FQ-CoDel (Link: [https://gitlab.com/nsnam/ns-3-dev/blob/master/src/traffic-control/model/fq-codel-queue-disc.h](https://gitlab.com/nsnam/ns-3-dev/blob/master/src/traffic-control/model/fq-codel-queue-disc.h))
* Details of set associative hash (Link: [https://www.bufferbloat.net/projects/codel/wiki/CakeTechnical/](https://www.bufferbloat.net/projects/codel/wiki/CakeTechnical/))
---
