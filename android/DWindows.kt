package org.dbsoft.dwindows.dwtest

import android.os.Bundle
import android.view.View
import android.widget.LinearLayout
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
    /*
     * These are the Android calls to actually create the UI...
     * forwarded from the C Dynamic Windows API
     */
    fun windowNew(title: String, style: Int): DWindows
    {
        // For now we just return our DWindows activity...
        // in the future, later calls should create new activities
        return this
    }

    fun boxNew(type: Int, pad: Int): LinearLayout
    {
        val box = LinearLayout(this)
        box.layoutParams =
            LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT)
        if (type > 0) {
            box.orientation = LinearLayout.VERTICAL
        } else {
            box.orientation = LinearLayout.HORIZONTAL
        }
        return box
    }

    fun boxPack(box: LinearLayout, item: View, index: Int, width: Int, height: Int, hsize: Int, vsize: Int, pad: Int)
    {
        var w: Int = width;
        var h: Int = height;

        if(hsize > 0) {
            w = LinearLayout.LayoutParams.FILL_PARENT
        } else if(w < 1) {
            w = LinearLayout.LayoutParams.WRAP_CONTENT
        }
        if(vsize > 0) {
            h = LinearLayout.LayoutParams.FILL_PARENT
        } else if(h < 1) {
            h = LinearLayout.LayoutParams.WRAP_CONTENT
        }
        item.layoutParams = LinearLayout.LayoutParams(w, h)
        box.addView(item)
    }

    /*
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