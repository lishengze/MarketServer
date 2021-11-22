import sys
import time
import redis
import collections
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.Qt import QThread, pyqtSignal, QWidget, QFrame
from PyQt5.Qt import QGridLayout, QLabel, QComboBox
from PyQt5.QtWidgets import QApplication, QMainWindow
from concurrent import futures
import time
import math
import logging
import grpc
import quote_data_pb2
import risk_controller_pb2
import risk_controller_pb2_grpc
import stream_engine_pb2
import stream_engine_pb2_grpc
from google.protobuf import empty_pb2
import struct
import json

MIX_EXCHANGE_NAME = '_bcts_'

def decode_decimal(v):
    return v.base * 1.0 / (10**v.prec)

def get_combox_data(combox):
    ret = set()
    for i in range(combox.count()):
        ret.add(combox.itemData(i))
    return ret

def CallChangeStreamEngineParams(addr, params):
    with grpc.insecure_channel(addr) as channel:
        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)
        request = stream_engine_pb2.SetParamsReq()
        request.depth = params['depth']
        request.frequency = params['frequency']
        request.precise = params.get('precise', 0)
        request.symbol = params.get('symbol', "")
        request.raw_frequency = params['raw_frequency']
        stub.SetParams(request)

def CallGetStreamEngineParams(addr):
    with grpc.insecure_channel(addr) as channel:
        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)
        request = stream_engine_pb2.GetParamsReq()
        resp = stub.GetParams(request)
        return resp

def CallGetRiskControlParams(addr):
    with grpc.insecure_channel(addr) as channel:
        stub = risk_controller_pb2_grpc.RiskControllerStub(channel)
        resp = stub.GetParams(empty_pb2.Empty())
        return resp

def CallOtcQuery(addr):
    with grpc.insecure_channel(addr) as channel:
        stub = risk_controller_pb2_grpc.RiskControllerStub(channel)
        request = risk_controller_pb2.QuoteRequest()
        request.symbol = "BTC_USDT"
        request.direction = 0
        request.amount = 0.025
        request.turnover = 0
        resp = stub.OtcQuote(request)
        return resp

def CallGetKlinesStreamEngine(addr):
    with grpc.insecure_channel(addr) as channel:
        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)

        index = 1611687600

        '''
        request = stream_engine_pb2.GetKlinesRequest()
        request.exchange = "HUOBI"
        request.symbol = "BTC_USDT"
        request.resolution = 60
        request.start_time = index
        request.end_time = index
        resp = stub.GetKlines(request)
        for kline in resp.klines:
            if kline.index == index:
                print(kline)

        request = stream_engine_pb2.GetKlinesRequest()
        request.exchange = "OKEX"
        request.symbol = "BTC_USDT"
        request.resolution = 60
        request.start_time = index
        request.end_time = index
        resp = stub.GetKlines(request)
        for kline in resp.klines:
            if kline.index == index:
                print(kline)

        request = stream_engine_pb2.GetKlinesRequest()
        request.exchange = "BINANCE"
        request.symbol = "BTC_USDT"
        request.resolution = 60
        request.start_time = index
        request.end_time = index
        resp = stub.GetKlines(request)
        for kline in resp.klines:
            if kline.index == index:
                print(kline)
        '''

        request = stream_engine_pb2.GetKlinesRequest()
        request.exchange = ""
        request.symbol = "BTC_USDT"
        request.resolution = 3600
        request.start_time = 1611684000
        request.end_time = 1611687500
        resp = stub.GetKlines(request)
        print(resp)

        return resp

