# Software Development & Security
## Group Assignment (Due 10th May)
### Cody Lobban, Will Ng, John Dickson, Zakariya Al-Darmaki

#### Targets/Plans: Interactive terminal based Command and control Bot-net coded in c targeting Linux operating system. Infrastructure includes master and slave function. Bot can have functionality to create persistence, and receive remote command execution from the master.


#### TO-DO (incomplete) :
..* 1. *Connectivity*

  * 1.1 Client
    * 1.1.1 Client Framework
      * 1.1.1.1 Connect to Server
      
  * 1.2 Server
    * 1.2.1 Server Framework
      * 1.2.1.1 Accept Multiple Client Connections
        * 1.2.1.1.1 Accept Singular Client Connection
        * 1.2.1.1.2 Listen for Client Connection
        * 1.2.1.1.3 Close Client Connection
        * 1.2.1.1.4 Obfuscate Data
        
..* 2. *Communication*
  * 2.1 Remote Code Execution
    * 2.1.1 Server: Send Command
      * 2.1.1.1 Parse User Input
    * 2.1.2 Client: Execute Command
      * 2.1.2.1 Parse Command from Server
      * 2.1.2.2 Return Output to Server
      
..* 3. *Interface* 
  * 3.1 Server Interface 
    * 3.1.1 Welcome Screen
      * 3.1.1.1 Help Menu
    * 3.1.2 Analytic Console
      * 3.1.2.1 Connected Clients
      * 3.1.2.2 Jobs List
      * 3.1.2.3 Help Menu
    * 3.1.3 Control Console
      * 3.1.3.1 Help Menu
