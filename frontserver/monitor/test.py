import time
from dingtalkchatbot.chatbot import DingtalkChatbot
import subprocess
import datetime
import threading
import psutil
        
                      
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
    print(counters1)

    time.sleep(0.1)

    counters2 = readCpuInfo()
    print(counters2)

    print(calcCpuUsage(counters1, counters2))
            

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
    print(calcMemUsage(counters))

def readNetInfo():
    f = open('/proc/net/dev')
    lines = f.readlines()
    print(lines) 
    f.close()
    res = {'in':0, 'out':0}

    for line in lines:
        if line.lstrip().startswith("eth0"):
            # for centos
            line = line.replace(':', ' ')
            items = line.split()
            res['in'] = float(items[1]) / 1024 / 1024 / 8
            res['out'] = float(items[int(len(items)/2) + 1]) / 1024 / 1024/ 8
    return res
            
def get_net_usage():
    print(readNetInfo())

def basic_test():
    cmd_str = "df -h"
    result_str = subprocess.getoutput(cmd_str)
    result_list = result_str.split("\n")
    print(len(result_list))    

def test_all_info():
    p = psutil.Process(9359)
    print(p.name())      #进程名
    print(p.exe())         #进程的bin路径
    print(p.cwd())        #进程的工作目录绝对路径
    print(p.status())     #进程状态
    print(p.create_time())  #进程创建时间
    print(p.uids())      #进程uid信息
    print(p.gids())      #进程的gid信息
    print(p.cpu_times())    #进程的cpu时间信息,包括user,system两个cpu信息
    print(p.cpu_affinity())  #get进程cpu亲和度,如果要设置cpu亲和度,将cpu号作为参考就好
    cpu_list = []
    cpu_count = 10
    for i in range(cpu_count):        
        p_cpu = p.cpu_percent(interval=1/cpu_count)
        cpu_list.append(p_cpu)
    print(float(sum(cpu_list))/len(cpu_list))

    # print(p.cpu_percent())
    print(p.memory_percent())  #进程内存利用率
    print(p.memory_info())    #进程内存rss,vms信息

    print(p.io_counters())    #进程的IO信息,包括读写IO数字及参数
    # print(p.connectios())    #返回进程列表
    print(p.num_threads())  #进程开启的线程数

    # from subprocess import PIPE
    # p = psutil.Popen(["/usr/bin/python", "-c", "print('hello')"],stdout=PIPE)
    # p.name()
    # p.username()    


def test_psutil():
    for proc in psutil.process_iter():
        if proc.name() == "simulate_trade":
            print("pid-%d,name:%s" % (proc.pid,proc.name()))   
        
def get_disk_info():
    # 循环磁盘分区
    content = ""
    for disk in psutil.disk_partitions():
        # 读写方式 光盘 or 有效磁盘类型
        if 'cdrom' in disk.opts or disk.fstype == '':
            continue
        disk_name_arr = disk.device.split(':')
        disk_name = disk_name_arr[0]
        disk_info = psutil.disk_usage(disk.device)
        # 磁盘剩余空间，单位G
        free_disk_size = disk_info.free//1024//1024//1024
        # 当前磁盘使用率和剩余空间G信息
        info = "%s盘使用率：%s%%， 剩余空间：%iG \n" % (disk_name, str(disk_info.percent), free_disk_size)
        # print(info)
        # 拼接多个磁盘的信息
        content = content + info
    print(content)

    # return content
# def send_dingding_msg():            
                      

            
if __name__ == '__main__':
    # get_cpu_usage()
    # get_mem_usage()
    # get_net_usage()
    test_all_info()
