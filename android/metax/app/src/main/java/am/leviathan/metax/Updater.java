package am.leviathan.metax;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.util.Xml;

import org.xmlpull.v1.XmlPullParser;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import static android.content.Context.NOTIFICATION_SERVICE;


public class Updater extends AsyncTask<String, Integer, String> {

    private final String TAG = "Updater";
    private String mAPKFileUrl = "";
    private String mAPKFileName = "";
    private String mLatestVersionName = "";
    private String mUrlString = "";
    private String mResult = null;
    private int mCurrentVersionCode = 0;
    private int mLatestVersionCode = 0;

    private Context mContext = null;
    private Activity mActivity = null;
    private URL mUrl = null;

    public Updater (Context context) {
        this.mContext = context;
    }

    public Updater (Activity activity) {
        this.mActivity = activity;
        this.mContext = this.mActivity;
    }

    @Override
    protected String doInBackground(String... strings) {
        this.mUrlString = strings[0];
        if ((mResult = getLatestVersionInfo()) != null) return mResult;
        if ((mResult = downloadAPK()) != null) return mResult;
        if (strings[1].equals("Activity")) requestConfirmation();
        if (strings[1].equals("Service")) notifer();
        return null;
    }

    private String getLatestVersionInfo() {
        try {
            mCurrentVersionCode = mContext.getPackageManager()
                    .getPackageInfo(mContext.getPackageName(), 0).versionCode;
        } catch (Exception e) {
            return "getting app current version error: " + e.toString();
        }
        if (!isNetworkAvailable()) return "No internet connection";
        try {
            mUrl = new URL(mUrlString);
            HttpURLConnection connection = (HttpURLConnection) mUrl.openConnection();
            connection.setRequestMethod("GET");
            connection.setDoOutput(true);
            connection.connect();
            InputStream inputStream = connection.getInputStream();
            XmlPullParser p = Xml.newPullParser();
            p.setInput(inputStream, null);
            while (p.next() != XmlPullParser.END_DOCUMENT) {
                if (p.getEventType() == XmlPullParser.START_TAG) {
                    String n = p.getName();
                    p.next();
                    if (p.getEventType() == XmlPullParser.TEXT) {
                        String t = p.getText();
                        switch (n) {
                            case "latestVersionCode":
                                mLatestVersionCode = Integer.parseInt(t.trim());
                                break;
                            case "url":
                                mAPKFileUrl = t;
                                break;
                            case "latestVersion":
                                mLatestVersionName = t.trim();
                                break;
                            default:
                                break;
                        }
                        mAPKFileName = mLatestVersionName + "_" + mLatestVersionCode;
                    }
                }
            }
            return null;
        } catch (Exception e) {
            return "update checking error: " + e.toString();
        }
    }

    private String downloadAPK() {
        if (!isUpdateAvailable()) return "No update available";
        if (!isNetworkAvailable()) return "No internet connection";
        InputStream input = null;
        OutputStream output = null;
        HttpURLConnection connection = null;
        try {
            URL url = new URL(mAPKFileUrl);
            connection = (HttpURLConnection) url.openConnection();
            connection.connect();

            // expect HTTP 200 OK, so we don't mistakenly save error report
            // instead of the file
            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                return "Server returned HTTP " + connection.getResponseCode()
                        + " " + connection.getResponseMessage();
            }

            File file = new File("/storage/emulated/0/update.apk");

            if (!file.exists()) file.createNewFile();
            // download the file
            input = connection.getInputStream();
            output = new FileOutputStream(file);

            byte data[] = new byte[4096];
            int count;
            while ((count = input.read(data)) != -1) {
                output.write(data, 0, count);
            }
        } catch (Exception e) {
            return "update download error: " + e.toString();
        }
        return null;
    }

    private void notifer () {

        Intent resultIntent = new Intent(Intent.ACTION_VIEW);
        resultIntent.setDataAndType(Uri.fromFile(new File("/storage/emulated/0/update.apk")),
                "application/vnd.android.package-archive");
        resultIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK); // without this flag android returned a intent error!

        PendingIntent resultPendingIntent =
                PendingIntent.getActivity(
                        mContext,
                        0,
                        resultIntent,
                        PendingIntent.FLAG_UPDATE_CURRENT
                );
        NotificationCompat.Builder mBuilder =
                new NotificationCompat.Builder(mContext)
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .setContentTitle("Metax update").setVibrate(new long[] { 600, 500})
                        .setContentText("Touch for install update!")
                        .setContentIntent(resultPendingIntent);

        int mNotificationId = 001;
        NotificationManager mNotifyMgr =
                (NotificationManager) mContext.getSystemService(NOTIFICATION_SERVICE);
        mNotifyMgr.notify(mNotificationId, mBuilder.build());
    }

    private void requestConfirmation() {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                new AlertDialog.Builder(mActivity)
                        .setTitle("Update Available")
                        .setMessage("Downloaded " + mLatestVersionName + " version of Metax \nDo you want update this app")
                        .setNegativeButton("Cancel", null)
                        .setPositiveButton("Update", new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                installAPK();
                            }
                        }).show();
            }
        });
    }

    private void installAPK() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.fromFile(new File("/storage/emulated/0/update.apk")),
                "application/vnd.android.package-archive");
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK); // without this flag android returned a intent error!
        mContext.startActivity(intent);
    }

    @Override
    protected void onPostExecute(String result) {
        Log.d("Updater", "AsyncTask has been finished");
        if (result != null)
            Log.e(TAG,"Update error: " + result);
        else
            Log.d(TAG,"File downloaded");
    }


    private boolean isNetworkAvailable() {
        ConnectivityManager connectivityManager
                = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
        return activeNetworkInfo != null && activeNetworkInfo.isConnected();
    }

    private boolean isUpdateAvailable () {
        if (mLatestVersionCode > mCurrentVersionCode && !mAPKFileName.isEmpty()) return true;
        return false;
    }
}
