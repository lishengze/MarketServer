from dingtalkchatbot.chatbot import DingtalkChatbot
import subprocess
import datetime
import threading
import psutil

from util import *

def get_process():
    return ["front_server", "demo4risk", "demo4quote"]

class MonitorUtrade(object):
    def __init__(self):

        self._program_last_status = {}
        self._program_curr_status = {}
        self._program_pid = {}

        process_list = get_process()

        for process_name in process_list:
            self._program_last_status[process_name] = -1
            self._program_curr_status[process_name] = -1
            self._program_pid[process_name] = []
            
        self.dingding = DingtalkChatbot("https://oapi.dingtalk.com/robot/send?access_token=4cf0db490004f0924c0e2a8e785680384117ca2c3d26ec44aa3da1af5b4d496b")
        self._check_secs = 10
        self._log_file = "log/monitor.log"
        self._logger = open(self._log_file, 'w')
        self._logger.close()

    def __del__(self):
        self._logger.close()
        print("__del__")

    def launch(self):
        self.set_timer()

    def log_info(self, msg):
        print(msg)
        self._logger = open(self._log_file, 'a')
        self._logger.write(msg+"\n")
        self._logger.close()

    def output_program_info(self, program):
        try:
            cmd_str = "ps -aux|grep " + program
            result_str = subprocess.getoutput(cmd_str) + "\n"
            self.log_info(result_str)
            self.send_dingding_msg(result_str)

            result_list = result_str.split("\n")
            self.get_opu_pid(result_list, True)            
        except Exception as e:
            print("Exception output_program_info")
            print(e)

    def send_dingding_msg(self, msg):
        try:
            msg = 'msg ' + msg
            self.dingding.send_text(msg, False)        
        except Exception as e:            
            print("Exception send_dingding_msg")
            print(e)

        
    def get_opu_pid(self, ori_str_list, out=True):
        result = 0
        try:
            for info_str in ori_str_list:
                # if out:
                    # self.send_dingding_msg(info_str)
                    # self.log_info(info_str)
                if info_str.find("python") != -1 and info_str.find("OPU") != -1:
                    info_list = info_str.split(" ")
                    trans_list = []
                    for item in info_list:
                        if item != '':
                            trans_list.append(item)

                    result = int(trans_list[1])  
                    #if out:
                        # self.send_dingding_msg(str(trans_list))
                        # self.log_info(str(trans_list)+"\n")
                        # self.log_info(str(result)+"\n")
        except Exception as e:
            print("Exception get_opu_pid")
            print(e)

        return result

    def get_status_manually(self, program_name):
        result = 0
        try:
            cmd_str = "ps -aux|grep " + program_name
            result_str = subprocess.getoutput(cmd_str)

            result_list = result_str.split("\n")
            result = self.get_opu_pid(result_list)      
        except Exception as e:
            print("Exception get_status_manually")
            print(e)
   
        return result

    def get_status(self, program_name=""):      
        result = 0      
        try:
            self._program_pid[program_name] = []
            if program_name.find("OPU") != -1:
                result = self.get_status_manually(program_name)
                if result != 0:
                    self._program_pid[program_name].append(result)
                    result = 1
            else:
                for proc in psutil.process_iter():
                    if proc.name() == program_name:
                        self._program_pid[program_name].append(proc.pid)
                        result = 1      
        except Exception as e:
            print("Exception get_status")
            print(e)              

        return result

    def get_usage_info(self, program):
        msg = "get_usage_info"
        try:
            mem_info = 0
            cpu_info = 0
            for program_id in self._program_pid[program]:
                process = psutil.Process(program_id)
                mem_info += process.memory_percent()
                cpu_info += get_process_cpu_usage(process)
            msg = get_datetime_str() + (" %12s mem_usage: %5s, cpu_usage: %5s \n" % (program, str(mem_info), str(cpu_info)))
        except Exception as e:
            print("Exception get_usage_info")
            print(e)

        return msg

    def get_disk_info(self):
        result_str = "get_disk_info"
        try:
            cmd_str = "df -h"
            result_str = get_datetime_str() + "\n" + subprocess.getoutput(cmd_str) + "\n"
        except Exception as e:
            print("Exception get_disk_info")
            print(e)

        return result_str

    def set_curr_status(self):
        try:
            for program in self._program_curr_status: 
                self._program_curr_status[program] = self.get_status(program)
            
            self.log_info(str(self._program_curr_status))
            self.log_info(str(self._program_pid)+"\n")            
        except Exception as e:
            print("Exception set_curr_status")
            print(e)

    def record_usage_info(self):        
        try:
            record_msg = ""
            for program in self._program_curr_status: 
                if self._program_curr_status[program] == 1:
                    msg = self.get_usage_info(program)
                    record_msg += msg
            
            mem_usage = get_mem_usage()
            cpu_usage = get_cpu_usage()
            all_cpu_info = get_datetime_str() + (" [All] cpu usage: %s \n" % (cpu_usage))
            all_mem_info = get_datetime_str() + (" [All] mem usage: %s \n" % (mem_usage))

            if mem_usage > 85:
                msg = ("[All] mem usage: %s, too high!, cpu usage: %s" %(mem_usage, cpu_usage))
                self.send_dingding_msg(msg)
            
            record_msg += all_cpu_info
            record_msg += all_mem_info
            # record_msg += self.get_disk_info()
            self.log_info(record_msg)     
                    
        except Exception as e:
            print("Exception record_usage_info")
            print(e)


    def check_program_status(self):
        try:
            for program in self._program_last_status:
                if self._program_last_status[program] == 0 and self._program_curr_status[program] == 1:
                    msg = get_datetime_str() + "  Start: " + str(program) + ' start! \n'
                    self.log_info(msg)
                    self.send_dingding_msg(msg)
                    # self.output_program_info(program)

                if self._program_last_status[program] == 1 and self._program_curr_status[program] == 0:
                    msg = get_datetime_str() + "  Warining: " + str(program) + ' crashed! \n'
                    self.log_info(msg)
                    self.send_dingding_msg(msg)   
                    #self.output_program_info(program)

                self._program_last_status[program] = self._program_curr_status[program]
        except Exception as e:
            print("Exception check_program_status")
            print(e)
         
    def timer_func(self):
        try:
            self.set_curr_status()
            self.record_usage_info()
            self.check_program_status()

            self.set_timer()
        except Exception as e:
            print("Exception timer_func")
            print(e)

    def set_timer(self):
        try:
            # print("Set Timer")
            timer = threading.Timer(self._check_secs, self.timer_func)
            # timer = threading.Timer(self._check_secs, self.print_data)
            timer.start()
        except Exception as e:
            print("Exception set_timer")
            print(e)

    def print_data(self):
    
        print("Test")

        self.set_timer()        
        
def start_monitor():
    try:
        monitor_obj = MonitorUtrade()
        monitor_obj.launch()
    except Exception as e:
        print("Exception start_monitor")
        print(e)


if __name__ == "__main__":    
    start_monitor()
    # basic_test()
    # test_psutil()
    # get_disk_info()
    # pass    