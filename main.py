from threading import Thread
from time import sleep, ctime
# IMPORT PACKAGES AND MODULES
# ///////////////////////////////////////////////////////////////
from gui.uis.windows.main_window.functions_main_window import *
import sys
import os

import time
from Bluetest import *

import serial
import serial.tools.list_ports

# IMPORT QT CORE
# ///////////////////////////////////////////////////////////////
from qt_core import *

# IMPORT SETTINGS
# ///////////////////////////////////////////////////////////////
from gui.core.json_settings import Settings

# IMPORT PY ONE DARK WINDOWS
# ///////////////////////////////////////////////////////////////
# MAIN WINDOW
from gui.uis.windows.main_window import *

# IMPORT PY ONE DARK WIDGETS
# ///////////////////////////////////////////////////////////////
from gui.widgets import *

# ADJUST QT FONT DPI FOR HIGHT SCALE AN 4K MONITOR
# ///////////////////////////////////////////////////////////////
os.environ["QT_FONT_DPI"] = "96"
os.environ["QT_SCALE_FACTOR"] = "1.5"
# IF IS 4K MONITOR ENABLE 'os.environ["QT_SCALE_FACTOR"] = "2"'
# IF IS 2K MONITOR ENABLE 'os.environ["QT_SCALE_FACTOR"] = "1.5"'

