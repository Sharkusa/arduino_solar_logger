#!/usr/bin/python2.7

# Program Summary
# This script launches external process based on value of batteryvoltage 
# battery_volts is obtained from external JSON API
# Process Id of the launched process is stored locally.
# polling_minutes decides the frequency of process checking
# polling_minutes = 1, indicates the program checks battery_volts every minute
 
import json
import urllib2
import subprocess
import time
import os
import signal

POLLING_MINUTES = 15
api_url = "http://192.168.88.234/cgi-bin/getLastVolts.py"
#process_path = "/root/controller/procStart.sh"
#process_path = "/root/veriumMiner/cpuminer -o stratum+tcp://vrm.n3rd3d.com:3333 -u Longhorn.d1_test_server -p 1234 --api-bind 0.0.0.0:4048 -t1"
process_path = "~/veriumMiner/cpuminer -o stratum+tcp://vrm.n3rd3d.com:3333 -u Longhorn.solar_test_server -p 1234 --api-bind 0.0.0.0:4048 -t12"
#process_path = "/root/veriumMiner/cpuminer"
#process_param = "-o stratum+tcp://vrm.n3rd3d.com:3333 -u Longhorn.d1_test_server -p 1234 --api-bind 0.0.0.0:4048 -t1"
MIN_VOLTS = 12.100000
AVG_VOLTS = 12.500000
MAX_VOLTS = 13.000000

BASE_THREAD_COUNT = 6
MAX_THREAD_COUNT = 12
poll_time = POLLING_MINUTES*60
proc_id = False



#def sleeper(process_id):
def sleeper():
	global proc_id
	process_path = ""
	process_id = False
	maxThreads = False
	while True:
		jsonobj = json.load(urllib2.urlopen(api_url))
		battery_volts = jsonobj[0]['battery']
		print "Got Battery Voltage from API"
		print jsonobj
		print battery_volts

		if(float(battery_volts) >= MAX_VOLTS):
 			print "Voltage above MAX"
			if maxThreads:
				print "cpu miner already with MAX_THREAD_COUNT. Nothing to do"
			else:
			  if process_id:
				  print "Process running with BASE_THREAD_COUNT. STOP and START again"
				  os.killpg(os.getpgid(process_id.pid), signal.SIGTERM)
				  time.sleep(5)
			  print "Process starting with MAX_THREAD_COUNT"
			  process_path = base_path + str(MAX_THREAD_COUNT)
			  print process_path
			  maxThreads = True
			  process_id = subprocess.Popen(process_path, shell=True, stderr=subprocess.STDOUT, preexec_fn=os.setsid)
			  proc_id = process_id
		if((float(battery_volts) > MIN_VOLTS) and (float(battery_volts) < MAX_VOLTS)):
			if maxThreads:
				print "cpu miner running with MAX_THREAD_COUNT. STOP MAX THREADS"
				os.killpg(os.getpgid(process_id.pid), signal.SIGTERM)
				process_id = False
				proc_id = False
				maxThreads = False
				time.sleep(5)
		if((float(battery_volts) > AVG_VOLTS) and (float(battery_volts) < MAX_VOLTS)):
			if not process_id:
        #no running process. start new
				print "Voltage above AVG: start cpu miner"
				process_path = base_path + str(BASE_THREAD_COUNT)
				print process_path
				process_id = subprocess.Popen(process_path, shell=True, stderr=subprocess.STDOUT, preexec_fn=os.setsid)
				proc_id = process_id
				maxThreads = False
				#process_id = subprocess.Popen([process_path, process_param.split()], shell=False, stderr=subprocess.STDOUT)
			else:
				print "Process already running. No need to start again"
			
		if(float(battery_volts) < MIN_VOLTS):
		 	print "Voltage below MIN: stop cpu miner"
			if(process_id != False):
				#os.kill(process_id.pid, signal.SIGTERM)
				os.killpg(os.getpgid(process_id.pid), signal.SIGTERM)
				#process_id.kill()
				process_id = False
				proc_id = False
                                maxThreads = False
			else:
				print "nothing to stop"
		print "sleep for"
		print poll_time
		print "seconds\n\n"
		#sleep for poll_time seconds
		time.sleep(poll_time)

try:
    print('\n\nControl Program Started')
    #sleeper(proc_id)
    sleeper()
except KeyboardInterrupt:
    print('\n\nKeyboard exception received. Exiting.')
    if(proc_id != False):
     print('killing child process')
     print('proc id')
     print(proc_id)
     os.killpg(os.getpgid(proc_id.pid), signal.SIGTERM)
     #os.kill(proc_id.pid, signal.SIGTERM)
     #proc_id.kill()
    exit()
