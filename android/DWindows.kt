package org.dbsoft.dwindows

import android.content.ClipData
import android.content.ClipboardManager
import android.content.DialogInterface
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.drawable.GradientDrawable
import android.media.AudioManager
import android.media.ToneGenerator
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.InputFilter
import android.text.InputFilter.LengthFilter
import android.text.method.PasswordTransformationMethod
import android.util.Log
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.*
import android.widget.SeekBar.OnSeekBarChangeListener
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.collection.SimpleArrayMap
import androidx.recyclerview.widget.RecyclerView
import androidx.viewpager.widget.ViewPager
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayout.OnTabSelectedListener
import com.google.android.material.tabs.TabLayoutMediator
import java.util.*


class DWTabViewPagerAdapter : RecyclerView.Adapter<DWTabViewPagerAdapter.DWEventViewHolder>() {
    public val viewList = mutableListOf<LinearLayout>()
    public val pageList = mutableListOf<Long>()
    public var currentPageID = 0L

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
            DWEventViewHolder(viewList.get(viewType))

    override fun getItemCount() = viewList.count()
    override fun getItemViewType(position: Int): Int {
        return position
    }
    override fun onBindViewHolder(holder: DWEventViewHolder, position: Int) {
        holder.setIsRecyclable(false);
    }

    class DWEventViewHolder(var view: View) : RecyclerView.ViewHolder(view)
}

class DWindows : AppCompatActivity() {
    var firstWindow: Boolean = true
    var windowLayout: LinearLayout? = null

