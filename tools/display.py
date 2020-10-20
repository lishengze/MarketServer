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
import api_pb2
import api_pb2_grpc
import stream_engine_server_pb2
import stream_engine_server_pb2_grpc
import empty_pb2

def CallChangeStreamEngineParams(addr, params):
    with grpc.insecure_channel(addr) as channel:
        stub = stream_engine_server_pb2_grpc.StreamEngineServiceStub(channel)
        request = stream_engine_server_pb2.SetParamsReq()
        request.depth = params['depth']
        request.frequency = params['frequency']
        request.precise = params.get('precise', 0)
        request.symbol = params.get('symbol', "")
        stub.SetParams(request)


class GrpcStreamThread(QThread):
    def __init__(self, addr, parent=None):
        super().__init__(parent)
        self.addr = addr

    def run(self):
        while True:            
            try:
                with grpc.insecure_channel(self.addr) as channel:
                    self.run_grpc(channel)
            except Exception as ex:
                print(ex)
                time.sleep(5)

    def run_grpc(self, channel):
        pass

class QueryApiThread(GrpcStreamThread):
    breakSignal = pyqtSignal(api_pb2.MarketStreamData)
  
    def __init__(self, addr, parent=None):
        super().__init__(addr, parent)
  
    def run_grpc(self, channel):
        stub = api_pb2_grpc.TradeStub(channel)
        responses = stub.ServeMarketStream(empty_pb2.Empty())
        for resp in responses:
            for quote in resp.quotes:
                self.breakSignal.emit(quote)

class QueryStreamEngineThread(GrpcStreamThread):
    breakSignal = pyqtSignal(stream_engine_server_pb2.QuoteData)
  
    def __init__(self, addr, parent=None):
        super().__init__(addr, parent)
  
    def run_grpc(self, channel):
        stub = stream_engine_server_pb2_grpc.StreamEngineServiceStub(channel)
        responses = stub.MultiSubscribeQuote(stream_engine_server_pb2.SubscribeQuoteReq())
        for resp in responses:
            for quote in resp.quotes:
                self.breakSignal.emit(quote)

