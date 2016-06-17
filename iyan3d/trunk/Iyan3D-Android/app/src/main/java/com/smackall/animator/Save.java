package com.smackall.animator;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;
import android.text.InputType;
import android.widget.EditText;

import com.smackall.animator.Helper.AnimDB;
import com.smackall.animator.Helper.Constants;
import com.smackall.animator.Helper.DatabaseHelper;
import com.smackall.animator.Helper.FileHelper;
import com.smackall.animator.Helper.PathManager;
import com.smackall.animator.Helper.UIHelper;
import com.smackall.animator.opengl.GL2JNILib;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipOutputStream;

/**
 * Created by Sabish.M on 23/3/16.
 * Copyright (c) 2015 Smackall Games Pvt Ltd. All rights reserved.
 */
public class Save {

    private Context mContext;
    private DatabaseHelper db;
    private static final int BUFFER = 2048;

    public Save(Context context,DatabaseHelper db){
        this.mContext = context;
        this.db = db;
    }

    public void enterNameForAnimation()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle("Save your Animation as template");
        final EditText input = new EditText(mContext);
        input.setHint("Animation Name");
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        builder.setView(input);
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                saveAnimation(input.getText().toString());
            }
        });
        builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });
        builder.show();
    }

    public void saveAnimation(final String name)
    {
        if(name.length() == 0){ UIHelper.informDialog(mContext,"Animation name cannot be empty."); return;}
        if(FileHelper.isItHaveSpecialChar(name)) {UIHelper.informDialog(mContext,"Animation Name cannot contain any special characters.");return;}
        final int assetId = 80000 + db.getNextAnimationAssetId();
        final int type = (GL2JNILib.getNodeType(GL2JNILib.getSelectedNodeId()) == Constants.NODE_TEXT_SKIN) ? 1 : 0;
        ((EditorView)((Activity)mContext)).glView.queueEvent(new Runnable() {
            @Override
            public void run() {
                GL2JNILib.saveAnimation(((EditorView)mContext).nativeCallBacks,assetId,name,type);
            }
        });
    }

    public void addToDatabase(final boolean status, final String name, final int type)
    {
        ((Activity)mContext).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (status) {
                    final int assetId = 80000 + db.getNextAnimationAssetId();
                    AnimDB animDB = new AnimDB();
                    animDB.setAssetsId(assetId);
                    animDB.setAnimName(name);
                    animDB.setPublishedId(0);
                    animDB.setRating(0);
                    animDB.setBonecount(GL2JNILib.jointSize(GL2JNILib.getSelectedNodeId()));
                    animDB.setAnimType(type);
                    animDB.setUserid(((EditorView)(Activity)mContext).userDetails.uniqueId);
                    animDB.setUserName(((EditorView)(Activity)mContext).userDetails.userName);
                    animDB.setKeyword(((EditorView)(Activity)mContext).userDetails.userName);
                    animDB.setUploaded(new SimpleDateFormat("dd-MM-yyyy HH:mm:ss", Locale.getDefault()).format(new Date()));
                    db.addNewMyAnimationAssets(animDB);
                    ((EditorView)((Activity)mContext)).imageManager.makeThumbnail(PathManager.LocalAnimationFolder+"/"+assetId+".png");
                    UIHelper.informDialog(mContext,"Animation Saved Successfully.");
                }
                else
                    System.out.println("Error");
            }
        });
    }

    public void saveI3DFile(String fileName)
    {
        ArrayList<String> fileNames = ((EditorView)mContext).zipManager.getFiles(true);
        String zipname = fileName+".i3d";
        String path  =PathManager.LocalProjectFolder+"/";

        try  {
            BufferedInputStream origin = null;
            FileOutputStream dest = new FileOutputStream(path+zipname);

            ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(dest));
            byte data[] = new byte[BUFFER];

            for(int i=0; i < fileNames.size(); i++) {
                if(!FileHelper.checkValidFilePath(fileNames.get(i))) {
                    break;
                }
                FileInputStream fi = new FileInputStream(fileNames.get(i));
                origin = new BufferedInputStream(fi, BUFFER);
                ZipEntry entry = new ZipEntry(fileNames.get(i).substring(fileNames.get(i).lastIndexOf("/") + 1));
                try {
                    out.putNextEntry(entry);
                }
                catch (ZipException e){
                    e.printStackTrace();
                    continue;
                }
                int count;
                while ((count = origin.read(data, 0, BUFFER)) != -1) {
                    out.write(data, 0, count);
                }
                origin.close();
            }

            out.finish();
            out.close();
        } catch(Exception e) {
            e.printStackTrace();
        }
    }
}
