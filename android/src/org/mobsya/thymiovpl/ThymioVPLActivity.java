package org.mobsya.thymiovpl;

import org.qtproject.qt5.android.bindings.*;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.Context;
import android.content.BroadcastReceiver;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import java.lang.String;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;



import android.os.AsyncTask;


public class ThymioVPLActivity extends QtActivity {

    native void onDeviceAvailabilityChanged();


    private UsbManager m_usb_manager;

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
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if(intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
            System.out.println("device attached");
            onDeviceAvailabilityChanged();
        }

    }
}
