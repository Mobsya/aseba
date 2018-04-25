package org.mobsya.thymiovpl;

import org.qtproject.qt5.android.bindings.*;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.Context;
import android.content.BroadcastReceiver;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

import java.io.IOException;
import java.lang.String;
import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Iterator;

import android.os.AsyncTask;

public class ThymioVPLActivity extends QtActivity {

    native void onDeviceAvailabilityChanged();

    private static final int EPFL_VENDOR_ID = 0x0617;

    private UsbManager m_usb_manager;
    private static ThymioVPLActivity m_instance;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        BroadcastReceiver detachReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                if(intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_DETACHED)){
                    System.out.println("device detached");
                    onDeviceAvailabilityChanged();
                }
                if(intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
                    System.out.println("device attached");
                    onDeviceAvailabilityChanged();
                }
            };
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        registerReceiver(detachReceiver, filter);

        m_usb_manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        m_instance = this;
    }

    @Override
    protected void onNewIntent(Intent intent) {
        System.out.println(intent.getAction());
        super.onNewIntent(intent);
        if(intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
            System.out.println("device attached");
            onDeviceAvailabilityChanged();
        }

    }

    protected  List<UsbDevice> doListDevices()  {
        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = manager.getDeviceList();

        System.out.println(deviceList.size());

        List<UsbDevice> compatibleDevices = new ArrayList<UsbDevice>();

        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
        while(deviceIterator.hasNext()) {
            UsbDevice d = deviceIterator.next();
            if(d.getVendorId() != EPFL_VENDOR_ID) {
                continue;
            }
            compatibleDevices.add(d);
        }
        return compatibleDevices;
    }

    public static UsbDevice[] listDevices() {
        List<UsbDevice> devices = m_instance.doListDevices();
        return devices.toArray(new UsbDevice[devices.size()]);
    }

   /* public static ThymioSerialConnection openUsbDevice(int address, UsbDevice d) throws IOException, Exception {
        UsbManager manager = (UsbManager) m_instance.getSystemService(Context.USB_SERVICE);
        UsbDeviceConnection connection = manager.openDevice(d);
        ThymioSerialConnection thymioSerialConnection = new ThymioSerialConnection(address, d, connection);
        return thymioSerialConnection;
    }
    */

    public static UsbDeviceConnection openUsbDevice(UsbDevice d) throws IOException, Exception {
        UsbManager manager = (UsbManager) m_instance.getSystemService(Context.USB_SERVICE);
        UsbDeviceConnection connection = manager.openDevice(d);
        for(int i = 0; i < d.getInterfaceCount(); i++) {
            connection.claimInterface(d.getInterface(i), true);
        }
        return connection;
    }
}

































