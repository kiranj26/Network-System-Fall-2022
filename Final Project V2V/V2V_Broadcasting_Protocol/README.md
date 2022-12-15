# V2V BROADCASTING PROTOCOL

**Memebers** : Kevin Jonnes, Isha Burange, Bhoomika Singla, Kiran Jojare.  
**Subject**  : Network System, fall 2022

NOTE: V2V_model is the only code that is used. The test module was used for early testing.

## INTRODUCTION

This project is to imulate a broadcast protocol for vehicle to vehocle communications. 
Each program instance acts as a vehicle and will communicate with other vehicles, selectively expecting acks/retransmitting for vehicles withing the Prange of static variable defined at the beginning of the code. 

It is worth noting there are ack/event timeout variables that can be used to vary the time spent before retransmitting and giving up on a given transmission.

There is also a p value used to set a probability of triggering an emergency event.

## INSTRUCTIONS TO RUN

* Setup VMs on a LAN. We used virtualbox with VMs on a bridged network so dhcp is setup for us, however there are many solutions that can be used. Linux was used for testing, but windows should be fine for the python modules used.

* The LAN variable should be set to the broadcasat LAN address the vms are connected to (last address in the subnet range). Not using this will give you an error after setting tx port settings to SO_BROADCAST...

* We used vscode with a python notebook enabled to allow for faster troubleshooting of submodules, 
but the code could just be pasted into a single block if desired.

## RESULTS

The results were evaluated over a couple of test scenarios. Of particular interest was showing 3 vehilces at varying speeds. 

This allowed us to upserve stdout and see broadcasts observed, proximity tables being updated when appropriate, emergency messages are being sent, waiting for a response, and updating the retransmit table.

As the vehicles get out of range, they stop updating the proximity table and the entry is removed. 
This scenario exercises all code modules, though more testing/tweaking variables should be done.

## TAKEAWAYS:
A lot more tweaking is needed to look at recieve timeouts, buffer read/write optimization (ratio of sends to reads), ack/event timeouts, etc. This code could also be multithreaded/use select/pselect. I believe a shared key could also be used to create subgroups of vehicles or using a cellular link for authenticating vehicles. 

## EXTENT BUGS
No Extent bugs were found in the code
