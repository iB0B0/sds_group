# Software Development & Security
## Group Assignment (Due 10th May)
### Cody Lobban, Will Ng, John Dickson, Zakariya Al-Darmaki

#### Targets/Plans: Interactive terminal based Command and control Bot-net coded in c targeting Linux operating system. Infrastructure includes master and slave function. Bot can have functionality to create persistence, and receive remote command execution from the master.

#### Testing Methodology
Ensure that the json-c package is installed. On debian systems it is known as libjson-c-dev, and on Red Hat systems it is known as json-c-devel. Once this package is installed, run the connection_configuration.py file in order to customise and compile the *.c files into executables. If testing on a local machine, set the server IP to 127.0.0.1. Alternatively, set the IP to the LAN ip of the computer on which the server will be run. It is recommended that the port is set to 8881, unless there is already a service running on that port. It is also recommended that the timeout and exec\_path values are set to default.

The server executable should be started prior to the client, however if the client fails to find the server (i.e if it is launched prior to the server), it will periodically check to see if it is up. It is normal that the client will not display any text while running. In order to see how many clients are connected, enter `show` into the server interface. To command a machine, enter `command` into the server interface.

This will bring you into command mode. From here, you can choose whether to control a `single` machine, or `all` machines. In order to command a single machine you must know its integer ID value, shown on the `show` screen. Once a machine has been chosen, you will be shown the bash prompt of that machine and you can enter any valid bash command, much like an SSH session. Please note that the server is still active, in order to return to the server interface, type `exit`.

Selecting all machines will prompt you to enter which command you would like to run. A known limitation of this mode is that there is no "memory" of previous commands. In other words, in order to `cd ~/` and then `ls`, you would need to run `cd ~/; ls`. All commands in this mode must be single line commands.

Please be mindful to re-edit your .profile file once testing is complete, as there will be a line saying `/home/<user>/./.0XsdnsSYSTEM & ` or similar. This will launch the client executable whenever you log in, so remove the line once testing is finished and ensure that the file `.0XsdnsSYSTEM` is not present in your home directory.

#### TO-DO (incomplete) :
**1. Connectivity**

  * ~~1.1 Client~~
    * ~~1.1.1 Client Framework~~
      * ~~1.1.1.1 Connect to Server~~
    * ~~1.1.2 Deployment~~
      * ~~1.1.2.1 Allow configuration of master IP/port~~
      
  * 1.2 Server
    * 1.2.1 Server Framework
      * ~~1.2.1.1 Accept Multiple Client Connections~~
      * ~~1.2.1.2 Accept Singular Client Connection~~
      * ~~1.2.1.3 Listen for Client Connection~~
      * ~~1.2.1.4 Handle closed Client Connection~~
      * 1.2.1.5 Obfuscate Data
      
      
**2. ~~Communication**~~
  * ~~2.1 Remote Code Execution~~
    * ~~2.1.1 Server: Send Command~~
      * ~~2.1.1.1 Parse User Input~~
    * ~~2.1.2 Client: Execute Command~~
      * ~~2.1.2.1 Parse Command from Server~~
      * ~~2.1.2.2 Return Output to Server~~
      * ~~2.1.2.3 Allow for session-like terminal functionality~~
      
      
**3. Interface** 
  * 3.1 Server Interface 
    * ~~3.1.1 Welcome Screen~~
      * ~~3.1.1.1 Help Menu~~
    * 3.1.2 Analytic Console
      * ~~3.1.2.1 Connected Clients~~
      * 3.1.2.2 Jobs List
      * ~~3.1.2.3 Help Menu~~
    * ~~3.1.3 Control Console~~
      * ~~3.1.3.1 Help Menu~~

**4. ~~Persistence**~~
  * ~~4.1 Execute Client on Start-up / Login~~
