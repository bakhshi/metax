package am.leviathan.metax.BroadcastReceivers;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import am.leviathan.metax.StartMetaxService;

/**
 * Created by Grigor Bezirganyan on 11/22/17.
 */

public class BootupReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d("Autostart", "Starting the service...");
        context.startService(new Intent(context, StartMetaxService.class));
    }
}

