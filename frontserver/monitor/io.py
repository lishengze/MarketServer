import os
import re
import sys
import time
from string import strip

sys_proc_path = '/proc/'
re_find_process_number = '^\d+$'

def collect_info():
	_tmp={}
	re_find_process_dir=re.compile(re_find_process_number)
	for i in os.listdir(sys_proc_path):
		if  re_find_process_dir.match(i):
			process_name=open("%s%s/stat" % (sys_proc_path,i),"rb").read().split(" ")[1]
			rw_io = open("%s%s/io" % (sys_proc_path, i), "rb").readlines()
			for _info in rw_io:
				cut_info = strip(_info).split(':')
				if strip(cut_info[0]) == "read_bytes":
					read_io = int(strip(cut_info[1]))
				if strip(cut_info[0]) == "write_bytes":
					write_io = int(strip(cut_info[1]))
			_tmp[i] = {"name":process_name, "read_bytes":read_io, "write_bytes":write_io}
	#print _tmp
	return _tmp

def main(_sleep_time, _list_num):
	_sort_read_dict = {}
	_sort_write_dict = {}
	process_info_list_frist = collect_info()
	time.sleep(_sleep_time)
	process_info_list_second = collect_info()
	for loop in process_info_list_second.keys():
		second_read_v = process_info_list_second[loop]["read_bytes"]
		second_write_v = process_info_list_second[loop]["write_bytes"]
		try:
			frist_read_v = process_info_list_frist[loop]["read_bytes"]
		except:
			frist_read_v = 0
		try:
			frist_write_v = process_info_list_frist[loop]["write_bytes"]
		except:
			frist_write_v = 0
		_sort_read_dict[loop] = second_read_v - frist_read_v
		_sort_write_dict[loop] = second_write_v - frist_write_v
	#print  _sort_read_dict
	sort_read_dict = sorted(_sort_read_dict.items(),key=lambda _sort_read_dict:_sort_read_dict[1],reverse=True)
	sort_write_dict = sorted(_sort_write_dict.items(),key=lambda _sort_write_dict:_sort_write_dict[1],reverse=True)
	print "pid     process     read(bytes) pid     process     write(btyes)"
	for _num in range(_list_num):
	#	print "%s%s" % (_num,_list_num)
	#	print "%s" % sort_read_dict[_num]
		read_pid = sort_read_dict[_num][0]
		write_pid = sort_write_dict[_num][0]
		res = "%s" % read_pid
		res += " " * (8 - len(read_pid)) + process_info_list_second[read_pid]["name"]
		res += " " * (12 - len(process_info_list_second[read_pid]["name"])) + "%s" % sort_read_dict[_num][1]
		res += " " * (12 - len("%s" % sort_read_dict[_num][1])) + write_pid
		res += " " * (8 - len(write_pid)) + process_info_list_second[write_pid]["name"]
		res += " " * (12 - len("%s" % process_info_list_second[write_pid]["name"])) + "%s" % sort_write_dict[_num][1]
		print res
	print "\n" * 1

if __name__ == '__main__':
	try:
		_sleep_time = sys.argv[1]
	except:
		_sleep_time = 3
	try:
		_num = sys.argv[2]
	except:
		_num = 3
	try:
		loop = sys.argv[3]
	except:
		loop = 1
	for i in range(int(loop)):
		main(int(_sleep_time), int(_num))