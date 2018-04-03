package org.mobsya.thymiovpl;


import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbRequest;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;


class ThymioSerialConnection {
    private UsbDevice d;
    private UsbDeviceConnection con;
    private int cppObject;
    UsbEndpoint inEndpoint, outEndpoint;
    Thread thread;
    private AtomicBoolean stop;
    /***
     *  Default Serial Configuration
     *  Baud rate: 115200
     *  Data bits: 8
     *  Stop bits: 1
     *  Parity: None
     *  Flow Control: Off
     */
    private static final byte[] CDC_DEFAULT_LINE_CODING = new byte[] {
            (byte) 0x00, // Offset 0:4 dwDTERate
            (byte) 0xC2,
            (byte) 0x01,
            (byte) 0x00,
            (byte) 0x00, // Offset 5 bCharFormat (1 Stop bit)
            (byte) 0x00, // bParityType (None)
            (byte) 0x08  // bDataBits (8)
    };


    /*public native void jni_onData(int obj, byte[] data);


    public void doOnData(byte[] data) {
        jni_onData(cppObject, data);
    }*/


    public ThymioSerialConnection(int address, UsbDevice d, UsbDeviceConnection con) throws Exception {
        this.d = d;
        this.con = con;
        this.cppObject = address;

        for(int i = 0; i < d.getInterfaceCount(); i++) {
            UsbInterface usbInterface = d.getInterface(i);
            System.out.println("Interface " + i + "class:" + usbInterface.getInterfaceClass());
            if(usbInterface.getInterfaceClass() == UsbConstants.USB_CLASS_CDC_DATA  || usbInterface.getInterfaceClass() == UsbConstants.USB_CLASS_COMM) {
                System.out.println("Found CDC INTERFACE - CLAMING IT");
                con.claimInterface(usbInterface, true);
                int numberEndpoints = usbInterface.getEndpointCount();
                for(int j=0; j<numberEndpoints;j++)
                {
                    UsbEndpoint endpoint = usbInterface.getEndpoint(j);

                    System.out.println("Found end point" + endpoint.toString() + " direction " + endpoint.getDirection() + " type " + endpoint.getType());
                    if(endpoint.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK
                            && endpoint.getDirection() == UsbConstants.USB_DIR_IN)
                    {
                        inEndpoint = endpoint;
                        System.out.println("Found in endpoint");
                    }else if(endpoint.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK
                            && endpoint.getDirection() == UsbConstants.USB_DIR_OUT)
                    {
                        outEndpoint = endpoint;
                        System.out.println("Found out endpoint");
                    }
                }
            }
           // break;
        }
        if(inEndpoint == null || outEndpoint == null ) {
            throw  new Exception("Can not configure endpoint");
        }

        con.controlTransfer(0x21, 0x22, 0, 0, null, 0, 1000);
        System.out.println("Set Baud Rate");
        con.controlTransfer(0x21, 0x20, 0, 0, CDC_DEFAULT_LINE_CODING, CDC_DEFAULT_LINE_CODING.length, 1000);
        System.out.println("Set DTR");
        con.controlTransfer(0x21, 0x22, 0x03, 0, null, 0, 1000);
    }

    public int read(byte[] data) {
        System.out.println("Reading in " + data.length);
        int s = con.bulkTransfer(inEndpoint, data, data.length, 0);
        System.out.println("Read " + s + " Bytes");
        return s;
    }

    public int write(byte[] data) {
        int s = con.bulkTransfer(outEndpoint, data, data.length, 0);
        System.out.println("Wrote " + s + " Bytes");
        return s;
    }

}