# MAIN WINDOW
# ///////////////////////////////////////////////////////////////
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        # SETUP MAIN WINDOw
        # Load widgets from "gui\uis\main_window\ui_main.py"
        # ///////////////////////////////////////////////////////////////
        self.ui = UI_MainWindow()
        self.ui.setup_ui(self)

        # LOAD SETTINGS
        # ///////////////////////////////////////////////////////////////
        settings = Settings()
        self.settings = settings.items

        self.setWindowIcon(QIcon(self.settings["icon"]))

        self.Port = self.settings["port"]
        self.BluetoothSerial=serial.Serial()
        # SETUP MAIN WINDOW
        # ///////////////////////////////////////////////////////////////
        self.hide_grips = True # Show/Hide resize grips
        SetupMainWindow.setup_gui(self)
        MainFunctions.set_page(self, self.ui.load_pages.page_1)
        # SHOW MAIN WINDOW
        # ///////////////////////////////////////////////////////////////
        self.show()

        self.uiInitiation()

        self.portList=[]


    def uiInitiation(self):
        self.push_button_Open.clicked.connect(lambda: self.flipPort())
        self.push_button_Show.clicked.connect(lambda: self.send("[NUMBER]"+self.line_edit.text()))
        self.push_button_EL.clicked.connect(lambda: self.send("[NUMBER]0.337187"))

        self.push_button_SyncTime.clicked.connect(lambda: self.syncTime())

        self.push_button_stopwatch_start.clicked.connect(lambda: self.stopwatchStart())
        self.push_button_stopwatch_reset.clicked.connect(lambda: self.stopwatchReset())

        self.push_button_timer_start.clicked.connect(lambda: self.timerStart())
        self.push_button_timer_reset.clicked.connect(lambda: self.timerReset())

        self.toggle_button.stateChanged.connect(lambda: self.changeTimeMode())

    def changeTimeMode(self):
        if not self.toggle_button.isChecked():
            self.send("[TIMEMODE]0")
        elif self.toggle_button.isChecked():
            self.send("[TIMEMODE]2")
    def timerStart(self):
        if self.push_button_timer_start.text() == "Start":
            try:
                hour=str(int(self.line_edit_Hour.text())%24).rjust(2,"0")
            except:
                hour="00"

            try:
                minute=str(int(self.line_edit_Minute.text())%60).rjust(2,"0")
            except:
                minute="00"

            try:
                second=str(int(self.line_edit_Second.text())%60).rjust(2,"0")
            except:
                second="00"

            self.send("[TIMER]"+"000000"+hour+minute+second)
            sleep(1)
            self.send("[TIMER]CONTINUE")
            self.push_button_timer_start.setText("Pause")

        elif self.push_button_timer_start.text() == "Continue":
            self.push_button_timer_start.setText("Pause")
            self.send("[TIMER]CONTINUE")

        elif self.push_button_timer_start.text() == "Pause":
            self.push_button_timer_start.setText("Continue")
            self.send("[TIMER]PAUSE")


    def timerReset(self):
        self.send("[TIMER]RESET")
        self.push_button_timer_start.setText("Start")

    def stopwatchStart(self):
        if self.push_button_stopwatch_start.text()=="Start" or self.push_button_stopwatch_start.text()=="Continue":
            self.send("[STOPWATCH]CONTINUE")
            self.push_button_stopwatch_start.setText("Pause")
        elif self.push_button_stopwatch_start.text()=="Pause":
            self.send("[STOPWATCH]PAUSE")
            self.push_button_stopwatch_start.setText("Continue")

    def stopwatchReset(self):
        self.send("[STOPWATCH]RESET")
        self.push_button_stopwatch_start.setText("Start")

    def send(self,str):
        try:
            self.BluetoothSerial.write(str.encode('utf-8'))
            print("[NTC][Info]: send command: " + str)
            self.showState("successfully opened Port " + self.BluetoothSerial.name, "green")
        except:
            print("[NTC][Failed]: failed sending command: "+str)
            self.showState("Connection failed","red")


    def syncTime(self):
        t = time.localtime()
        tString=str(t.tm_year)[-2:]+ str(t.tm_mon).rjust(2, '0')+ str(t.tm_mday).rjust(2, '0')+str(t.tm_hour).rjust(2, '0')+ str(t.tm_min).rjust(2, '0')+ str(t.tm_sec).rjust(2, '0')

        self.send("[SYNCTIME]"+tString)

    def flipPort(self):
        if self.push_button_Open.text()=="Connect":
            self.portList = list(serial.tools.list_ports.comports())

            foundNixie=0
            if len(self.portList) <= 0:
                print("[NTC][Failed]:No Port Available")
                self.showState("No Port Available","red")

            else:
                for comport in self.portList:
                    if(list(comport)[0])==self.Port:
                        foundNixie=1
                if foundNixie==0:
                    self.showState("Cannot Find Nixie Tube", "red")
                    print("[NTC][Failed]:Cannot Find Nixie Tube")
                else:
                    self.showState("Found Port, Connecting...", "yellow")
                    print("[NTC][Info]:Found Port, Connecting...")

            try:
                self.BluetoothSerial = serial.Serial(self.Port, 9600)
                if self.BluetoothSerial.isOpen():  # 判断串口是否成功打开
                    print("[NTC][Info]:Open Port Successful")
                    self.showState("successfully opened Port "+self.BluetoothSerial.name,"green")
                    self.push_button_Open.setText("Disconnect")

                else:
                    print("[NTC][Failed]:Open Port Failed")
                    self.showState("Open Port Failed", "red")
            except:
                print("[NTC][Failed]:Open Port Failed (exception)")
                self.showState("Open Port Failed", "red")

        elif self.push_button_Open.text() == "Disconnect":
            self.BluetoothSerial.close()
            if self.BluetoothSerial.isOpen():  # 判断串口是否关闭
                print("[NTC][Failed]:Cannot shut connection down")
            else:
                self.push_button_Open.setText("Connect")
                print("[NTC][Info]:Connection is closed")
                self.showState("Not Connected", "grey")

    def showState(self,str,color):
        self.ui.credits.text_label.setText(str)
        if color=="red":
            self.ui.credits.text_label.setStyleSheet("color: red;")
        elif color=="green":
            self.ui.credits.text_label.setStyleSheet("color: green;")
        elif color=="grey":
            self.ui.credits.text_label.setStyleSheet("color: grey;")
        elif color=="yellow":
            self.ui.credits.text_label.setStyleSheet("color: yellow;")
    # LEFT MENU BTN IS CLICKED
    # Run function when btn is clicked
    # Check funtion by object name / btn_id
    # ///////////////////////////////////////////////////////////////
    def btn_clicked(self):
        # GET BT CLICKED
        btn = SetupMainWindow.setup_btns(self)


        # LEFT MENU
        # ///////////////////////////////////////////////////////////////
        
        # HOME BTN
        if btn.objectName() == "btn_home":
            # Select Menu
            self.ui.left_menu.select_only_one(btn.objectName())

            # Load Page 1
            MainFunctions.set_page(self, self.ui.load_pages.page_1)
            self.send("[MODE]1")
        # WIDGETS BTN
        if btn.objectName() == "clock":
            # Select Menu
            self.ui.left_menu.select_only_one(btn.objectName())

            # Load Page 2
            MainFunctions.set_page(self, self.ui.load_pages.page_2)
            self.send("[MODE]2")



        # LOAD USER PAGE
        if btn.objectName() == "stopwatch":
            # Select Menu
            self.ui.left_menu.select_only_one(btn.objectName())

            # Load Page 3 
            MainFunctions.set_page(self, self.ui.load_pages.page_3)
            self.send("[MODE]3")

        if btn.objectName() == "hourglass":
            # Select Menu
            self.ui.left_menu.select_only_one(btn.objectName())

            # Load Page 3
            MainFunctions.set_page(self, self.ui.load_pages.page_4)
            self.send("[MODE]4")



        # DEBUG
        #print(f"Button {btn.objectName()}, clicked!")

    # LEFT MENU BTN IS RELEASED
    # Run function when btn is released
    # Check funtion by object name / btn_id
    # ///////////////////////////////////////////////////////////////
    def btn_released(self):
        # GET BT CLICKED
        btn = SetupMainWindow.setup_btns(self)

        # DEBUG
        #print(f"Button {btn.objectName()}, released!")

    # RESIZE EVENT
    # ///////////////////////////////////////////////////////////////
    def resizeEvent(self, event):
        SetupMainWindow.resize_grips(self)

    # MOUSE CLICK EVENTS
    # ///////////////////////////////////////////////////////////////
    def mousePressEvent(self, event):
        # SET DRAG POS WINDOW
        self.dragPos = event.globalPos()


# SETTINGS WHEN TO START
# Set the initial class and also additional parameters of the "QApplication" class
# ///////////////////////////////////////////////////////////////
if __name__ == "__main__":
    # APPLICATION
    # ///////////////////////////////////////////////////////////////
    app = QApplication(sys.argv)
    window = MainWindow()

    # EXEC APP
    # ///////////////////////////////////////////////////////////////
    sys.exit(app.exec_())