package com.smackall.iyan3dPro.Helper;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.view.WindowManager;
import android.widget.Toast;

import com.smackall.iyan3dPro.EditorView;
import com.smackall.iyan3dPro.R;

/**
 * Created by Sabish.M on 5/3/16.
 * Copyright (c) 2015 Smackall Games Pvt Ltd. All rights reserved.
 */
public class UIHelper {

    public static int ScreenType;

    public static void informDialog(final Context context, final String msg) {
        ((Activity) context).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder informDialog = new AlertDialog.Builder(context);
                informDialog
                        .setMessage(msg)
                        .setCancelable(false)
                        .setPositiveButton(context.getString(R.string.ok), new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                try {
                                    if (((EditorView) context).addToDownloadManager != null) {
                                        ((EditorView) context).addToDownloadManager.errorMessageShowing = false;
                                    }
                                } catch (ClassCastException e) {
                                    e.printStackTrace();
                                }
                                if (dialog != null)
                                    dialog.dismiss();
                            }
                        });
                informDialog.create();
                try {
                    informDialog.show();
                } catch (WindowManager.BadTokenException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    public static void informDialog(final Context context, final String msg, boolean dummy) {
        ((Activity) context).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder informDialog = new AlertDialog.Builder(context);
                informDialog
                        .setMessage(msg)
                        .setCancelable(false)
                        .setPositiveButton(context.getString(R.string.ok), new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                if (dialog != null)
                                    dialog.dismiss();
                            }
                        });
                informDialog.create();
                try {
                    informDialog.show();
                } catch (WindowManager.BadTokenException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    public static void showSignInPanelWithMessage(final Context context, final String msg) {
        ((Activity) context).runOnUiThread(new Runnable() {
            @Override
            public void run() {

                AlertDialog.Builder informDialog = new AlertDialog.Builder(context);
                informDialog
                        .setMessage(msg)
                        .setCancelable(false)
                        .setPositiveButton(context.getString(R.string.ok), new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                if (dialog != null)
                                    dialog.dismiss();
                            }
                        });
                informDialog.create();
                try {
                    informDialog.show();
                } catch (WindowManager.BadTokenException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    public static void setScreenCategory(Context context) {
        if ((context.getResources().getConfiguration().screenLayout &
                Configuration.SCREENLAYOUT_SIZE_MASK) ==
                Configuration.SCREENLAYOUT_SIZE_LARGE) {
            ScreenType = Constants.SCREEN_LARGE;
        } else if ((context.getResources().getConfiguration().screenLayout &
                Configuration.SCREENLAYOUT_SIZE_MASK) ==
                Configuration.SCREENLAYOUT_SIZE_NORMAL) {
            ScreenType = Constants.SCREEN_NORMAL;
        } else if ((context.getResources().getConfiguration().screenLayout &
                Configuration.SCREENLAYOUT_SIZE_MASK) ==
                Configuration.SCREENLAYOUT_SIZE_XLARGE) {
            ScreenType = Constants.SCREEN_XLARGE;
        } else {
            ScreenType = Constants.SCREEN_UNDEFINED;
        }
    }

    public static void dpi(Context mContext) {
        setScreenCategory(mContext);
        int density = mContext.getResources().getDisplayMetrics().densityDpi;

        if (density <= 120) {
            Toast.makeText(mContext, "LDPI", Toast.LENGTH_LONG).show();
        } else if (density <= 160) {
            Toast.makeText(mContext, "MDPI", Toast.LENGTH_LONG).show();
        } else if (density <= 240) {
            Toast.makeText(mContext, "HDPI", Toast.LENGTH_LONG).show();
        } else if (density <= 320) {
            Toast.makeText(mContext, "XHDPI", Toast.LENGTH_LONG).show();
        } else if (density <= 480) {
            Toast.makeText(mContext, "XXHDPI", Toast.LENGTH_LONG).show();
        } else if (density <= 640) {
            Toast.makeText(mContext, "XXXHDPI", Toast.LENGTH_LONG).show();
        }
    }
}
