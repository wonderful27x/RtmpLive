package com.example.wyrtmplive;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private SurfaceView surfaceView;
    private Button startLive;
    private Button stopLive;
    private Button switchCamera;
    private WonderfulPush wonderfulPush;
    private EditText address;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface);
        startLive = findViewById(R.id.startLive);
        stopLive = findViewById(R.id.stopLive);
        switchCamera = findViewById(R.id.switchCamera);
        address = findViewById(R.id.address);

        wonderfulPush = new WonderfulPush(this);

        List<String> permissionList = permissionCheck();
        if (permissionList.isEmpty()){
            wonderfulPush.setHolder(surfaceView.getHolder());
        }else {
            permissionRequest(permissionList,1);
        }

        switchCamera.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                wonderfulPush.switchCamera();
            }
        });

        startLive.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (wonderfulPush.getState() == LiveState.STOP){
                    startLive.setText("暂停");
                    wonderfulPush.startLive(address.getText().toString());
                }else if(wonderfulPush.getState() == LiveState.PAUSE){
                    startLive.setText("暂停");
                    wonderfulPush.liveControl(LiveState.LIVING);
                }else if(wonderfulPush.getState() == LiveState.LIVING){
                    startLive.setText("继续直播");
                    wonderfulPush.liveControl(LiveState.PAUSE);
                }
            }
        });

        stopLive.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                wonderfulPush.stopLive();
            }
        });

        // Example of a call to a native method
//        TextView tv = findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());
    }



    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String stringFromJNI();

    //判断是否授权所有权限
    private List<String> permissionCheck(){
        List<String> permissions = new ArrayList<>();
        if (!checkPermission(Manifest.permission.CAMERA)){
            permissions.add(Manifest.permission.CAMERA);
        }
        if (!checkPermission(Manifest.permission.RECORD_AUDIO)){
            permissions.add(Manifest.permission.RECORD_AUDIO);
        }
        if (!checkPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)){
            permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        }
        return permissions;
    }

    //发起权限申请
    private void permissionRequest(List<String> permissions,int requestCode){
        String[] permissionArray = permissions.toArray(new String[permissions.size()]);
        ActivityCompat.requestPermissions(this,permissionArray,requestCode);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if(requestCode == 1){
            if (grantResults.length >0){
                for (int result:grantResults){
                    if (result != PackageManager.PERMISSION_GRANTED){
                        Toast.makeText(MainActivity.this,"对不起，您拒绝了权限无法使用此功能！",Toast.LENGTH_SHORT).show();
                        return;
                    }
                }
                wonderfulPush.setHolder(surfaceView.getHolder());
            }else {
                Toast.makeText(MainActivity.this,"发生未知错误！",Toast.LENGTH_SHORT).show();
            }
        }
    }

    //判断是否有权限
    private boolean checkPermission(String permission){
        return ContextCompat.checkSelfPermission(this,permission) == PackageManager.PERMISSION_GRANTED;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        wonderfulPush.stopLive();
    }
}
