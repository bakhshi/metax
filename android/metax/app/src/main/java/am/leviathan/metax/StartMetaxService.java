package am.leviathan.metax;

import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Environment;
import android.os.IBinder;
import android.support.v7.app.NotificationCompat;
import android.util.Log;

import java.io.File;

import am.leviathan.metax.BroadcastReceivers.Alarm;

import static android.content.ContentValues.TAG;

public class StartMetaxService extends Service {
    Alarm alarm;
    Integer m_start_id;
    public StartMetaxService() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate: Starting Metax Service...");

        Thread t = new Thread("MetaxService(" + m_start_id + ")") {
            @Override
            public void run() {
                _onStart();
            }
        };
        t.start();

        //Intent when notification is clicked
        Intent mainIntent = new Intent(this, MainActivity.class);
        mainIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);

        PendingIntent pending = PendingIntent.getActivity(this, 0, mainIntent, 0);

        //Starting service in foreground.
        NotificationCompat.Builder notification = new NotificationCompat.Builder(this);
        notification.setOngoing(true)
                .setContentTitle("Metax")
                .setContentText("Service running...")
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pending);
        startForeground(2, notification.build());

        alarm = new Alarm();
        alarm.setAlarm(this);
    }

    @Override
    public int onStartCommand(final Intent intent, int flags, final int startId) {
        super.onStartCommand(intent, flags, startId);
        Log.d(TAG, "onStartCommand: Handling Metax Service Start command.");
        m_start_id = startId;

        return START_STICKY;
    }

    private void _onStart() {
        //Your Start-Code for the service
        // Perform your long running operations here.
        Log.d("Leviathan", "Starting Metax");
        //String dirPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + getPackageName() + "/";
        String dirPath = getApplicationContext().getExternalFilesDir(null).getAbsolutePath() + "/";
        File dir = MainActivity.createDirectory(this);
        MainActivity.removeTmpFiles(dir);
        startWebServerAndMetax(dirPath, "config.xml");
        //startMetax(dirPath);
    }

    @Override
    public void onDestroy() {
        alarm.cancelAlarm(this);
    }

    /* A native method that is implemented by the
     * 'metax_web_api' native library, which is packaged
     * with this application.
     */

    //public native void startMetax(String dir);
    public static native void startWebServerAndMetax(String dir, String conf_file);

    public static native void stopMetaxServer();

	/* this is used to load the 'metax_web_api' library on application
     * startup. The library has already been unpacked into
     * /data/data/node.leviathan.am.leviathan/lib/libmetax_web_api.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("metax_web_api");
    }
}
