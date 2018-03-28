package org.mobsya.thymiovpl;


import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.provider.Settings;

import com.felhr.usbserial.CDCSerialDevice;
import com.felhr.usbserial.UsbSerialInterface;


class ThymioSerialConnection {
    private UsbDevice d;
    private UsbDeviceConnection con;
    private CDCSerialDevice serialPort;
    private int cppObject;


    public native void jni_onData(int obj, byte[] data);


    public void doOnData(byte[] data) {
        jni_onData(cppObject, data);
    }

    private UsbSerialInterface.UsbReadCallback mCallback = new UsbSerialInterface.UsbReadCallback() {
        @Override
        public void onReceivedData(byte[] data) {
            if(data != null && data.length > 0) {
                System.out.println(data.length);
                doOnData(data);
            }
        }
    };

    public ThymioSerialConnection(int address, UsbDevice d, UsbDeviceConnection con) {
        this.d = d;
        this.con = con;
        this.cppObject = address;

        this.serialPort = new CDCSerialDevice(d, con);
        if(serialPort != null && serialPort.open()) {
            /*serialPort.setDTR(true);
            serialPort.setBaudRate(115200);
            serialPort.setDataBits(UsbSerialInterface.DATA_BITS_8);
            serialPort.setStopBits(UsbSerialInterface.STOP_BITS_1);
            serialPort.setParity(UsbSerialInterface.PARITY_NONE);
            serialPort.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);*/
        }
        this.serialPort.read(mCallback);
    }


    public int write(byte[] data) {
        serialPort.write(data);
        return data.length;
    }

}