class GrpcStreamThread(QThread):
    def __init__(self, addr, name, parent=None):
        super().__init__(parent)
        self.name = name
        self.addr = addr
        self.channel = None
        self.stopped = False

    def run(self):
        while not self.stopped:
            try:
                print(self.name, "connect to", self.addr)
                self.channel = grpc.insecure_channel(self.addr)
                self.run_grpc(self.channel)
            except Exception as ex:
                print(self.name, "error connect to", self.addr, ex)
                continue

    def run_grpc(self, channel):
        pass

    def stop_stream(self):
        if self.channel:
            self.channel.close()
            self.channel = None
        self.stopped = True
        self.exit()
        self.stopped = False

class QueryRiskControlThread(GrpcStreamThread):
    breakSignal = pyqtSignal(quote_data_pb2.MarketStreamDataWithDecimal)
  
    def __init__(self, addr, parent=None):
        super().__init__(addr, 'QueryApiThread', parent)
  
    def run_grpc(self, channel):
        stub = risk_controller_pb2_grpc.RiskControllerStub(channel)        
        responses = stub.ServeMarketStream4Client(empty_pb2.Empty())
        for resp in responses:
            for quote in resp.quotes:
                self.breakSignal.emit(quote)

class SubscribeKlineThread(GrpcStreamThread):
    breakSignal = pyqtSignal(stream_engine_pb2.GetKlinesResponse)
  
    def __init__(self, addr, parent=None):
        super().__init__(addr, 'SubscribeKlineThread', parent)
  
    def run_grpc(self, channel):
        print("SubscribeKlineThread running...")
        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)
        responses = stub.GetLast(stream_engine_pb2.GetKlinesRequest())
        for resp in responses:
            print(resp)
            #print(struct.unpack("=QQHQHQHQHQH", resp.data))
            pass

class QueryStreamEngineConfigThread(QThread):
    breakSignal = pyqtSignal(str)

    def __init__(self, addr, parent=None):
        super().__init__(parent)
        self.addr = addr

    def run(self):
        while True:
            try:
                resp = CallGetStreamEngineParams(self.addr)
                self.breakSignal.emit(resp.json_data)
            except:
                pass
            time.sleep(5)

class QueryRiskControlConfigThread(QThread):
    breakSignal = pyqtSignal(risk_controller_pb2.GetParamsResponse)

    def __init__(self, addr, parent=None):
        super().__init__(parent)
        self.addr = addr

    def run(self):
        while True:
            try:
                resp = CallGetRiskControlParams(self.addr)
                self.breakSignal.emit(resp)
            except:
                pass
            time.sleep(5)

