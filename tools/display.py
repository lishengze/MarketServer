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
import kline_server_pb2
import kline_server_pb2_grpc
import empty_pb2
import struct
import json

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


class GrpcStreamThread(QThread):
    def __init__(self, addr, parent=None):
        super().__init__(parent)
        self.addr = addr

    def run(self):
        while True:            
            try:
                with grpc.insecure_channel(self.addr) as channel:
                    if self.run_grpc(channel) == False:
                        return
            except Exception as ex:
                print(ex)
                time.sleep(5)

    def run_grpc(self, channel):
        pass

class QueryApiThread(GrpcStreamThread):
    breakSignal = pyqtSignal(quote_data_pb2.MarketStreamData)
  
    def __init__(self, addr, parent=None):
        super().__init__(addr, parent)
  
    def run_grpc(self, channel):
        stub = risk_controller_pb2_grpc.RiskControllerStub(channel)
        responses = stub.ServeMarketStream4Client(empty_pb2.Empty())
        for resp in responses:
            for quote in resp.quotes:
                self.breakSignal.emit(quote)

class SubscribeKlineThread(GrpcStreamThread):
    breakSignal = pyqtSignal(kline_server_pb2.GetKlinesResponse)
  
    def __init__(self, addr, parent=None):
        super().__init__(addr, parent)
  
    def run_grpc(self, channel):
        print("SubscribeKlineThread running...")
        stub = kline_server_pb2_grpc.KlineServerStub(channel)
        responses = stub.GetLast(kline_server_pb2.GetKlinesRequest())
        for resp in responses:
            print(struct.unpack("=QQHQHQHQHd", resp.data))

class QueryStreamEngineThread(GrpcStreamThread):
    breakSignal = pyqtSignal(quote_data_pb2.MarketStreamData)
  
    def __init__(self, addr, parent=None):
        super().__init__(addr, parent)
  
    def run_grpc(self, channel):
        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)
        responses = stub.SubscribeMixQuote(stream_engine_pb2.SubscribeMixQuoteReq())
        for resp in responses:
            for quote in resp.quotes:
                #print(quote)
                self.breakSignal.emit(quote)

class QueryConfigThread(QThread):
    breakSignal = pyqtSignal(str)

    def __init__(self, addr, parent=None):
        super().__init__(parent)
        self.addr = addr

    def run(self):
        while True:
            resp = CallGetStreamEngineParams(self.addr)
            self.breakSignal.emit(resp.json_data)
            time.sleep(5)

class QueryRawThread(GrpcStreamThread):
    breakSignal = pyqtSignal(dict)
  
    def __init__(self, exchange, symbol, addr, parent=None):
        super().__init__(addr, parent)
        self.exchange = exchange
        self.symbol = symbol
        self.stopped = False
        print("restart single thread @", self.exchange, self.symbol)

    def stop_grpc(self):
        self.stopped = True

    def run_grpc(self, channel):
        stub = stream_engine_pb2_grpc.StreamEngineStub(channel)
        request = stream_engine_pb2.SubscribeQuoteReq()
        request.exchange = self.exchange
        request.symbol = self.symbol
        responses = stub.SubscribeQuote(request)
        cache = collections.defaultdict(dict)
        for resp in responses:
            if self.stopped:
                print("exit single thread @", self.exchange, self.symbol)
                return False
            for quote in resp.quotes:
                last_seqno = quote.seq_no
                if quote.is_snap:
                    cache['ask'] = {}
                    for depth in quote.asks:
                        cache['ask'][float(depth.price)] = depth.volume
                    cache['bid'] = {}
                    for depth in quote.bids:
                        cache['bid'][float(depth.price)] = depth.volume
                else:
                    for depth in quote.asks:
                        #print('quote.asks', depth)
                        if depth.volume < 0.000001:
                            try:
                                del cache['ask'][float(depth.price)]
                            except:
                                pass
                        else:
                            cache['ask'][float(depth.price)] = depth.volume
                    for depth in quote.bids:
                        if depth.volume < 0.000001:
                            try:
                                del cache['bid'][float(depth.price)]
                            except:
                                pass
                        else:
                            cache['bid'][float(depth.price)] = depth.volume
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

        for i, (txt_price, txt_volumes) in enumerate(ask_data):
            if i >= len(self.asks):
                break
            (lbl_price, lbl_volumes) = self.asks[i]
            lbl_price.setText(txt_price)
            lbl_volumes.setText(txt_volumes)
        for i, (txt_price, txt_volumes) in enumerate(bid_data):
            if i >= len(self.bids):
                break
            (lbl_price, lbl_volumes) = self.bids[i]
            lbl_price.setText(txt_price)
            lbl_volumes.setText(txt_volumes)        

