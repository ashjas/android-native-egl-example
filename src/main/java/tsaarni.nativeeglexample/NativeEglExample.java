//
// Copyright 2011 Tero Saarni
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package tsaarni.nativeeglexample;

import android.app.Activity;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.widget.Toast;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.View.OnClickListener;
import android.util.Log;


public class NativeEglExample extends Activity implements SurfaceHolder.Callback
{

    private static String TAG = "EglSample";

    private float mDensity = 0;
    private float mPreviousX =0;
    private float mPreviousY =0;
    private float mDeltaX = 0;
    private float mDeltaY = 0;

    private float mDownX = 0;
    private float mDownY = 0;
    private float mUpX = 0;
    private float mUpY = 0;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.i(TAG, "onCreate()");
        
        setContentView(R.layout.main);
        SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surfaceview);
        surfaceView.getHolder().addCallback(this);
       /* surfaceView.setOnClickListener(new OnClickListener() {
                public void onClick(View view) {
                    Toast toast = Toast.makeText(NativeEglExample.this,
                                                 "This demo combines Java UI and native EGL + OpenGL renderer",
                                                 Toast.LENGTH_LONG);
                    toast.show();
                }});*/
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.i(TAG, "onStart()");
        nativeOnStart();
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        mDensity = displayMetrics.density;

    }
    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (event != null)
        {
            float x = event.getX();
            float y = event.getY();
            switch(event.getAction())
            {
                case MotionEvent.ACTION_DOWN: {
                    Log.d(TAG,"ACTION_DOWN --> X:" + x +",Y:" + y);
                    mPreviousX = mDownX = x;
                    mPreviousY = mDownY = y;
                    //return true;
                    break;
                }
                case MotionEvent.ACTION_MOVE: {
                    //float deltaX = (x - mPreviousX) / mDensity / 2f;
                    //float deltaY = (y - mPreviousY) / mDensity / 2f;
                    float deltaX = (x - mPreviousX) ;
                    float deltaY = (y - mPreviousY) ;
                    mDeltaX = deltaX;
                    mDeltaY = deltaY;

                    mPreviousX = x;
                    mPreviousY = y;
                    setPan2(mDeltaX,mDeltaY);
                    //Log.d(TAG,"ACTION_MOVE --> X:" + x +",Y:" + y);
                    Log.d(TAG,"ACTION_MOVE --> dX:" + mDeltaX +",dY:" + mDeltaY);
                    //return true;
                    break;
                }
                case MotionEvent.ACTION_UP: {
                    Log.d(TAG,"ACTION_UP --> X:" + x +",Y:" + y);
                    //return true;
                    break;
                }
            }

        }


        return super.onTouchEvent(event);
    }
    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume()");
        nativeOnResume();
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        Log.i(TAG, "onPause()");
        nativeOnPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.i(TAG, "onStop()");
        nativeOnStop();
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        nativeSetSurface(holder.getSurface());
    }

    public void surfaceCreated(SurfaceHolder holder) {
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeSetSurface(null);
    }


    public static native void nativeOnStart();
    public static native void nativeOnResume();
    public static native void nativeOnPause();
    public static native void setPan2(float x,float y);
    public static native void nativeOnStop();
    public static native void nativeSetSurface(Surface surface);

    static {
        System.loadLibrary("nativeegl");
    }

}
