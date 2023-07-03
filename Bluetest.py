import serial

import serial.tools.list_ports

if __name__ == "__main__":

    ports_list = list(serial.tools.list_ports.comports())
    if len(ports_list) <= 0:
        print("无串口设备。")
    else:
        print("可用的串口设备如下：")
        for comport in ports_list:
            print(list(comport)[0], list(comport)[1])


    try:
        ser = serial.Serial("COM7", 9600)
        if ser.isOpen():  # 判断串口是否成功打开
            print("打开串口成功。")
            print(ser.name)  # 输出串口号

            # 串口发送 ABCDEFG，并输出发送的字节数。
            write_len = ser.write("ABCDEFG".encode('utf-8'))
            print("串口发出{}个字节。".format(write_len))

            ser.close()
            if ser.isOpen():  # 判断串口是否关闭
                print("串口未关闭。")
            else:
                print("串口已关闭。")


        else:
            print("打开串口失败。")
    except:
        print("打开串口失败。")