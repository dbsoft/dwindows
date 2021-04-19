package org.dbsoft.dwindows.dwtest

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity


class DWindows : AppCompatActivity()
{
    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.dwindows_main)

        val m = packageManager
        var s = packageName
        val p = m.getPackageInfo(s!!, 0)
        s = p.applicationInfo.dataDir

        // Example of a call to a native method
        findViewById<TextView>(R.id.sample_text).text = dwindowsInit(s)
    }

    /**
     * Native methods that are implemented by the 'dwindows' native library,
     * which is packaged with this application.
     */
    external fun dwindowsInit(dataDir: String): String

    companion object
    {
        // Used to load the 'dwindows' library on application startup.
        init
        {
            System.loadLibrary("dwindows")
        }
    }
}