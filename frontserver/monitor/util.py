from dingtalkchatbot.chatbot import DingtalkChatbot
import subprocess
import datetime
import threading
import psutil
import time

def get_datetime_str():
    return datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def get_process_cpu_usage(process):
    cpu_count = 10
    cpu_list = []
    usage = 0
    for i in range(cpu_count):        
        p_cpu = process.cpu_percent(interval=1.0 /cpu_count)
        #if p_cpu == 0.0:
         #   continue
        cpu_list.append(p_cpu)
    if len(cpu_list) > 0:
        #print(cpu_list)
        usage = float(sum(cpu_list))/len(cpu_list)
    return usage

def readCpuInfo():
    f = open('/proc/stat')
    lines = f.readlines();
    f.close()
    for line in lines:
        line = line.lstrip()
        counters = line.split()
        if len(counters) < 5:
            continue
            
        if counters[0].startswith('cpu'):
            break
            
    total = 0
    for i in range(1, len(counters)):
        total = total + int(counters[i])
    idle = int(counters[4])
            
    return {'total':total, 'idle':idle}
                        
def calcCpuUsage(counters1, counters2):
    idle = counters2['idle'] - counters1['idle']
    total = counters2['total'] - counters1['total']
    return 100 - (idle*100/total)
            
def get_cpu_usage():
    counters1 = readCpuInfo()
    # print(counters1)

    time.sleep(0.1)

    counters2 = readCpuInfo()
    # print(counters2)

    useage = calcCpuUsage(counters1, counters2)

    return useage

def readMemInfo():
    res = {'total':0, 'free':0, 'buffers':0, 'cached':0}
    f = open('/proc/meminfo')
    lines = f.readlines()

    f.close()
    i = 0
    for line in lines:
        if i == 4:
            break
          
        line = line.lstrip()
        memItem = line.lower().split()
          
        if memItem[0] == 'memtotal:':
            res['total'] = int(memItem[1])
            i = i +1
            continue
        elif memItem[0] == 'memfree:':
            res['free'] = int(memItem[1])
            i = i +1
            continue
        elif memItem[0] == 'buffers:':
            res['buffers'] = int(memItem[1])
            i = i +1
            continue
        elif memItem[0] == 'cached:':
            res['cached'] = int(memItem[1])
            i = i +1
            continue
          
    return res
          
def calcMemUsage(counters):          
    used = counters['total'] - counters['free'] - counters['buffers'] - counters['cached']
    total = counters['total']
    return used * 100 / total

def get_mem_usage():
    counters = readMemInfo()          
    usage = calcMemUsage(counters)
    return usage