class DisplayWidget(QWidget):
    DISPLAY_DEPTH = 20 # 这个值固定不变
    DEPTH = 20
    FREQUENCY = 1
    RAW_FREQUENCY = 1
    #STREAMENGINE_ADDR = "172.25.3.207:9000"
    #API_ADDR = "172.25.3.207:9900"
    #KLINESERVER_ADDR = "172.25.3.207:9990"
    STREAMENGINE_ADDR = "36.255.220.139:9110"
    API_ADDR = "36.255.220.139:9111"
    KLINESERVER_ADDR = "36.255.220.139:9110"
    
    def __init__(self):
        super().__init__()

        self.current_symbol = ''
        self.current_exchange = ''

        # 初始化ui
        self.initUI()
        
        # 启动获取api聚合行情接口
        self.api_thread = QueryApiThread(self.API_ADDR)
        self.api_thread.breakSignal.connect(self.update_api_data)
        self.api_thread.start()
        
        # 启动获取stream engine聚合行情接口
        self.streamengine_thread = QueryStreamEngineThread(self.STREAMENGINE_ADDR)
        self.streamengine_thread.breakSignal.connect(self.update_streamengine_data)
        self.streamengine_thread.start()

        # 原始行情聚合接口需要动态获取
        #self.raw_thread = QueryRawThread(self.current_exchange, self.current_symbol, self.STREAMENGINE_ADDR)
        self.raw_thread = QueryRawThread('', '', self.STREAMENGINE_ADDR)
        self.raw_thread.breakSignal.connect(self.update_raw_data)
        self.raw_thread.start()

        # 查询服务器配置参数
        self.cfg_thread = QueryConfigThread(self.STREAMENGINE_ADDR)
        self.cfg_thread.breakSignal.connect(self.update_config_data)
        self.cfg_thread.start()

        # 启动K线更新接口
        self.kline_thread = SubscribeKlineThread(self.KLINESERVER_ADDR)
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

        label = QLabel("-------------------")
        ctl_grid.addWidget(label, 5, 0)
        label = QLabel("RiskController设置:")
        ctl_grid.addWidget(label, 6, 0)
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

    def update_api_data(self, msg):
        def _make_data(depths):
            ret = []
            for depth in depths:
                # 价位数据
                txt_price = depth.price
                # 挂单数据
                total = depth.volume
                desc = ""
                for exchange, volume in depth.data.items():
                    desc += "{0}:{1:.04f},".format(exchange, volume)
                txt_volumes = "{0:.04f}(".format(total) + desc + ")"
                # 添加数据
                ret.append((txt_price, txt_volumes))
            return ret
        if msg.symbol != self.current_symbol:
            return
        ask_data = _make_data(msg.asks)
        bid_data = _make_data(msg.bids)
        self.api_widget.bind_data(ask_data, bid_data)

    def update_streamengine_data(self, msg):
        def _make_data(depths):
            ret = []
            for i, depth in enumerate(depths):
                # 价位数据
                txt_price = depth.price
                # 挂单数据
                total = 0
                desc = ""
                for exchange, volume in depth.data.items():
                    #self._add_exchange(data.exchange, msg.symbol)
                    total += volume
                    desc += "{0}:{1:.04f},".format(exchange, volume)
                txt_volumes = "{0:.04f} - ".format(total) + desc
                # 添加数据
                ret.append((txt_price, txt_volumes))
            return ret

        # 添加品种
        #self._add_symbol(msg.symbol)
        if msg.symbol != self.current_symbol:
            return
        ask_data = _make_data(msg.asks)
        bid_data = _make_data(msg.bids)
        self.streamengine_widget.bind_data(ask_data, bid_data)

    def update_raw_data(self, msg):
        def _make_data(depths, is_ask):
            ret = []
            keys = sorted(depths.keys())
            if not is_ask:
                keys = keys[::-1]
            for price in keys:
                ret.append((str(price), str(depths[price])))
            return ret

        ask_data = _make_data(msg['ask'], True)
        bid_data = _make_data(msg['bid'], False)
        self.raw_widget.bind_data(ask_data, bid_data)

    def update_config_data(self, msg):
        # 解析完整的配置信息
        self.configs = json.loads(msg)
        print(self.configs)

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
        self.precise_lbl.setText(str(self.configs[self.current_symbol]['precise']))
        self.frequency_lbl.setText(str(self.configs[self.current_symbol]['frequency']))
        self.mixdepth_lbl.setText(str(self.configs[self.current_symbol]['mix_depth']))
        self.mixfrequency_lbl.setText(str(self.configs[self.current_symbol]['mix_frequency']))

    def symbol_changed(self, symbol):
        print('symbol_changed', symbol)
        self.current_symbol = symbol

        exchanges = list(self.configs[self.current_symbol]['exchanges'].keys())
        # exchange_combox
        if get_combox_data(self.exchange_combox) != set(exchanges):
            self.exchange_combox.clear()
            self.exchange_combox.addItems(exchanges)
            if self.current_exchange in exchanges: # 重新设置选中项
                self.exchange_combox.setCurrentText(self.current_exchange)
            elif exchanges: # 默认设置第一个
                self.current_exchange = exchanges[0]
                self.exchange_combox.activated[str].emit(self.current_exchange)
                
        # 设置其他
        self.precise_lbl.setText(str(self.configs[self.current_symbol]['precise']))
        self.frequency_lbl.setText(str(self.configs[self.current_symbol]['frequency']))
        self.mixdepth_lbl.setText(str(self.configs[self.current_symbol]['mix_depth']))
        self.mixfrequency_lbl.setText(str(self.configs[self.current_symbol]['mix_frequency']))

    def exchange_changed(self, exchange):
        print('exchange_changed', exchange)
        self.current_exchange = exchange

        # 停止之前的数据线程
        if self.raw_thread:
            self.raw_thread.stop_grpc()
            self.raw_thread.exit()
        # 创建新的数据线程
        self.raw_thread = QueryRawThread(self.current_exchange, self.current_symbol, self.STREAMENGINE_ADDR)
        self.raw_thread.breakSignal.connect(self.update_raw_data)
        self.raw_thread.start()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = DisplayWidget()

    sys.exit(app.exec_())
