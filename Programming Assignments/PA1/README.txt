# NetSys-Assignment1
UDP Socket Communication

## Communication commands 
'''
Client				<-->	Server

Put <filename>	 	--->	Creates/opens <filename> file and saves file data
	Next Packet 	<---	Send ACK on reception of each packet
	Done 		 	--->	File received completely
	
Get <filename>	 	--->	Opens <filename> file
	store 			<---	Send each packet
	send ACK	 	--->	Next packet
	Got file		<---	Done

Delete <filename>	--->	rm <filename
	Got ACK			<---	file deleted
	
ls					--->	ls in current folder
	display ls		<---	send ls data

exit				---> 	exit server gracefully
	exit client		<---	send ACK
'''
	
## Packet structure

### Data packet
1st byte - Packet number between '0' to '9' then repeat
2nd byte - number of data bytes in packet - max 100
3rd to max 102nd byte - data bytes to be sent 

### ACK packet
1st byte - Packet number between '0' to '9' then repeat
2nd byte - 0x01

### NACK packet
1st byte - Packet number between '0' to '9' then repeat
2nd byte - 0xFF

### Note:
Above packet structure is made for confirming whether all packets and data bytes are received.
In case of packet missing or data missing resend packet logic is implemented.

