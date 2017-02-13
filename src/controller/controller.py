#!/usr/bin/python

import sys
import getopt
import time
from kazoo.client import KazooClient
from kazoo.client import KazooState
from kazoo.exceptions import KazooException

previous_state = {}
local_state = {}
ti = 32

def my_listener(state):
    if state == KazooState.LOST:
        # Register somewhere that the session was lost
        print "Lost, I am..."
    elif state == KazooState.SUSPENDED:
        # Handle being disconnected from Zookeeper
        print "Suspended, I am.."
    else:
        # Handle being connected/reconnected to Zookeeper
        print "Connected, I am.."

def zk_initialize(ip, port):
    return KazooClient(hosts= ip + ':' + port)

def zk_strt(zk):
    zk.start()
    zk.add_listener(my_listener)

def compare_state(proxy):
    if (previous_state[proxy] == local_state[proxy]):
        print "Crash!!"
    else:
        previous_state[proxy] = local_state[proxy]
        print "No Crash!!"

def verify_hb(zk):
    proxies = zk.get_children("/hb/")
    
    for proxy in proxies:
        data, stat = zk.get("/hb/" + proxy + "/")
        #zk.set("/hb/" + proxy + "/", "some data")
        #print("Version: %s, data: %s" % (stat.version, data.decode("utf-8")))
        local_state[proxy] = data.decode("utf-8")
        compare_state(proxy)
        
def set_state(zk):
     proxies = zk.get_children("/hb/")

     for proxy in proxies:
         data, stat = zk.get("/hb/" + proxy + "/") 
         previous_state[proxy] = data.decode("utf-8")
         
       
def main():
    # Default values
    ip = "127.0.0.1"
    port = "2181"
    global ti
    
    try:
        opts, args = getopt.getopt(sys.argv[1:],"port",["ti=","ip=","port="])
    except getopt.GetoptError:
        print 'connection.py --ti <> --ip <ip> --port <port>'
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-ip", "--ip"):
            ip = arg
        elif opt in ("-port", "--port"):
            port = arg
        elif opt in ("-ti", "--ti"):
            ti = arg
    
    zk = zk_initialize(ip, port)
    zk_strt(zk)
    zk_initialize(ip, port)
    
    set_state(zk)
    time.sleep(int(float(ti)))
    
    while(1):
        verify_hb(zk)
        time.sleep(int(float(ti)))

if __name__ == "__main__":
    main()
