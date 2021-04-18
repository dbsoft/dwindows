package org.dbsoft.dwindows.dwtest

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView

class DWindows : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.dwindows_main)

        // Example of a call to a native method
        findViewById<TextView>(R.id.sample_text).text = stringFromJNI()
    }

    /**
     * A native method that is implemented by the 'dwindows' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'dwindows' library on application startup.
        init {
            System.loadLibrary("dwindows")
        }
    }
}