    // We only want to call this once when the app starts up
    // By default Android will call onCreate for rotation and other
    // changes.  This is incompatible with Dynamic Windows...
    // Make sure the following is in your AndroidManifest.xml
    // android:configChanges="orientation|screenSize|screenLayout|keyboardHidden"
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Turn on rotation
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR)

        // Get the Android app path
        val m = packageManager
        var s = packageName
        val p = m.getPackageInfo(s!!, 0)
        s = p.applicationInfo.dataDir

        // Initialize the Dynamic Windows code...
        // This will start a new thread that calls the app's dwmain()
        dwindowsInit(s)
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)

        // Send a DW_SIGNAL_CONFIGURE on orientation change
        if(windowLayout != null) {
            var width: Int = windowLayout!!.width
            var height: Int = windowLayout!!.height

            eventHandlerInt(windowLayout as View, 1, width, height, 0, 0)
        }
    }

    /*
     * These are the Android calls to actually create the UI...
     * forwarded from the C Dynamic Windows API
     */
    fun windowNew(title: String, style: Int): LinearLayout? {
        if (firstWindow) {
            var dataArrayMap = SimpleArrayMap<String, Long>()
            windowLayout = LinearLayout(this)

            windowLayout!!.tag = dataArrayMap
            setContentView(windowLayout)
            this.title = title
            // For now we just return our DWindows' main activity layout...
            // in the future, later calls should create new activities
            firstWindow = false
            return windowLayout
        }
        return null
    }

    fun windowFromId(window: View, cid: Int): View {
        return window.findViewById(cid)
    }

    fun windowSetData(window: View, name: String, data: Long) {
        if (window.tag != null) {
            var dataArrayMap: SimpleArrayMap<String, Long> = window.tag as SimpleArrayMap<String, Long>

            if (data != 0L) {
                dataArrayMap.put(name, data)
            } else {
                dataArrayMap.remove(name)
            }
        }
    }

    fun windowGetData(window: View, name: String): Long {
        var retval: Long = 0L

        if (window.tag != null) {
            var dataArrayMap: SimpleArrayMap<String, Long> = window.tag as SimpleArrayMap<String, Long>

            retval = dataArrayMap.get(name)!!
        }
        return retval
    }

    fun windowSetEnabled(window: View, state: Boolean) {
        window.setEnabled(state)
    }

    fun windowSetText(window: View, text: String) {
        if (window is TextView) {
            var textview: TextView = window
            textview.text = text
        } else if (window is Button) {
            var button: Button = window
            button.text = text
        } else if (window is LinearLayout) {
            // TODO: Make sure this is actually the top-level layout, not just a box
            this.title = text
        }
    }

    fun windowGetText(window: View): String? {
        if (window is TextView) {
            var textview: TextView = window
            return textview.text.toString()
        } else if (window is Button) {
            var button: Button = window
            return button.text.toString()
        } else if (window is LinearLayout) {
            // TODO: Make sure this is actually the top-level layout, not just a box
            return this.title.toString()
        }
        return null
    }

    fun clipboardGetText(): String {
        var cm: ClipboardManager = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
        var clipdata = cm.primaryClip

        if (clipdata != null && clipdata.itemCount > 0) {
            return clipdata.getItemAt(0).coerceToText(this).toString()
        }
        return ""
    }

    fun clipboardSetText(text: String) {
        var cm: ClipboardManager = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
        var clipdata = ClipData.newPlainText("text", text)

        cm.setPrimaryClip(clipdata)
    }

    fun boxNew(type: Int, pad: Int): LinearLayout {
        val box = LinearLayout(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        box.tag = dataArrayMap
        box.layoutParams =
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT
                )
        if (type > 0) {
            box.orientation = LinearLayout.VERTICAL
        } else {
            box.orientation = LinearLayout.HORIZONTAL
        }
        box.setPadding(pad, pad, pad, pad)
        return box
    }

    fun boxPack(
        box: LinearLayout,
        item: View,
        index: Int,
        width: Int,
        height: Int,
        hsize: Int,
        vsize: Int,
        pad: Int
    ) {
        var w: Int = LinearLayout.LayoutParams.WRAP_CONTENT
        var h: Int = LinearLayout.LayoutParams.WRAP_CONTENT

        if (item is LinearLayout) {
            if (box.orientation == LinearLayout.VERTICAL) {
                if (hsize > 0) {
                    w = LinearLayout.LayoutParams.MATCH_PARENT
                }
            } else {
                if (vsize > 0) {
                    h = LinearLayout.LayoutParams.MATCH_PARENT
                }
            }
        }
        var params: LinearLayout.LayoutParams = LinearLayout.LayoutParams(w, h)

        if (item !is LinearLayout && (width != -1 || height != -1)) {
            item.measure(0, 0)
            if (width > 0) {
                w = width
            } else if (width == -1) {
                w = item.getMeasuredWidth()
            }
            if (height > 0) {
                h = height
            } else if (height == -1) {
                h = item.getMeasuredHeight()
            }
        }
        if (box.orientation == LinearLayout.VERTICAL) {
            if (vsize > 0) {
                if (w > 0) {
                    params.weight = w.toFloat()
                } else {
                    params.weight = 1F
                }
            }
        } else {
            if (hsize > 0) {
                if (h > 0) {
                    params.weight = h.toFloat()
                } else {
                    params.weight = 1F
                }
            }
        }
        if (pad > 0) {
            params.setMargins(pad, pad, pad, pad)
        }
        var grav: Int = Gravity.CLIP_HORIZONTAL or Gravity.CLIP_VERTICAL
        if (hsize > 0 && vsize > 0) {
            params.gravity = Gravity.FILL or grav
        } else if (hsize > 0) {
            params.gravity = Gravity.FILL_HORIZONTAL or grav
        } else if (vsize > 0) {
            params.gravity = Gravity.FILL_VERTICAL or grav
        }
        item.layoutParams = params
        box.addView(item, index)
    }

    fun boxUnpack(item: View) {
        var box: LinearLayout = item.parent as LinearLayout
        box.removeView(item)
    }

    fun boxUnpackAtIndex(box: LinearLayout, index: Int): View? {
        var item: View = box.getChildAt(index)

        box.removeView(item)
        return item
    }

    fun buttonNew(text: String, cid: Int): Button {
        val button = Button(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        button.tag = dataArrayMap
        button.text = text
        button.id = cid
        button.setOnClickListener {
            eventHandlerSimple(button, 8)
        }
        return button
    }

    fun entryfieldNew(text: String, cid: Int, password: Int): EditText {
        val entryfield = EditText(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        entryfield.tag = dataArrayMap
        entryfield.id = cid
        if (password > 0) {
            entryfield.transformationMethod = PasswordTransformationMethod.getInstance()
        }
        entryfield.setText(text)
        return entryfield
    }

    fun entryfieldSetLimit(entryfield: EditText, limit: Long) {
        entryfield.filters = arrayOf<InputFilter>(LengthFilter(limit.toInt()))
    }

    fun radioButtonNew(text: String, cid: Int): RadioButton {
        val radiobutton = RadioButton(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        radiobutton.tag = dataArrayMap
        radiobutton.id = cid
        radiobutton.text = text
        radiobutton.setOnClickListener {
            eventHandlerSimple(radiobutton, 8)
        }
        return radiobutton
    }

    fun checkboxNew(text: String, cid: Int): CheckBox {
        val checkbox = CheckBox(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        checkbox.tag = dataArrayMap
        checkbox.id = cid
        checkbox.text = text
        checkbox.setOnClickListener {
            eventHandlerSimple(checkbox, 8)
        }
        return checkbox
    }

    fun checkOrRadioSetChecked(control: View, state: Int)
    {
        if(control is CheckBox) {
            var checkbox: CheckBox = control
            checkbox.isChecked = state != 0
        } else if(control is RadioButton) {
            var radiobutton: RadioButton = control
            radiobutton.isChecked = state != 0
        }
    }

    fun checkOrRadioGetChecked(control: View): Boolean
    {
        if(control is CheckBox) {
            var checkbox: CheckBox = control
            return checkbox.isChecked
        } else if(control is RadioButton) {
            var radiobutton: RadioButton = control
            return radiobutton.isChecked
        }
        return false
    }

    fun textNew(text: String, cid: Int, status: Int): TextView {
        val textview = TextView(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        textview.tag = dataArrayMap
        textview.id = cid
        textview.text = text
        if (status != 0) {
            val border = GradientDrawable()

            // Set a black border on white background...
            // might need to change this to invisible...
            // or the color from windowSetColor
            border.setColor(-0x1)
            border.setStroke(1, -0x1000000)
            textview.background = border
        }
        return textview
    }

    fun notebookNew(cid: Int, top: Int): RelativeLayout
    {
        val notebook = RelativeLayout(this)
        val pager = ViewPager2(this)
        val tabs = TabLayout(this)
        var w: Int = RelativeLayout.LayoutParams.MATCH_PARENT
        var h: Int = RelativeLayout.LayoutParams.WRAP_CONTENT
        var dataArrayMap = SimpleArrayMap<String, Long>()

        notebook.tag = dataArrayMap
        notebook.id = cid
        tabs.id = View.generateViewId()
        pager.id = View.generateViewId()
        pager.adapter = DWTabViewPagerAdapter()
        TabLayoutMediator(tabs, pager) { tab, position ->
            // This code never gets called?
        }.attach()

        var params: RelativeLayout.LayoutParams = RelativeLayout.LayoutParams(w, h)
        if(top != 0) {
            params.addRule(RelativeLayout.ALIGN_PARENT_TOP)
        } else {
            params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM)
        }
        tabs.tabGravity = TabLayout.GRAVITY_FILL
        tabs.tabMode = TabLayout.MODE_FIXED
        notebook.addView(tabs, params)
        params = RelativeLayout.LayoutParams(w, w)
        if(top != 0) {
            params.addRule(RelativeLayout.BELOW, tabs.id)
        } else {
            params.addRule(RelativeLayout.ABOVE, tabs.id)
        }
        notebook.addView(pager, params)
        tabs.addOnTabSelectedListener(object : OnTabSelectedListener {
            override fun onTabSelected(tab: TabLayout.Tab) {
                val adapter = pager.adapter as DWTabViewPagerAdapter

                pager.currentItem = tab.position
                eventHandlerNotebook(notebook, 15, adapter.pageList[tab.position])
            }
            override fun onTabUnselected(tab: TabLayout.Tab) {}
            override fun onTabReselected(tab: TabLayout.Tab) {}
        })
        return notebook
    }

    fun notebookPageNew(notebook: RelativeLayout, flags: Long, front: Int): Long
    {
        var pager: ViewPager2? = null
        var tabs: TabLayout? = null
        var pageID = 0L

        if(notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
            pager = notebook.getChildAt(0) as ViewPager2
            tabs = notebook.getChildAt(1) as TabLayout
        } else if(notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
            pager = notebook.getChildAt(1) as ViewPager2
            tabs = notebook.getChildAt(0) as TabLayout
        }

        if(pager != null && tabs != null) {
            var adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
            var tab = tabs.newTab()

            // Increment our page ID... making sure no duplicates exist
            do {
                adapter.currentPageID += 1
            }
            while(adapter.currentPageID == 0L || adapter.pageList.contains(adapter.currentPageID))
            pageID = adapter.currentPageID
            // Temporarily add a black tab with an empty layout/box
            if(front != 0) {
                adapter.viewList.add(0, LinearLayout(this))
                adapter.pageList.add(0, pageID)
                tabs.addTab(tab, 0)
            } else {
                adapter.viewList.add(LinearLayout(this))
                adapter.pageList.add(pageID)
                tabs.addTab(tab)
            }
        }
        return pageID
    }

    fun notebookPageDestroy(notebook: RelativeLayout, pageID: Long)
    {
        var pager: ViewPager2? = null
        var tabs: TabLayout? = null

        if(notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
            pager = notebook.getChildAt(0) as ViewPager2
            tabs = notebook.getChildAt(1) as TabLayout
        } else if(notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
            pager = notebook.getChildAt(1) as ViewPager2
            tabs = notebook.getChildAt(0) as TabLayout
        }

        if(pager != null && tabs != null) {
            var adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
            val index = adapter.pageList.indexOf(pageID)
            val tab = tabs.getTabAt(index)

            if (tab != null) {
                adapter.viewList.removeAt(index)
                adapter.pageList.removeAt(index)
                tabs.removeTab(tab)
            }
        }
    }

    fun notebookPageSetText(notebook: RelativeLayout, pageID: Long, text: String)
    {
        var pager: ViewPager2? = null
        var tabs: TabLayout? = null

        if(notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
            pager = notebook.getChildAt(0) as ViewPager2
            tabs = notebook.getChildAt(1) as TabLayout
        } else if(notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
            pager = notebook.getChildAt(1) as ViewPager2
            tabs = notebook.getChildAt(0) as TabLayout
        }

        if(pager != null && tabs != null) {
            val adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
            val index = adapter.pageList.indexOf(pageID)
            val tab = tabs.getTabAt(index)

            if (tab != null) {
                tab.text = text
            }
        }
    }

    fun notebookPagePack(notebook: RelativeLayout, pageID: Long, box: LinearLayout)
    {
        var pager: ViewPager2? = null
        var tabs: TabLayout? = null

        if(notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
            pager = notebook.getChildAt(0) as ViewPager2
            tabs = notebook.getChildAt(1) as TabLayout
        } else if(notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
            pager = notebook.getChildAt(1) as ViewPager2
            tabs = notebook.getChildAt(0) as TabLayout
        }

        if(pager != null && tabs != null) {
            var adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
            val index = adapter.pageList.indexOf(pageID)

            // Make sure the box is MATCH_PARENT
            box.layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
            );

            adapter.viewList[index] = box
        }
    }

    fun notebookPageGet(notebook: RelativeLayout): Long
    {
        var pager: ViewPager2? = null
        var tabs: TabLayout? = null

        if(notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
            pager = notebook.getChildAt(0) as ViewPager2
            tabs = notebook.getChildAt(1) as TabLayout
        } else if(notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
            pager = notebook.getChildAt(1) as ViewPager2
            tabs = notebook.getChildAt(0) as TabLayout
        }

        if(pager != null && tabs != null) {
            var adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
            return adapter.pageList.get(tabs.selectedTabPosition)
        }
        return 0L
    }

    fun notebookPageSet(notebook: RelativeLayout, pageID: Long)
    {
        var pager: ViewPager2? = null
        var tabs: TabLayout? = null

        if(notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
            pager = notebook.getChildAt(0) as ViewPager2
            tabs = notebook.getChildAt(1) as TabLayout
        } else if(notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
            pager = notebook.getChildAt(1) as ViewPager2
            tabs = notebook.getChildAt(0) as TabLayout
        }

        if(pager != null && tabs != null) {
            val adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
            val index = adapter.pageList.indexOf(pageID)
            val tab = tabs.getTabAt(index)

            tabs.selectTab(tab)
        }
    }

    fun sliderNew(vertical: Int, increments: Int, cid: Int): SeekBar
    {
        val slider = SeekBar(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        slider.tag = dataArrayMap
        slider.id = cid
        slider.max = increments
        if(vertical != 0) {
            slider.rotation = 270F
        }
        slider.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onStopTrackingTouch(seekBar: SeekBar) {
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {
            }

            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                eventHandler(slider, null, 14, null, null, slider.progress, 0, 0, 0)
            }
        })
        return slider
    }

    fun percentNew(cid: Int): ProgressBar
    {
        val percent = ProgressBar(this)
        var dataArrayMap = SimpleArrayMap<String, Long>()

        percent.tag = dataArrayMap
        percent.id = cid
        percent.max = 100
        return percent
    }

    fun percentGetPos(percent: ProgressBar): Int
    {
        return percent.progress
    }

    fun percentSetPos(percent: ProgressBar, position: Int)
    {
        percent.progress = position
    }

    fun percentSetRange(percent: ProgressBar, range: Int)
    {
        percent.max = range
    }

    fun timerConnect(interval: Long, sigfunc: Long, data: Long): Timer
    {
        // creating timer task, timer
        val t = Timer()
        val tt: TimerTask = object : TimerTask() {
            override fun run() {
                if(eventHandlerTimer(sigfunc, data) == 0) {
                    t.cancel()
                }
            }
        }
        t.scheduleAtFixedRate(tt, interval, interval)
        return t
    }

    fun timerDisconnect(timer: Timer)
    {
        timer.cancel()
    }

    fun doBeep(duration: Int)
    {
        val toneGen = ToneGenerator(AudioManager.STREAM_ALARM, 100)
        toneGen.startTone(ToneGenerator.TONE_CDMA_PIP, duration)
        val handler = Handler(Looper.getMainLooper())
        handler.postDelayed({
            toneGen.release()
        }, (duration + 50).toLong())
    }

    fun debugMessage(text: String)
    {
        Log.d(null, text)
    }

    fun messageBox(title: String, body: String, flags: Int): Int
    {
        // make a text input dialog and show it
        var alert = AlertDialog.Builder(this)
        var retval: Int = 0

        alert.setTitle(title)
        alert.setMessage(body)
        if((flags and (1 shl 3)) != 0) {
            alert.setPositiveButton(
                "Yes",
                DialogInterface.OnClickListener { _: DialogInterface, _: Int ->
                    retval = 1
                    throw java.lang.RuntimeException()
                });
        }
        if((flags and ((1 shl 1) or (1 shl 2))) != 0) {
            alert.setNegativeButton(
                "Ok",
                DialogInterface.OnClickListener { _: DialogInterface, _: Int ->
                    retval = 0
                    throw java.lang.RuntimeException()
                });
        }
        if((flags and ((1 shl 3) or (1 shl 4))) != 0) {
            alert.setNegativeButton(
                "No",
                DialogInterface.OnClickListener { _: DialogInterface, _: Int ->
                    retval = 0
                    throw java.lang.RuntimeException()
                });
        }
        if((flags and ((1 shl 2) or (1 shl 4))) != 0) {
            alert.setNeutralButton(
                "Cancel",
                DialogInterface.OnClickListener { _: DialogInterface, _: Int ->
                    retval = 2
                    throw java.lang.RuntimeException()
                });
        }
        alert.show();

        // loop till a runtime exception is triggered.
        try {
            Looper.loop()
        } catch (e2: RuntimeException) {
        }
        return retval
    }

    fun dwindowsExit(exitcode: Int)
    {
        this.finishAffinity()
        System.exit(exitcode)
    }

    fun dwindowsShutdown()
    {
        this.finishAffinity()
    }

    /*
     * Native methods that are implemented by the 'dwindows' native library,
     * which is packaged with this application.
     */
    external fun dwindowsInit(dataDir: String)
    external fun eventHandler(
        obj1: View?,
        obj2: View?,
        message: Int,
        str1: String?,
        str2: String?,
        inta: Int,
        intb: Int,
        intc: Int,
        intd: Int
    ): Int
    external fun eventHandlerInt(
        obj1: View,
        message: Int,
        inta: Int,
        intb: Int,
        intc: Int,
        intd: Int
    )
    external fun eventHandlerSimple(obj1: View, message: Int)
    external fun eventHandlerNotebook(obj1: View, message: Int, pageID: Long)
    external fun eventHandlerTimer(sigfunc: Long, data: Long): Int

    companion object
    {
        // Used to load the 'dwindows' library on application startup.
        init
        {
            System.loadLibrary("dwindows")
        }
    }
}