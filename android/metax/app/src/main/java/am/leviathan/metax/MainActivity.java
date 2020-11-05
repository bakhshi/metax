package am.leviathan.metax;

import android.Manifest;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.crashlytics.android.Crashlytics;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends AppCompatActivity {

    private static final int EXTERNAL_STORAGE_REQUEST_CODE = 123;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        // Prompt Android 6 user if permission is not yet granted
        if (ContextCompat.checkSelfPermission(this,
                                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                        || ContextCompat.checkSelfPermission(this,
                                Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                        || ContextCompat.checkSelfPermission(this,
                                Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this,
                                new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                        Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.INTERNET},
                                        EXTERNAL_STORAGE_REQUEST_CODE);
        } else {
                File dir = createDirectory(getApplicationContext());
                removeTmpFiles(dir);
                if (!isMyServiceRunning(StartMetaxService.class)) {
                        startService(new Intent(getApplicationContext(), StartMetaxService.class));
                }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        new Updater(MainActivity.this)
                .execute("http://172.17.7.25/metax/changelog.xml", "Activity");
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
            switch (requestCode) {
                    case EXTERNAL_STORAGE_REQUEST_CODE:
                            if (grantResults[0] == PackageManager.PERMISSION_GRANTED
                                            && grantResults[1] == PackageManager.PERMISSION_GRANTED
                                            && grantResults[2] == PackageManager.PERMISSION_GRANTED) {
                                    File dir = createDirectory(getApplicationContext());
                                    removeTmpFiles(dir);
                                    if (!isMyServiceRunning(StartMetaxService.class)) {
                                            startService(new Intent(getApplicationContext(), StartMetaxService.class));
                                    }
                            } else {
                                    Toast.makeText(this,  "Read/Write External Storage Not Granted", Toast.LENGTH_LONG).show();
                            }
            }
    }

    private boolean isMyServiceRunning(Class<?> serviceClass) {
            ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
            for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
                    if (serviceClass.getName().equals(service.service.getClassName())) {
                            return true;
                    }
            }
            return false;
    }

    public static void removeTmpFiles(File rootFolder) {
            if (! rootFolder.isDirectory()) {
                    return;
            }
            File[] childFiles = rootFolder.listFiles();
            if (childFiles == null || childFiles.length == 0) {
                    return;
            }
            String tmp = "tmp";
            for (File childFile : childFiles){
                    if (childFile.getName().toLowerCase().contains(tmp)) {
                            childFile.delete();
                    }
            }
    }

    public static void copyAssetFile(Context context, String d, String cn) {
            AssetManager am = context.getAssets();
            try {
                    File c = new File(d + cn);
                    if (! c.exists()) {
                            copyAsset(am, cn, d  + cn);
                            Log.d("Leviathan", " Copied from assets file: " + cn);
                    }
            } catch (Exception e) {
                    e.printStackTrace();
            }
    }

    public static void createKeysDirectory(Context context, String d) {
	    String dirPath = d + "/keys/";
            File dir1 = new File(dirPath);
            if (! dir1.exists()) {
                    dir1.mkdirs();
            }
            copyAssetFile(context, dirPath, "user_json_info.json");
	    dirPath = d + "/keys/user/";
            File dir2 = new File(dirPath);
            if (! dir2.exists()) {
                    dir2.mkdirs();
            }
            copyAssetFile(context, dirPath, "private.pem");
            copyAssetFile(context, dirPath, "public.pem");
    }

    public static File createDirectory(Context context) {
            String dirPath = context.getExternalFilesDir(null).getAbsolutePath() + "/";
            File dir = new File(dirPath);
            Log.d("LEVIATHAN: DIR_PATH", dirPath);
            if (! dir.exists()) {
                    dir.mkdirs();
            }
            copyAssetFile(context, dirPath, "config.xml");
            //new camera/doorbell setup config file.
            copyAssetFile(context, dirPath, "setup_config.xml");
            //copyAssetFile(context, dirPath, "user.json");
            //copyAssetFile(context, dirPath, "peers_rating.json");
            //createKeysDirectory(context, dirPath);
            return dir;
    }

    private static boolean copyAsset(AssetManager assetManager,
                    String fromAssetPath, String toPath) {
            InputStream in = null;
            OutputStream out = null;
            try {
                    in = assetManager.open(fromAssetPath);
                    new File(toPath).createNewFile();
                    out = new FileOutputStream(toPath);
                    copyFile(in, out);
                    in.close();
                    in = null;
                    out.flush();
                    out.close();
                    out = null;
                    return true;
            } catch(Exception e) {
                    e.printStackTrace();
                    return false;
            }
    }

    private static void copyFile(InputStream in, OutputStream out) throws IOException {
            byte[] buffer = new byte[1024];
            int read;
            while((read = in.read(buffer)) != -1){
                    out.write(buffer, 0, read);
            }
    }
}
