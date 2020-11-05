package am.leviathan.metax.BroadcastReceivers;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import am.leviathan.metax.StartMetaxService;

/**
 * Created by glap on 11/22/17.
 */

public class StartMetaxReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        context.startService(new Intent(context, StartMetaxService.class));
    }
}
