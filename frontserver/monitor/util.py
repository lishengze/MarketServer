from dingtalkchatbot.chatbot import DingtalkChatbot
import subprocess
import datetime
import threading
import psutil
import time

import subprocess
import os

def print_dict(dict_data):
    for index in dict_data:
        if type(dict_data[index]) is dict:
            print(str(index)+": ")
            print_dict(dict_data[index])
        else:
            print(str(index)+": ")
            print(dict_data[index])

def print_list(list_data):
    for item in list_data:
        print(item)

def print_info(data):
    if type(data) is dict:
        print_dict(data)
    elif type(data) is list:
        print_list(data)    
    else:
        print(data)

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

def get_disk_info(file_sys_path):
    disk_info = psutil.disk_usage(file_sys_path)
    # print(disk_info)
    return disk_info.percent

def get_disk_io_info():
    info = psutil.disk_io_counters(perdisk=True)

    for item in info:
        if info[item].read_time != 0 and info[item].write_time != 0:
            # read_io = info[item].read_bytes / 8 / 1024 / info[item].read_time
            # write_io = info[item].write_bytes / 8 / 1024 / info[item].write_time
            print(info[item])

            read_io = info[item].read_bytes / 8 / 1024 
            write_io = info[item].write_bytes / 8 / 1024    
            # print("%s, read_io: %f, write_io: %f" % (item, read_io, write_io))
        
    # # print_info(info)
    # read_io = info.read_bytes / 8 / 1024 / info.read_time
    # write_io = info.write_bytes / 8 / 1024 / info.write_time
    # return [read_io, write_io]

def get_process_disk_io(pid):
    p = psutil.Process(pid)
    info = p.io_counters()
    read_io = info.read_bytes / 1024 / 8
    write_io = info.write_bytes / 1024 / 8
    return [read_io, write_io]

def get_cpu_info_ps():
    # print("CPU 逻辑数量 %s" % psutil.cpu_count())
    # # CPU 物理核心 2 说明是双核超线程
    # print("CPU 物理核心 %s" % psutil.cpu_count(logical = False))
    # # scputimes(user=34319.75390625, system=18179.125, idle=934659.6875, interrupt=3766.7846422195435, dp
    # print("CPU 运行时间 " ,psutil.cpu_times())
    # print("CPU 使用率 " ,psutil.cpu_percent(interval=1))

    return psutil.cpu_percent(interval=1)

def get_cpu_usage():
    # counters1 = readCpuInfo()
    # # print(counters1)

    # time.sleep(0.1)

    # counters2 = readCpuInfo()
    # # print(counters2)

    # useage = calcCpuUsage(counters1, counters2)

    return get_cpu_info_ps()

def remove_all_empty_str(atom_data_list):
    i = 0
    while i < len(atom_data_list):
        if atom_data_list[i] == '':
            atom_data_list.remove(atom_data_list[i])
            i -= 1 
        i += 1    
    return atom_data_list

# data = subprocess.run(cmd, shell=True)
# print(data)

# ori_data = os.system(cmd)

# if type(ori_data) is list:
#     data_list = ori_data.split("\n")
#     for atom_data in data_list:
#         if len(atom_data) < 10:
#             continue

#         atom_data_list = atom_data.split(" ")
#         print(len(atom_data_list))
#         print(atom_data_list)

def get_disk_io_info_shell():
    try:
        result = []
        cmd = "iostat -x -d 1 1"

        val = os.popen(cmd)
        for atom_data in val.readlines():
            if len(atom_data) < 10:
                continue

            atom_data_list = atom_data.split(" ")
            atom_data_list = remove_all_empty_str(atom_data_list)

            # print(atom_data_list) 
            cur_data = []
            if atom_data_list[0] != "Device" and atom_data_list[0] != "Linux":
                rKB = 0
                r_wait = 0
                wKB = 0
                w_wait = 0
                util = 0

                if len(atom_data_list) < 15:
                    rKB = float(atom_data_list[5])
                    r_wait = float(atom_data_list[10])
                    wKB = float(atom_data_list[6])
                    w_wait = float(atom_data_list[11])
                    util = float(atom_data_list[len(atom_data_list)-1])
                elif len(atom_data_list) >15:
                    rKB = float(atom_data_list[1])
                    r_wait = float(atom_data_list[5])
                    wKB = float(atom_data_list[8])
                    w_wait = float(atom_data_list[11])
                    util = float(atom_data_list[len(atom_data_list)-1])
                    
                result.append([atom_data_list[0], rKB, r_wait, wKB, w_wait, util])

        return result

    except Exception as e:
        print("Exception get_disk_io_info_shell")
        print(e)

def get_process_disk_io_shell():
    try:
        result = {}
        cmd = "pidstat -d 1 1"

        val = os.popen(cmd)
        
        i = 0
        ave_data_start = False
        cur_data_start = False

        for atom_data in val.readlines():
            if len(atom_data) < 10:
                continue

            atom_data_list = atom_data.split(" ")
            atom_data_list = remove_all_empty_str(atom_data_list)

            print(atom_data_list)            

            i = i + 1

            if (not cur_data_start) and (atom_data_list[1] == "UID" or atom_data_list[2] == "UID"):
                print("cur_data_start")
                print(atom_data_list)   
                cur_data_start = True
                continue
        
            if cur_data_start and (atom_data_list[1] == "UID" or atom_data_list[2] == "UID"):
                print("ave_data_start")
                print(atom_data_list)                   
                ave_data_start = True
                continue

            if cur_data_start and ave_data_start:
                print("cur_data_start and ave_data_start")
                print(atom_data_list)

                result[atom_data_list[2]] = [float(atom_data_list[3]), float(atom_data_list[4])] 

                # print(atom_data_list[2], str(pid))
                # if atom_data_list[2] == str(pid):
                #     print(atom_data_list)
                #     result = [float(atom_data_list[3]), float(atom_data_list[4])]     

        print("result!")
        print(result)
        return result

    except Exception as e:
        print("Exception get_process_disk_io_shell")
        print(e)



class Test(object):
    def __init__(self):
        super().__init__()
        self.__name__ = "Test"

        # self.test_get_disk_info()

        # self.test_get_cpu_info_ps()

        self.test_get_disk_io_info()

    def test_get_disk_info(self):
        file_sys_path = "/"
        disk_info = get_disk_info(file_sys_path)
        print("file_sys_path: %s, Percent: %f \n" % (file_sys_path, disk_info))

    def test_get_cpu_info_ps(self):
        get_cpu_info_ps()

    def test_get_disk_io_info(self):
        # data = get_disk_io_info_shell()
        # print(data)

        get_process_disk_io_shell()

        while (True):
            # get_disk_io_info_shell()
            get_process_disk_io_shell()
            time.sleep(1)
            print("\n\n")

if __name__ == '__main__':
    test_obj = Test()
