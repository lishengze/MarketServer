from dingtalkchatbot.chatbot import DingtalkChatbot
import subprocess
import datetime
import time
import threading
import psutil


def get_process():
    return ["front_server", "demo4risk", "demo4quote", "python3"]

class Write(object):
    def __init__(self):

        self._log_file = "write.log"
        self._logger = open(self._log_file, 'w')
        self._logger.close()

    def __del__(self):
        self._logger.close()
        print("__del__")

    def log_info(self, msg):
        print(msg)
        self._logger = open(self._log_file, 'a')
        self._logger.write(msg+"\n")
        self._logger.close()

def write_main():
    write_obj = Write()
    while (True):
        write_obj.log_info("1234567890")

if __name__ == '__main__':
    write_main()
