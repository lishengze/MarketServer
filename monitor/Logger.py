import logging
import logging.handlers
import datetime
import time


class Logger(object):
    def __init__(self, all_file_Name="log/all.log", error_file_name="log/error.log"):
        LOG_FORMAT = "%(pathname)s-%(lineno)s - %(asctime)s - %(levelname)s - %(message)s"
        DATE_FORMAT = "%Y/%m/%d %H:%M:%S"    
        logging.basicConfig(level=logging.DEBUG, format=LOG_FORMAT, datefmt=DATE_FORMAT)

        self._logger = logging.getLogger('user_logger')
        self._logger.setLevel(logging.DEBUG)

        # detail_handler = logging.handlers.TimedRotatingFileHandler('log/all.log', when='midnight', interval=1, backupCount=7, atTime=datetime.time(0, 0, 0, 0))

        detail_handler = logging.handlers.RotatingFileHandler(all_file_Name, maxBytes=8*1024*1024*500, backupCount=3)
        
        detail_handler.setFormatter(logging.Formatter("%(asctime)s - %(levelname)s - %(message)s"))

        # error_handler = logging.FileHandler('log/error.log')

        error_handler = logging.handlers.RotatingFileHandler(error_file_name, maxBytes=8*1024*1024*500, backupCount=3)

        error_handler.setLevel(logging.WARNING)
        error_handler.setFormatter(logging.Formatter(fmt="%(asctime)s - %(levelname)s - %(filename)s[:%(lineno)d] - %(message)s"))

        self._logger.addHandler(detail_handler)
        self._logger.addHandler(error_handler)        

    def Debug(self, info):
        self._logger.debug(info)

    def Info(self, info):
        self._logger.info(info)

    def Warning(self, info):
        self._logger.warning(info)

    def Error(self, info):
        self._logger.error(info)

    def Critical(self, info):
        self._logger.critical(info)        

def test_logger():
    logger = Logger();  

    logger.Debug("test debug %s" % (time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))
    logger.Info("test info %s" % (time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))
    logger.Warning("test warning %s" % (time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))
    logger.Error("test error %s" % (time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))
    logger.Critical("test critical %s" % (time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())))

def main():
    test_logger()

if __name__ == '__main__':
    main()        