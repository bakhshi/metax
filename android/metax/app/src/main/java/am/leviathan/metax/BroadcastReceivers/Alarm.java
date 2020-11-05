package am.leviathan.metax.BroadcastReceivers;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.SystemClock;
import android.util.Log;

import am.leviathan.metax.Updater;

import static android.content.ContentValues.TAG;

public class Alarm extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive: Alarm received");

        new Updater(context)
                .execute("http://172.17.7.25/metax/changelog.xml", "Service");

//TODO This is old update implementation, which needs to be deleted
//        AppUpdater appUpdater = new AppUpdater(context)
//                .setDisplay(Display.NOTIFICATION)
//                .setUpdateFrom(UpdateFrom.XML)
//                .setUpdateXML("http://172.17.7.25/metax/changelog.xml")
//                .setTitleOnUpdateAvailable("Update available")
//                .setButtonUpdate("Update now")
//	            .setButtonDismiss("Maybe later")
//	            .setButtonDoNotShowAgain(null)
//                .setIcon(R.mipmap.ic_launcher_round) // Notification icon
//                .setCancelable(false);
//        appUpdater.start();
    }

    public void setAlarm(Context context) {

        AlarmManager alarmMgr = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
        Intent i = new Intent(context, Alarm.class);
        PendingIntent pi = PendingIntent.getBroadcast(context, 0, i, 0);

        if (alarmMgr != null) {
            alarmMgr.setInexactRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                    SystemClock.elapsedRealtime() + AlarmManager.INTERVAL_DAY,
                    AlarmManager.INTERVAL_DAY, pi);
        }
        Log.d(TAG, "onReceive: Alarm set");
    }

    public void cancelAlarm(Context context) {
        Intent intent = new Intent(context, Alarm.class);
        PendingIntent sender = PendingIntent.getBroadcast(context, 0, intent, 0);
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        if (alarmManager != null) {
            alarmManager.cancel(sender);
        }
        Log.d(TAG, "onReceive: Alarm canceled");
    }
}
