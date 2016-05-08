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
import android.util.FloatMath;
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
    private static float RES = 20;
    private float mDensity = 0;
    private float mPreviousX =0;
    private float mPreviousY =0;
    private float mDeltaX = 0;
    private float mDeltaY = 0;

    private float mDownX = 0;
    private float mDownY = 0;
    private float mUpX = 0;
    private float mUpY = 0;
    private float w = 0;
    private float h = 0;
    private float distance = 0;
    private int fingers = 0;
    private int zoomActive = 0;
    private float o_camX = 0.0f;
    private float o_camY = 0.0f;
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
        w = displayMetrics.widthPixels;
        h = displayMetrics.heightPixels;
        Log.d(TAG,"w:" + displayMetrics.widthPixels +",h:" + displayMetrics.heightPixels);

    }
    protected final float fingerDist(MotionEvent event){
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
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
                    mPreviousX = x;
                    mPreviousY = y;
                    if(event.getPointerCount() == 3) {
                        mDownX = event.getX(2);
                        mDownY = event.getY(2);
                    }
                    break;
                }
                case MotionEvent.ACTION_MOVE: {

                    if(zoomActive != 1 && event.getPointerCount() == 1) {
                        if(fingers == 0)
                            fingers = 1;
                        if(fingers == 2 )
                            fingers = 1;
                        else {
                            mDeltaX += (x - mPreviousX) ;
                            mDeltaY += (y - mPreviousY) ;
                            mPreviousX = x;
                            mPreviousY = y;
                            setPan2(mDeltaX, mDeltaY);
                        }
                    }

                    if(event.getPointerCount() == 2)
                    {
                        zoomActive = 1;
                        Log.d(TAG,"2 fingers.. var:"+fingers);
                        if(fingers == 1)
                        {
                            distance = fingerDist(event);
                            fingers = 2;
                        }
                        if(fingers == 2) {

                            float newDist = fingerDist(event);
                            float d = distance/newDist;
                            distance = newDist;
                            Log.d(TAG,"zoom:"+d);
                            setZoom(d);
                        }
                    }
                    if(event.getPointerCount() == 3)
                    {
                        x = event.getX(2);
                        y = event.getY(2);
                        o_camX += x - mDownX;
                        o_camY += y - mDownY;
                        mDownX = x;
                        mDownY = y;
                        setOrthoCam(o_camX,o_camY);
                    }
                    //Log.d(TAG,"ACTION_MOVE --> dX:" + mDeltaX +",dY:" + mDeltaY);
                   // Log.d(TAG,"fingers:"+event.getPointerCount());
                    break;
                }
                case MotionEvent.ACTION_UP: {
                    zoomActive = 0;
                    Log.d(TAG,"ACTION_UP --> X:" + x +",Y:" + y);
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
    public static native void setOrthoCam(float x,float y);
    public static native void setZoom(float d);
    public static native void nativeOnStop();
    public static native void nativeSetSurface(Surface surface);

    static {
        System.loadLibrary("nativeegl");
    }

}
