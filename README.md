Process Migration
=================

My final year project for my Bachelor's degree. My team’s objective was to find a viable method
to implement process migration on a stock operating system without any major structural changes. 

We're using:
- DMTCP
- Google Protocol Buffers
- ZeroMQ
- nCurses

The DMTCP Coordinator was responsible for freezing, restarting and managing communications selected
for migrations. We have settled on a dynamic dual-threshold policy to determine whether a node was
overloaded or not, the CPU run queue length was decided to be the metric for gauging the load of a
system in the cluster. Spikes in the loads for high I/O systems by taking an exponentially damped 
moving average of the system load. The most challenging part to implement for the system was the state
information exchange module, which handled the exchange of the load information between nodes. We had
Google Protocol Buffers converting the data to wire format and have ZeroMQ, a messaging library broadcast
the information throughout the cluster.

The system is able to independently make a decision regarding process transfer and actually transfer a
running process from one node to another. 
