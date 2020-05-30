#program to set the editable configuration of server and client 
#It will automatically compile and generate the executable files

import os
import socket


def server_conf(server_port, timeout):
    with open('server.c', 'r') as file:
        file_data = file.read()

    # Replace the target string
   # file_data = file_data.replace('#define SERVER_IP "127.0.0.1"', server_ip)
    file_data = file_data.replace('#define SERVER_PORT 8881', server_port)
    file_data = file_data.replace('#define TIMEOUT_VALUE 120', timeout)
  #  file_data = file_data.replace('#define EXEC_PATH "/bin/bash"', path)
    # Write the file out again
    with open('tmpserver.c', 'w+') as file:
        file.write(file_data)


def client_conf(server_ip, server_port, timeout, path):
    with open('client.c', 'r') as file:
        file_data = file.read()

    # Replace the target string
    file_data = file_data.replace('#define SERVER_IP "127.0.0.1"', server_ip)
    file_data = file_data.replace('#define SERVER_PORT 8881', server_port)
    file_data = file_data.replace("#define TIMEOUT_VALUE 120", timeout)
    file_data = file_data.replace('#define EXEC_PATH "/bin/bash"', path)
    # Write the file out again
    with open('tmpclient.c', 'w+') as file:
        file.write(file_data)


# validate the IP address
def checkIP(address):
    splits = address.split(".")
    if len(splits) != 4:
        return False
    for i in splits:
        if not 0 <= int(i) <= 255:
            return False
    return True



print("*****************************************************************")
print("*****************************************************************")
print("\n******      Set your server and client configuration     ********\n")
print("*****************************************************************")
print("*****************************************************************\n\n")

#get the IP address of the local host
hostname = socket.gethostname()
local_IP = socket.gethostbyname(hostname)

#Get the server IP address and check validity
server_IP =  input(f">>Enter server IP:[Host IP {local_IP:}] ")
if checkIP(server_IP) == True:
    server_IP =  "#define SERVER_IP " + "\"" + server_IP + "\""
else:
    print(">>Invalid ip address. IP will be set to default 127.0.0.1")
    server_IP = '#define SERVER_IP "127.0.0.1"'


#Get the server port number and check validity
con_port = input(">>Enter connection port: ")
if con_port.isdigit() and int(con_port) < 65536:
    con_port = "#define SERVER_PORT " + con_port
else:
    print(">>Invalid port number.. port will be set to 8881")
    con_port = '#define SERVER_PORT 8881'


if input(">>Default timeout value is 120. Do you Want to change it?[Yes/No]").lower() ==  "yes":  
    timeout = "#define TIMEOUT_VALUE " + input(">>Enter timeout value: ")
else:
    timeout = '#define TIMEOUT_VALUE 120' 


if input(">>Default EXEC_PATH is \"/bin/bash\". Do you Want to change it?[Yes/No]").lower() ==  "yes":  
    EXEC_PATH = "#define EXEC_PATH " + "\"" + (input(">>Enter EXEC_PATH value: ")) + "\""
else:
    EXEC_PATH ="#define EXEC_PATH " + "\"/bin/bash\""


server_conf(con_port, timeout)
client_conf(server_IP,con_port,timeout,EXEC_PATH)

compile_server = 'gcc tmpserver.c -o server -lpthread -ljson-c'
os.system(compile_server)
os.system("rm tmpserver.c")

compile_client= 'gcc tmpclient.c -o client'
os.system(compile_client)
os.system("rm tmpclient.c")

print("\n\n>>Client and server executables have been generated and ready to run<<") 

print("\n\n****************************************************************")
print("*****************************************************************\n")
