package com.smackall.animator.Helper;

import android.content.Context;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Created by Sabish.M on 25/11/15.
 * Copyright (c) 2015 Smackall Games Pvt Ltd. All rights reserved.
 */
public class KeyMaker {

    public static boolean makeKey(Context context){
        File shared_prefs = new File("data/data/"+context.getPackageName()+"/shared_prefs/");
        FileHelper.mkDir(shared_prefs);
        File key = new File("data/data/"+context.getPackageName()+"/shared_prefs/key");
        try {
            if(!key.exists())
            key.createNewFile();
            return true;

        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    public static void writeToKeyFile(String data,Context context) throws FileNotFoundException {

        File key = new File("data/data/"+context.getPackageName()+"/shared_prefs/key");
        FileOutputStream stream = new FileOutputStream(key);
        try {
            stream.write(data.getBytes());
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                stream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static String readFromKeyFile(Context context) throws IOException {
        File key = new File("data/data/"+context.getPackageName()+"/shared_prefs/key");
        if(!key.exists())
            return null;
        int length = (int) key.length();

        byte[] bytes = new byte[length];

        FileInputStream in = new FileInputStream(key);
        try {
            in.read(bytes);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        } finally {
            in.close();
        }

        String contents = new String(bytes);
        return contents;
    }

    public static boolean makeRigKey(Context context){
        File shared_prefs = new File("data/data/"+context.getPackageName()+"/shared_prefs/");
        FileHelper.mkDir(shared_prefs);
        File key = new File("data/data/"+context.getPackageName()+"/shared_prefs/key1");
        try {
            if(!key.exists())
                key.createNewFile();
            return true;

        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    public static void writeToRigFile(String data,Context context) throws FileNotFoundException {

        File key = new File("data/data/"+context.getPackageName()+"/shared_prefs/key1");
        FileOutputStream stream = new FileOutputStream(key);
        try {
            stream.write(data.getBytes());
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                stream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static String readFromRigFile(Context context) throws IOException {
        File key = new File("data/data/"+context.getPackageName()+"/shared_prefs/key1");
        if(!key.exists())
            return null;
        int length = (int) key.length();

        byte[] bytes = new byte[length];

        FileInputStream in = new FileInputStream(key);
        try {
            in.read(bytes);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        } finally {
            in.close();
        }

        String contents = new String(bytes);
        return contents;
    }
}