class QueryStreamEngineThread(GrpcStreamThread):
    breakSignal = pyqtSignal(dict)
  
    def __init__(self, exchange, symbol, addr, parent=None):
        super().__init__(addr, 'QueryRawThread', parent)
        self.exchange = exchange
        self.symbol = symbol
        self.cache = collections.defaultdict(dict)
        self.cache4mix = collections.defaultdict(dict)

    def set_params(self, exchange, symbol):
        self.exchange = exchange
        self.symbol = symbol

    def __decode_one_resp(self, resp):
        data = resp.data
        while data:
            fields = data.split(b';', 2)
            length = fields[0]
            datatype = fields[1]
            subdata = fields[2];
            #print(length[:20], datatype[:20], subdata[:20])
            
            l = int(bytes.decode(length))
            dt = int(bytes.decode(datatype))
            binary = subdata[:l]
            if dt == 1:
                quote = quote_data_pb2.MarketStreamDataWithDecimal()
                quote.ParseFromString(binary)
                self.__on_quote(quote)
            elif dt == 2:
                trade = quote_data_pb2.TradeWithDecimal()
                trade.ParseFromString(binary)
                self.__on_trade(trade)
            data = subdata[l:]
                #print(data[:20])

    def __on_trade(self, trade):
        #print(trade)
        pass

    def __on_quote(self, quote):
        def refresh_cache(cache, key, depths):
            cache[key] = {}
            for depth in depths:
                cache[key][decode_decimal(depth.price)] = decode_decimal(depth.volume)

        def update_cache(cache, key, depths):
            for depth in depths:
                price = decode_decimal(depth.price)
                volume = decode_decimal(depth.volume)
                #print(key, price, volume)
                if volume < 0.00001:
                    try:
                        del cache[key][price]
                    except:
                        pass
                else:
                    cache[key][price] = volume

        cache = None
        if quote.exchange == MIX_EXCHANGE_NAME:
            if quote.symbol != self.symbol:
                return
            cache = self.cache4mix
        else:
            if quote.exchange != self.exchange or quote.symbol != self.symbol:
                return
            cache = self.cache

        #print(quote.exchange, quote.symbol, quote.is_snap)
        cache['exchange'] = quote.exchange
        cache['symbol'] = quote.symbol
        # 全量
        if quote.is_snap:
            refresh_cache(cache, 'ask', quote.asks)
            refresh_cache(cache, 'bid', quote.bids)
        # 增量
        else:
            update_cache(cache, 'ask', quote.asks)
            update_cache(cache, 'bid', quote.bids)
        self.breakSignal.emit(cache)

    def run_grpc(self, channel):
        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)
        request = stream_engine_pb2.SubscribeQuoteReq()
        request.symbol = self.symbol
        responses = stub.SubscribeQuoteInBinary(request)
        for resp in responses:
            self.__decode_one_resp(resp)

    def run_grpc1(self, channel):
        def refresh_cache(cache, key, depths):
            cache[key] = {}
            for depth in depths:
                cache[key][decode_decimal(depth.price)] = decode_decimal(depth.volume)

        def update_cache(cache, key, depths):
            for depth in depths:
                price = decode_decimal(depth.price)
                volume = decode_decimal(depth.volume)
                #print(key, price, volume)
                if volume < 0.00001:
                    try:
                        del cache[key][price]
                    except:
                        pass
                else:
                    cache[key][price] = volume

        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)
        request = stream_engine_pb2.SubscribeQuoteReq()
        request.exchange = self.exchange
        request.symbol = self.symbol
        responses = stub.SubscribeQuote(request)

        cache = collections.defaultdict(dict)
        for resp in responses:
            for quote in resp.quotes:
                #print(quote.is_snap)
                # 全量
                if quote.is_snap:
                    refresh_cache(cache, 'ask', quote.asks)
                    refresh_cache(cache, 'bid', quote.bids)
                    #print(cache)
                # 增量
                else:
                    update_cache(cache, 'ask', quote.asks)
                    update_cache(cache, 'bid', quote.bids)
                self.breakSignal.emit(cache)

class AskBidWidget(object):
    def __init__(self, depth):
        def _create_pricelevel(grid, row, label_prefix, level, is_first):
            label = QLabel(label_prefix + str(level))
            if is_first:
                font = QtGui.QFont()
                #font.setFamily("Arial")
                #font.setPointSize(18)
                font.setBold(True)
                font.setWeight(75)
                label.setFont(font)
            grid.addWidget(label, row, 0)
            lbl_price = QLabel("0.00")
            grid.addWidget(lbl_price, row, 1)
            lbl_volumes = QLabel("0")
            grid.addWidget(lbl_volumes, row, 2)
            return lbl_price, lbl_volumes

        self.depth = depth
        self.asks = []
        self.bids = []
        self.grid = QGridLayout()

        # 构造sell档位
        for i in range(self.depth):
            lbl_price, lbl_volumes = _create_pricelevel(self.grid, i, "Ask", self.depth-i, self.depth==i+1)
            self.add_ask(lbl_price, lbl_volumes)
        label = QLabel("---")
        self.grid.addWidget(label, self.depth, 0)
        # 构造buy档位
        for i in range(self.depth):
            lbl_price, lbl_volumes = _create_pricelevel(self.grid, i + self.depth + 1, "Bid", i+1, i==0)
            self.add_bid(lbl_price, lbl_volumes)
        self.grid.setColumnStretch(0, 1)
        self.grid.setColumnStretch(1, 1)
        self.grid.setColumnStretch(2, 6)

    def add_ask(self, lbl_price, lbl_volumes):
        self.asks.insert(0, (lbl_price, lbl_volumes))

    def add_bid(self, lbl_price, lbl_volumes):
        self.bids.append((lbl_price, lbl_volumes))

    def bind_data(self, ask_data, bid_data):
        for (lbl_price, lbl_volumes) in self.asks:
            lbl_price.setText("")
            lbl_volumes.setText("")
        for (lbl_price, lbl_volumes) in self.bids:
            lbl_price.setText("")
            lbl_volumes.setText("")

        for i, (price, volume) in enumerate(ask_data):            
            if i >= len(self.asks):
                break
            (lbl_price, lbl_volumes) = self.asks[i]
            lbl_price.setText( str(price) )
            lbl_volumes.setText( str(volume) )
        for i, (price, volume) in enumerate(bid_data):
            if i >= len(self.bids):
                break
            (lbl_price, lbl_volumes) = self.bids[i]
            lbl_price.setText( str(price) )
            lbl_volumes.setText( str(volume) )

