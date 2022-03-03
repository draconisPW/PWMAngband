package org.pwmangband.pwmangclient;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class Settings extends SDLActivity {
    public static void setEnvVars() {
        SDLActivity.nativeSetenv( "HOME", Environment.getExternalStorageDirectory().getAbsolutePath() + "/pwmangclient" );
        SDLActivity.nativeSetenv( "ANDROID_APP_PATH", Environment.getExternalStorageDirectory().getAbsolutePath() + "/pwmangclient" );
    }

    public static void createNewDirectory(String name) {
        //Create a directory before creating a new file inside it.
        File directory = new File(Environment.getExternalStorageDirectory().getAbsolutePath(), name);
        if (!directory.exists()) {
            directory.mkdirs();
        }
    }

    public static void unZip(Context context, String assetName, String outputDirectory, boolean isReWrite) throws IOException {
        //Create decompression target directory
        File file = new File(outputDirectory);
        //If the target directory does not exist, create
        if (!file.exists()) {
            file.mkdirs();
        }
        //Open compressed file
        InputStream inputStream = context.getAssets().open(assetName);
        ZipInputStream zipInputStream = new ZipInputStream(inputStream);
        //read an entry point
        ZipEntry zipEntry = zipInputStream.getNextEntry();
        //Use 1Mbuffer
        byte[] buffer = new byte[1024 * 1024];
        //Byte count when decompressing
        int count = 0;
        //If the entry point is empty, it means that all files and directories in the compressed package have been traversed
        while (zipEntry != null) {
            //If it is a directory
            if (zipEntry.isDirectory()) {
                file = new File(Environment.getExternalStorageDirectory() + outputDirectory + File.separator + zipEntry.getName());
                //The file needs to be overwritten or the file does not exist
                if (isReWrite || !file.exists()) {
                    file.mkdir();
                }
            } else {
                //if it is a file
                file = new File(Environment.getExternalStorageDirectory() + outputDirectory + File.separator + zipEntry.getName());
                //The file needs to be overwritten or the file does not exist, unzip the file
                if (isReWrite || !file.exists()) {
                    file.createNewFile();
                    FileOutputStream fileOutputStream = new FileOutputStream(file);
                    while ((count = zipInputStream.read(buffer))> 0) {
                        fileOutputStream.write(buffer, 0, count);
                    }
                    fileOutputStream.close();
                }
            }
            //Locate to the next file entry
            zipEntry = zipInputStream.getNextEntry();
        }
        zipInputStream.close();
    }

    public static void copyAssets(Context context, String file, String path) {
        AssetManager assetManager = context.getAssets();
        String[] files = null;
        InputStream in = null;
        OutputStream out = null;
        String filename = file;
        try
        {
            in = assetManager.open(filename);
            out = new FileOutputStream(Environment.getExternalStorageDirectory() + path + File.separator + filename);
            copyFile(in, out);
            in.close();
            in = null;
            out.flush();
            out.close();
            out = null;
        }
        catch(IOException e)
        {
            Log.e("tag", "Failed to copy asset file: " + filename, e);
        }
    }

    public static void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while((read = in.read(buffer)) != -1){
            out.write(buffer, 0, read);
        }
    }
}
