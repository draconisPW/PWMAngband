package org.pwmangband.pwmangclient;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.IOException;

public class PWMAngClient extends SDLActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Settings.setEnvVars();

        File folder = new File(Environment.getExternalStorageDirectory() + File.separator + "pwmangclient");
        if (!folder.exists()) {
            Log.e("Not Found Dir", "Creating directory 'pwmangclient'");
            Toast.makeText(getApplicationContext(),"Creating directory 'pwmangclient'", Toast.LENGTH_LONG).show();
            Settings.createNewDirectory("pwmangclient");
        } else {
            Log.e("Found Dir", "directory 'pwmangclient'" );
        }

        File file = new File(Environment.getExternalStorageDirectory() + File.separator + "pwmangclient/pwmangclient.ini");
        if (!file.exists()) {
            Log.e("File Not Found", "Copy assets file 'pwmangclient/pwmangclient.ini'");
            Toast.makeText(getApplicationContext(),"Copy assets file 'pwmangclient.ini'", Toast.LENGTH_LONG).show();
            Settings.copyAssets(getApplicationContext(), "pwmangclient.ini", "/pwmangclient");
        } else {
            Log.e("File Found", "file 'pwmangclient/pwmangclient.ini'" );
        }

        File folder_lib = new File(Environment.getExternalStorageDirectory() + File.separator + "pwmangclient/lib");
        if (!folder_lib.exists()) {
            Log.e("Not Found Dir", "Copy assets files 'pwmangclient/lib'");
            //Toast.makeText(getApplicationContext(),"Copy assets files 'lib'", Toast.LENGTH_LONG).show();
            Settings.createNewDirectory("pwmangclient/lib");
            try {
                Settings.unZip(getApplicationContext(), "lib.zip", "/pwmangclient", true);
            } catch (IOException e) {
                e.printStackTrace();
            }
            Toast.makeText(getApplicationContext(),"Copy assets files 'lib'", Toast.LENGTH_LONG).show();
        } else {
            Log.e("Found Dir", "directory 'pwmangclient/lib'" );
        }
    }
}