def init_logger(name):
    fh = logging.FileHandler(name, mode='w')
    fh.setLevel(logging.DEBUG)
    logger = logging.getLogger(name)
    logger.propagate = False
    #formatter = logging.Formatter("%(asctime)s - %(filename)s[line:%(lineno)d] - %(levelname)s: %(message)s")
    #fh.setFormatter(formatter)
    logger.addHandler(fh)
    return logger

def log_depth(logger, symbol, asks, bids):
    logger.info('[{}] {}'.format(symbol, time.ctime()))
    for (s_price, s_desc) in asks:
        logger.info('asks: {} {}'.format(s_price, s_desc))
    for (s_price, s_desc) in bids:
        logger.info('bids: {} {}'.format(s_price, s_desc))
    logger.info('')
    logger.info('')

class DisplayWidget(QWidget):
    DISPLAY_DEPTH = 20 # 这个值固定不变
    DEPTH = 20
    FREQUENCY = 1
    RAW_FREQUENCY = 1
    #STREAMENGINE_ADDR = "172.25.3.207:9000"
    #RISKCONTROL_ADDR = "172.25.3.207:9900"
    
    def __init__(self, env_name=""):
        super().__init__()

        #ret = CallGetStreamEngineParams(self.STREAMENGINE_ADDR)
        #print(ret)

        #ret = CallGetRiskControlParams(self.RISKCONTROL_ADDR)
        #print(ret)

        #ret = CallOtcQuery(self.RISKCONTROL_ADDR)
        #print(ret)

        #ret = CallGetKlinesStreamEngine(self.STREAMENGINE_ADDR)
        #exit(1)

        self.current_symbol = ''
        self.current_exchange = ''

        # 初始化日志对象
        logging.basicConfig(level=logging.INFO)
        self.logger_riskcontrol = init_logger('riskcontrol.log')
        self.logger_streamengine = init_logger('streamengine.log')

        #default qa
        self.STREAMENGINE_ADDR = "118.193.35.160:8110"
        self.RISKCONTROL_ADDR = "118.193.35.160:8111"

        #prd
        if env_name == "-prd":        
            self.STREAMENGINE_ADDR = "16.162.143.211:9110"
            self.RISKCONTROL_ADDR = "16.162.143.211:9111"

        #stg
        if env_name == "-stg":
            self.STREAMENGINE_ADDR = "18.162.52.222:8110"
            self.RISKCONTROL_ADDR = "18.162.52.222:8111"

        # 初始化ui
        self.initUI()
        
        # 启动获取风控后行情接口
        self.riskctl_thread = QueryRiskControlThread(self.RISKCONTROL_ADDR)
        self.riskctl_thread.breakSignal.connect(self.update_riskctl_data)
        self.riskctl_thread.start()

        # 原始行情接口需要动态获取
        self.stream_thread = QueryStreamEngineThread('', '', self.STREAMENGINE_ADDR)
        self.stream_thread.breakSignal.connect(self.update_stream_data)
        self.stream_thread.start()

        # 查询服务器配置参数
        self.cfg_thread = QueryStreamEngineConfigThread(self.STREAMENGINE_ADDR)
        self.cfg_thread.breakSignal.connect(self.update_config_data)
        self.cfg_thread.start()

        # 查询风控参数
        self.riskcontrol_status_thread = QueryRiskControlConfigThread(self.RISKCONTROL_ADDR)
        self.riskcontrol_status_thread.breakSignal.connect(self.update_riskcontrol_params)
        self.riskcontrol_status_thread.start()

        # 启动K线更新接口
        #self.kline_thread = SubscribeKlineThread(self.STREAMENGINE_ADDR)
        #self.kline_thread.start()

    def _create_combobox(self, options, init_val=None):
        ret = QComboBox(self)
        ret.addItems([str(d) for d in options])
        if init_val != None:
            for i, d in enumerate(options):
                if math.isclose(d, init_val, rel_tol=1e-5):
                    ret.setCurrentIndex(i)
                    break
        return ret

    def initUI(self):
        ctl_panel = QWidget()
        ctl_grid = QGridLayout()
        ctl_grid.setAlignment(QtCore.Qt.AlignTop)
        label = QLabel("StreamEngine设置:")
        ctl_grid.addWidget(label, 0, 0)

        label = QLabel("设置原始频率")
        ctl_grid.addWidget(label, 1, 0)
        self.frequency_lbl = QLabel()
        ctl_grid.addWidget(self.frequency_lbl, 1, 1)

        label = QLabel("设置精度")
        ctl_grid.addWidget(label, 2, 0)
        self.precise_lbl = QLabel()
        ctl_grid.addWidget(self.precise_lbl, 2, 1)

        label = QLabel("设置聚合频率")
        ctl_grid.addWidget(label, 3, 0)
        self.mixfrequency_lbl = QLabel()
        ctl_grid.addWidget(self.mixfrequency_lbl, 3, 1)

        label = QLabel("设置聚合深度")
        ctl_grid.addWidget(label, 4, 0)
        self.mixdepth_lbl = QLabel()
        ctl_grid.addWidget(self.mixdepth_lbl, 4, 1)

        label = QLabel("设置交易手续费")
        ctl_grid.addWidget(label, 5, 0)
        self.fee_lbl = QLabel()
        ctl_grid.addWidget(self.fee_lbl, 5, 1)

        label = QLabel("-------------------")
        ctl_grid.addWidget(label, 6, 0)
        label = QLabel("RiskController设置:")
        ctl_grid.addWidget(label, 7, 0)
        ctl_panel.setLayout(ctl_grid) 

        # stream engine数据显示区域
        streamengine_panel = QWidget()
        self.streamengine_widget = AskBidWidget(self.DISPLAY_DEPTH)
        streamengine_panel.setLayout(self.streamengine_widget.grid) 

        # api数据显示区域
        api_panel = QWidget()
        self.api_widget = AskBidWidget(self.DISPLAY_DEPTH)
        api_panel.setTabOrder
        api_panel.setLayout(self.api_widget.grid)

        # 原始数据显示区域
        raw_panel = QWidget()
        self.raw_widget = AskBidWidget(self.DISPLAY_DEPTH)
        raw_panel.setLayout(self.raw_widget.grid)   

        # 主layout区域
        wlayout = QGridLayout()
        # 0行 - 0列
        self.symbol_combox = QComboBox(self)
        self.symbol_combox.activated[str].connect(self.symbol_changed)
        wlayout.addWidget(self.symbol_combox, 0, 0)
        # 0行 - 1列
        wlayout.addWidget(QLabel("风控前行情"), 0, 1)
        # 0行 - 2列
        wlayout.addWidget(QLabel("风控后行情"), 0, 2)
        # 0行 - 3列
        self.exchange_combox = QComboBox(self)
        self.exchange_combox.activated[str].connect(self.exchange_changed)
        wlayout.addWidget(self.exchange_combox, 0, 3)
        # 1行 - 0列
        wlayout.addWidget(ctl_panel, 1, 0)
        # 1行 - 1列
        wlayout.addWidget(streamengine_panel, 1, 1)
        # 1行 - 2列
        wlayout.addWidget(api_panel, 1, 2)
        # 1行 - 3列
        wlayout.addWidget(raw_panel, 1, 3)
        wlayout.setColumnStretch(0, 1)
        wlayout.setColumnStretch(1, 6)
        wlayout.setColumnStretch(2, 6)
        wlayout.setColumnStretch(3, 2)

        # --------------
        self.setLayout(wlayout)
        self.move(300, 150)
        self.setWindowTitle('聚合行情展示')
        self.show()

    def log_riskcontrol(self, symbol, asks, bids):
        log_depth(self.logger_riskcontrol, symbol, asks, bids)

    def log_streamengine(self, symbol, asks, bids):
        log_depth(self.logger_streamengine, symbol, asks, bids)

    def update_riskctl_data(self, msg):
        def _make_data(depths):
            ret = []
            for depth in depths:
                # 价位数据
                txt_price = decode_decimal(depth.price)
                # 挂单数据
                #total = depth.volume
                #desc = ""
                #for exchange, volume in depth.data.items():
                #    desc += "{0}:{1:.04f},".format(exchange, volume)
                #txt_volumes = "{0:.04f}(".format(total) + desc + ")"
                txt_volumes = '{0}'.format(decode_decimal(depth.volume))
                # 添加数据
                ret.append((txt_price, txt_volumes))
            return ret
        if msg.symbol != self.current_symbol:
            return
        ask_data = _make_data(msg.asks)
        bid_data = _make_data(msg.bids)
        self.log_riskcontrol(msg.symbol, ask_data, bid_data)
        self.api_widget.bind_data(ask_data, bid_data)

    '''
    def update_streamengine_data(self, msg):
        try:
            def _make_data(depths):
                ret = []
                for depth in depths:
                    # 价位数据
                    txt_price = decode_decimal(depth.price)
                    # 挂单数据
                    total = 0
                    desc = ""
                    for exchange, volume in depth.data.items():
                        #self._add_exchange(data.exchange, msg.symbol)
                        total += decode_decimal(volume)
                        desc += "{0}:{1},".format(exchange, decode_decimal(volume))
                    txt_volumes = "{0:.4f} - ".format(total) + desc
                    # 添加数据
                    ret.append((txt_price, txt_volumes))
                return ret

            # 添加品种
            #self._add_symbol(msg.symbol)
            if msg.symbol != self.current_symbol:
                return
            ask_data = _make_data(msg.asks)
            bid_data = _make_data(msg.bids)
            self.log_streamengine(msg.symbol, ask_data, bid_data)
            self.streamengine_widget.bind_data(ask_data, bid_data)
        except Exception as ex:
            print(ex)
    '''

    def update_stream_data(self, msg):
        def _make_data(depths, is_ask):
            ret = []
            keys = sorted(depths.keys())
            if not is_ask:
                keys = keys[::-1]
            for price in keys:
                ret.append( ((price), (depths[price])) )
            return ret

        ask_data = _make_data(msg['ask'], True)
        bid_data = _make_data(msg['bid'], False)
        if msg['exchange'] == MIX_EXCHANGE_NAME:
            self.streamengine_widget.bind_data(ask_data, bid_data)
            self.log_streamengine(msg["symbol"], ask_data, bid_data)
        else:
            #print(msg)
            self.raw_widget.bind_data(ask_data, bid_data)
        
    def update_riskcontrol_params(self, msg):
        for k, v in msg.watermarks.items():
            print('watermark', k, decode_decimal(v))

        for k, v in msg.accounts.items():
            print('accounts', k, v)

        for k, v in msg.configuration.items():
            print("configuration", k, v)
        
        print('---------------------')

    def update_config_data(self, msg):
        # 解析完整的配置信息
        self.configs = json.loads(msg)
        #print(self.configs)

        # 解析符号和交易所信息，用于列表展示
        symbols = {}
        for symbol, symbol_configs in self.configs.items():
            symbols[symbol] = list(symbol_configs['exchanges'].keys())

        # symbol_combox
        if get_combox_data(self.symbol_combox) != set(symbols.keys()):
            self.symbol_combox.clear()
            self.symbol_combox.addItems(list(symbols.keys()))
            if self.current_symbol in symbols: # 重新设置选中项
                self.symbol_combox.setCurrentText(self.current_symbol)
            elif symbols: # 默认设置第一个
                self.current_symbol = list(symbols.keys())[0]
                self.symbol_combox.activated[str].emit(self.current_symbol)

        # exchange_combox
        exchanges = symbols[self.current_symbol]
        if get_combox_data(self.exchange_combox) != set(exchanges):
            self.exchange_combox.clear()
            self.exchange_combox.addItems(exchanges)
            if self.current_exchange in exchanges: # 重新设置选中项
                self.exchange_combox.setCurrentText(self.current_exchange)
            elif exchanges: # 默认设置第一个
                self.current_exchange = exchanges[0]
                self.exchange_combox.activated[str].emit(self.current_exchange)

        # 设置其他
        self.set_others()

    def set_others(self):
        cfg = self.configs[self.current_symbol]
        self.precise_lbl.setText(str(cfg['precise']))
        self.frequency_lbl.setText(str(cfg['frequency']))
        self.mixdepth_lbl.setText(str(cfg['depth']))
        self.mixfrequency_lbl.setText(str(cfg['frequency']))
        fee = cfg['exchanges'][self.current_exchange]    
        self.fee_lbl.setText('{},{:.03f},{:.03f}'.format(fee['fee_type'], fee['fee_maker'], fee['fee_taker']))

    def symbol_changed(self, symbol):
        #print()
        #print()
        #print('symbol_changed', symbol)
        self.current_symbol = symbol

        #self.streamengine_thread.stop_stream()
        #self.streamengine_thread.start()
        self.riskctl_thread.stop_stream()
        self.riskctl_thread.start()

        exchanges = list(self.configs[self.current_symbol]['exchanges'].keys())
        # exchange_combox
        if get_combox_data(self.exchange_combox) != set(exchanges):
            self.exchange_combox.clear()
            self.exchange_combox.addItems(exchanges)
            if self.current_exchange in exchanges: # 重新设置选中项
                self.exchange_combox.setCurrentText(self.current_exchange)
                self.exchange_combox.activated[str].emit(self.current_exchange)
            elif exchanges: # 默认设置第一个
                self.current_exchange = exchanges[0]
                self.exchange_combox.activated[str].emit(self.current_exchange)
        else:
            pass

        # 设置其他
        self.set_others()

    def exchange_changed(self, exchange):
        print('exchange_changed', exchange)
        self.current_exchange = exchange

        # 停止之前的数据线程
        self.stream_thread.stop_stream()
        self.stream_thread.set_params(self.current_exchange, self.current_symbol)
        self.stream_thread.start()

        # 设置其他
        self.set_others()
        

if __name__ == '__main__':
    app = QApplication(sys.argv)
    env_type = "-qa"

    if len(sys.argv) == 2:
        env_type = sys.argv[1]

    print("env_type: %s " % env_type)
    ex = DisplayWidget(env_name=env_type)

    sys.exit(app.exec_())