class QueryRawThread(GrpcStreamThread):
    breakSignal = pyqtSignal(stream_engine_server_pb2.QuoteData)
  
    def __init__(self, exchange, symbol, addr, parent=None):
        super().__init__(addr, parent)
        self.exchange = exchange
        self.symbol = symbol
        self.stopped = False
        print("restart single thread @", self.exchange, self.symbol)

    def stop_grpc(self):
        self.stopped = True

    def run_grpc(self, channel):
        stub = stream_engine_server_pb2_grpc.StreamEngineServiceStub(channel)
        request = stream_engine_server_pb2.GetQuoteReq()
        request.exchange = self.exchange
        request.symbol = self.symbol
        responses = stub.SubscribeOneQuote(request)
        for resp in responses:
            if self.stopped:
                print("exit single thread @", self.exchange, self.symbol)
                return
            for quote in resp.quotes:
                self.breakSignal.emit(quote)

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
        # 构造buy档位
        for i in range(self.depth):
            lbl_price, lbl_volumes = _create_pricelevel(self.grid, i + self.depth, "Bid", i+1, i==0)
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
    DEPTH = 20
    FREQUENCY = 1
    STREAMENGINE_ADDR = "172.25.3.207:9000"
    API_ADDR = "172.25.3.207:9900"
    
    def __init__(self):
        super().__init__()
        # 当前选定的品种
        self.current_symbol = ""
        # 当前选定的交易所（原始行情）
        self.current_exchange = ""
        # 所有聚合行情品种：来自api接口
        self.all_symbols = set()
        # 所有聚合行情品种对一个的交易所，来自api接口
        self.all_exchanges = collections.defaultdict(set)
        
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
        self.raw_thread = None

    def initUI(self):
        # 风控参数调整区域（暂未使用）
        ctl_panel = QWidget()
        ctl_grid = QGridLayout()
        ctl_grid.setAlignment(QtCore.Qt.AlignTop)
        label = QLabel("StreamEngine设置:")
        ctl_grid.addWidget(label, 0, 0)
        label = QLabel("设置深度")
        ctl_grid.addWidget(label, 1, 0)
        self.depth_combox = QComboBox(self)
        self.depth_combox.addItems(["20", "10", "5"])
        self.depth_combox.currentIndexChanged[str].connect(self.depth_changed)
        ctl_grid.addWidget(self.depth_combox, 1, 1)
        label = QLabel("设置频率")
        ctl_grid.addWidget(label, 2, 0)
        self.frequecy_combox = QComboBox(self)
        self.frequecy_combox.addItems(["1", "2", "10", "100"])
        self.frequecy_combox.currentIndexChanged[str].connect(self.frequency_changed)
        ctl_grid.addWidget(self.frequecy_combox, 2, 1)
        label = QLabel("设置精度")
        ctl_grid.addWidget(label, 3, 0)
        self.precise_combox = QComboBox(self)
        self.precise_combox.addItems(["1", "2", "3", "4"])
        self.precise_combox.currentIndexChanged[str].connect(self.precise_changed)
        ctl_grid.addWidget(self.precise_combox, 3, 1)
        label = QLabel("-------------------")
        ctl_grid.addWidget(label, 4, 0)
        label = QLabel("RiskController设置:")
        ctl_grid.addWidget(label, 5, 0)
        ctl_panel.setLayout(ctl_grid) 

        # stream engine数据显示区域
        streamengine_panel = QWidget()
        self.streamengine_widget = AskBidWidget(self.DEPTH)
        streamengine_panel.setLayout(self.streamengine_widget.grid) 

        # api数据显示区域
        api_panel = QWidget()
        self.api_widget = AskBidWidget(self.DEPTH)
        api_panel.setTabOrder
        api_panel.setLayout(self.api_widget.grid)

        # 原始数据显示区域
        raw_panel = QWidget()
        self.raw_widget = AskBidWidget(self.DEPTH)
        raw_panel.setLayout(self.raw_widget.grid) 

        # 主layout区域
        wlayout = QGridLayout()
        # 0行 - 0列
        self.symbol_combox = QComboBox(self)
        self.symbol_combox.currentIndexChanged[str].connect(self.symbol_changed)
        wlayout.addWidget(self.symbol_combox, 0, 0)
        # 0行 - 1列
        wlayout.addWidget(QLabel("风控前行情"), 0, 1)
        # 0行 - 2列
        wlayout.addWidget(QLabel("风控后行情"), 0, 2)
        # 0行 - 3列
        self.exchange_combox = QComboBox(self)
        self.exchange_combox.currentIndexChanged[str].connect(self.exchange_changed)
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

    def _add_symbol(self, symbol):
        # add to combobox
        if symbol not in self.all_symbols:
            self.all_symbols.add(symbol)
            self.symbol_combox.addItem(symbol)

    def _add_exchange(self, exchange, symbol):
        # add to combobox
        if exchange not in self.all_exchanges[symbol]:
            self.all_exchanges[symbol].add(exchange)
            self.exchange_combox.addItem(exchange)

    def update_api_data(self, msg):
        def _make_data(depths):
            ret = []
            for depth in depths:
                # 价位数据
                txt_price = depth.price
                # 挂单数据
                total = 0
                desc = ""
                for data in depth.data:
                    self._add_exchange(data.exchange, msg.symbol)
                    total += data.size
                    desc += "{0}:{1:.04f},".format(data.exchange, data.size)
                txt_volumes = "{0:.04f}(".format(total) + desc + ")"
                # 添加数据
                ret.append((txt_price, txt_volumes))
            return ret
        # 添加品种
        self._add_symbol(msg.symbol)
        if msg.symbol != self.current_symbol:
            return
        ask_data = _make_data(msg.ask_depth)
        bid_data = _make_data(msg.bid_depth)
        self.api_widget.bind_data(ask_data, bid_data)

    def update_streamengine_data(self, msg):
        def _make_data(depths):
            ret = []
            for depth in depths:
                # 价位数据
                fmt = "{0:.0" + str(depth.price.base) + "f}"
                txt_price = fmt.format(depth.price.value/(10**depth.price.base))
                # 挂单数据
                total = 0
                desc = ""
                for data in depth.data:
                    total += data.volume
                    desc += "{0}:{1:.04f},".format(data.exchange, data.volume)
                txt_volumes = "{0:.04f} - ".format(total) + desc
                # 添加数据
                ret.append((txt_price, txt_volumes))
            return ret

        if msg.symbol != self.current_symbol:
            return
        ask_data = _make_data(msg.ask_depth)
        bid_data = _make_data(msg.bid_depth)        
        self.streamengine_widget.bind_data(ask_data, bid_data)

    def update_raw_data(self, msg):
        def _make_data(depths):
            ret = []
            for depth in depths:
                # 价位数据
                fmt = "{0:.0" + str(depth.price.base) + "f}"
                txt_price = fmt.format(depth.price.value/(10**depth.price.base))
                # 挂单数据
                txt_volumes = ""
                if len(depth.data) > 0 :
                    txt_volumes = "{0:.04f}".format(depth.data[0].volume)
                # 添加数据
                ret.append((txt_price, txt_volumes))
            return ret

        ask_data = _make_data(msg.ask_depth)
        bid_data = _make_data(msg.bid_depth)
        self.raw_widget.bind_data(ask_data, bid_data)

    def symbol_changed(self, symbol):
        self.current_symbol = symbol

        # 品种变化的时候，动态更新原始行情的交易所列表
        exchanges = self.all_exchanges[symbol]
        self.exchange_combox.clear()
        self.exchange_combox.addItems(list(exchanges))

    def exchange_changed(self, exchange):
        self.current_exchange = exchange

        # 停止之前的数据线程
        if self.raw_thread:
            self.raw_thread.stop_grpc()
            self.raw_thread.exit()
        # 创建新的数据线程
        self.raw_thread = QueryRawThread(self.current_exchange, self.current_symbol, self.STREAMENGINE_ADDR)
        self.raw_thread.breakSignal.connect(self.update_raw_data)
        self.raw_thread.start()

    def depth_changed(self, depth):
        self.DEPTH = int(depth)
        CallChangeStreamEngineParams(self.STREAMENGINE_ADDR, {"depth": self.DEPTH, "frequency": self.FREQUENCY})

    def frequency_changed(self, frequency):
        self.FREQUENCY = int(frequency)
        CallChangeStreamEngineParams(self.STREAMENGINE_ADDR, {"depth": self.DEPTH, "frequency": self.FREQUENCY})

    def precise_changed(self, precise):
        CallChangeStreamEngineParams(self.STREAMENGINE_ADDR, {"depth": self.DEPTH, "frequency": self.FREQUENCY, "precise": int(precise), "symbol": self.current_symbol})
        

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = DisplayWidget()

    sys.exit(app.exec_())
