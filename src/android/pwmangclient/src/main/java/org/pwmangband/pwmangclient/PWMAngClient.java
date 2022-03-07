package org.pwmangband.pwmangclient;

import android.os.Bundle;

import org.libsdl.app.SDLActivity;

public class PWMAngClient extends SDLActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Settings.setEnvVars();
        Settings.checkInstall(getApplicationContext());
    }
}
