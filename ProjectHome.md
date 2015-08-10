# Description #
The Asterisk Manager Interface (AMI) allows a client program to connect to an Asterisk instance and issue commands or read events over a TCP/IP stream. Integrators will find this particularly useful when trying to track the state of a telephony client inside Asterisk, and directing that client based on custom (and possibly dynamic) rules.

This is an API developed in C (this is the only one using C langage), used to control and communicate with Asterisk IPPBX system over its AMI manager.
useful for embedded system, I hope :)

### Supported System ###
unix systems are supported only
### Asterisk SIP Users interface ###
You can create,Read and delete Users in Asterisk remotely
### Asterisk SIP Trunk interface ###
You can create,Read and delete Trunk in Asterisk remotely
### Asterisk SIP Register interface ###
Get Register of Asterisk giving you more control over Asterisk trunks
### Speed and Memory ###
Great Speed in communication with Asterisk, and small memory use, make it powerful in embedded system.
### configuration files ###
This API Manage all the Asterisk Files throw the AMI manager, and gives Restore/Back-up mechanism.
### Dynamic Configurations generator ###
Configurations Files are now auto-generated from XML other XML file, to centralise all informatin between the API and Asterisk.