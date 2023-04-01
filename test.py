import sqlite3
import time
import random
from socket import *
serverName = 'localhost'
NUM_MESSAGES=100000

# Read configuration from file
with open('.\\Debug\\fconfig.ini') as f:
    lines = f.readlines()
serverPort = int(lines[1].split('=')[1].strip())
print("Sending messages to "+str(serverPort))
prefix1 = lines[3].split('=')[1].strip()
prefix2 = lines[4].split('=')[1].strip()
print ("The messages with "+prefix1+" and "+prefix2+" prefixes should be saved in a database")
#print(prefix1)
#print(prefix2)
clientSocket = socket(AF_INET, SOCK_DGRAM)
messages=[]
# Define some test messages with prefixes
# messages = [
#     prefix2+'hello',
#     prefix1+'456',
#     prefix1+'4000674',
#     prefix1+'40506000',
#     prefix2+'world',
#     'invalid_prefix789'
# ]

for i in range(10,NUM_MESSAGES):
    if i % 2 == 0:
        message=prefix1+str(random.randrange(10, NUM_MESSAGES+1, 2))
    else:
        message=prefix2+"message"+str(random.randrange(11, NUM_MESSAGES, 2))
    #print (message)
    messages.append(message)

int_messages=[]
st_messages=[]
#for i in range(3):
for message in messages:
    if message[:len(prefix1)] == prefix1:
        int_messages.append(int(message[len(prefix1):]))
    elif message[:len(prefix2)]==prefix2:
        st_messages.append(message[len(prefix2):])
    #time.sleep(0.001)
    clientSocket.sendto(message.encode(),(serverName, serverPort))
    modifiedMessage, serverAddress = clientSocket.recvfrom(2048)
    #print(modifiedMessage.decode())
clientSocket.close()

db = sqlite3.connect('.\\Debug\\messagedb.db')
c = db.cursor()

rows = c.execute('SELECT * FROM messages').fetchall()
#print ("rows=")
#print (rows)
assert len(rows) == len(int_messages) + len(st_messages)
i=0
j=0
for row in rows:
    if row[1] != None:
        assert row[1] == int_messages[i]
        #print ("p1 messege="+str (row[1]))
        i=i+1
    elif row[2] != None:
        assert row[2] == st_messages[j]
        #print("p2 messege=" + row[2])
        j=j+1
print ("tests passed succesfully")