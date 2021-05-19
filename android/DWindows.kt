package org.dbsoft.dwindows

import android.R
import android.app.Activity
import android.app.Dialog
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.DialogInterface
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.*
import android.graphics.drawable.BitmapDrawable
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.media.AudioManager
import android.media.ToneGenerator
import android.os.*
import android.text.InputFilter
import android.text.InputFilter.LengthFilter
import android.text.InputType
import android.text.method.PasswordTransformationMethod
import android.util.Base64
import android.util.Log
import android.util.SparseBooleanArray
import android.util.TypedValue
import android.view.*
import android.view.View.OnTouchListener
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.*
import android.widget.AdapterView.OnItemClickListener
import android.widget.SeekBar.OnSeekBarChangeListener
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatEditText
import androidx.collection.SimpleArrayMap
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.constraintlayout.widget.ConstraintSet
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.res.ResourcesCompat
import androidx.core.view.MenuCompat
import androidx.recyclerview.widget.RecyclerView
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayout.OnTabSelectedListener
import com.google.android.material.tabs.TabLayoutMediator
import java.io.File
import java.io.FileInputStream
import java.io.FileNotFoundException
import java.io.IOException
import java.util.*
import java.util.concurrent.locks.ReentrantLock


class DWTabViewPagerAdapter : RecyclerView.Adapter<DWTabViewPagerAdapter.DWEventViewHolder>() {
    val viewList = mutableListOf<LinearLayout>()
    val pageList = mutableListOf<Long>()
    var currentPageID = 0L

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

private class DWWebViewClient : WebViewClient() {
    //Implement shouldOverrideUrlLoading//
    override fun shouldOverrideUrlLoading(view: WebView, url: String): Boolean {
        // We always want to load in our own WebView,
        // to match the behavior on the other platforms
        return false
    }
    override fun onPageStarted(view: WebView, url: String, favicon: Bitmap?) {
        // Handle the DW_HTML_CHANGE_STARTED event
        eventHandlerHTMLChanged(view, 19, url, 1)
    }

    override fun onPageFinished(view: WebView, url: String) {
        // Handle the DW_HTML_CHANGE_COMPLETE event
        eventHandlerHTMLChanged(view, 19, url, 4)
    }

    external fun eventHandlerHTMLChanged(obj1: View, message: Int, URI: String, status: Int)
}

class DWSpinButton(context: Context) : AppCompatEditText(context), OnTouchListener {
    var value: Long = 0
    var minimum: Long = 0
    var maximum: Long = 65535

    init {
        setCompoundDrawablesWithIntrinsicBounds(android.R.drawable.ic_media_previous, 0, android.R.drawable.ic_media_next, 0);
        setOnTouchListener(this)
    }

    override fun onTouch(v: View, event: MotionEvent): Boolean {
        val DRAWABLE_RIGHT = 2
        val DRAWABLE_LEFT = 0

        if (event.action == MotionEvent.ACTION_UP) {
            if (event.x >= v.width - (v as EditText)
                    .compoundDrawables[DRAWABLE_RIGHT].bounds.width()
            ) {
                val newvalue = this.text.toString().toLongOrNull()

                if(newvalue != null) {
                    value = newvalue + 1
                } else {
                    value += 1
                }
                if(value > maximum) {
                    value = maximum
                }
                if(value < minimum) {
                    value = minimum
                }
                setText(value.toString())
                eventHandlerInt(14, value.toInt(), 0, 0, 0)
                return true
            } else if (event.x <= (v as EditText)
                    .compoundDrawables[DRAWABLE_LEFT].bounds.width()
            ) {
                val newvalue = this.text.toString().toLongOrNull()

                if(newvalue != null) {
                    value = newvalue - 1
                } else {
                    value -= 1
                }
                if(value > maximum) {
                    value = maximum
                }
                if(value < minimum) {
                    value = minimum
                }
                setText(value.toString())
                eventHandlerInt(14, value.toInt(), 0, 0, 0)
                return true
            }
        }
        return false
    }

    external fun eventHandlerInt(
        message: Int,
        inta: Int,
        intb: Int,
        intc: Int,
        intd: Int
    )
}

class DWComboBox(context: Context) : AppCompatEditText(context), OnTouchListener, OnItemClickListener {
    var lpw: ListPopupWindow? = null
    var list = mutableListOf<String>()
    var selected: Int = -1

    init {
        setCompoundDrawablesWithIntrinsicBounds(0, 0, android.R.drawable.arrow_down_float, 0);
        setOnTouchListener(this)
        lpw = ListPopupWindow(context)
        lpw!!.setAdapter(
            ArrayAdapter(
                context,
                android.R.layout.simple_list_item_1, list
            )
        )
        lpw!!.anchorView = this
        lpw!!.isModal = true
        lpw!!.setOnItemClickListener(this)
    }

    override fun onItemClick(parent: AdapterView<*>?, view: View, position: Int, id: Long) {
        val item = list[position]
        selected = position
        setText(item)
        lpw!!.dismiss()
        eventHandlerInt(11, position, 0, 0, 0)
    }

    override fun onTouch(v: View, event: MotionEvent): Boolean {
        val DRAWABLE_RIGHT = 2

        if (event.action == MotionEvent.ACTION_UP) {
            if (event.x >= v.width - (v as EditText)
                    .compoundDrawables[DRAWABLE_RIGHT].bounds.width()
            ) {
                lpw!!.show()
                return true
            }
        }
        return false
    }

    external fun eventHandlerInt(
        message: Int,
        inta: Int,
        intb: Int,
        intc: Int,
        intd: Int
    )
}

class DWListBox(context: Context) : ListView(context), OnItemClickListener {
    var list = mutableListOf<String>()
    var selected: Int = -1

    init {
        setAdapter(
            ArrayAdapter(
                context,
                android.R.layout.simple_list_item_1, list
            )
        )
        onItemClickListener = this
    }

    override fun onItemClick(parent: AdapterView<*>?, view: View, position: Int, id: Long) {
        selected = position
        eventHandlerInt(11, position, 0, 0, 0)
    }

    external fun eventHandlerInt(
        message: Int,
        inta: Int,
        intb: Int,
        intc: Int,
        intd: Int
    )
}

class DWRender(context: Context) : View(context) {
    var cachedCanvas: Canvas? = null
    var typeface: Typeface? = null
    var fontsize: Float? = null

    override fun onSizeChanged(width: Int, height: Int, oldWidth: Int, oldHeight: Int) {
        super.onSizeChanged(width, height, oldWidth, oldHeight)
        // Send DW_SIGNAL_CONFIGURE
        eventHandlerInt(1, width, height, 0, 0)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        cachedCanvas = canvas
        // Send DW_SIGNAL_EXPOSE
        eventHandlerInt(7, 0, 0, this.width, this.height)
        cachedCanvas = null
    }

    external fun eventHandlerInt(
        message: Int,
        inta: Int,
        intb: Int,
        intc: Int,
        intd: Int
    )
}

class DWFileChooser(private val activity: Activity) {
    private val list: ListView = ListView(activity)
    private val dialog: Dialog = Dialog(activity)
    private var currentPath: File? = null

    // filter on file extension
    private var extension: String? = null
    fun setExtension(extension: String?) {
        this.extension = extension?.toLowerCase(Locale.ROOT)
    }

    // file selection event handling
    interface FileSelectedListener {
        fun fileSelected(file: File?)
    }

    fun setFileListener(fileListener: FileSelectedListener?): DWFileChooser {
        this.fileListener = fileListener
        return this
    }

    private var fileListener: FileSelectedListener? = null
    fun showDialog() {
        dialog.show()
    }

    /*
     * Sort, filter and display the files for the given path.
     */
    private fun refresh(path: File?) {
        currentPath = path
        if (path != null) {
            if (path.exists()) {
                val dirs = path.listFiles { file -> file.isDirectory && file.canRead() }
                val files = path.listFiles { file ->
                    if (!file.isDirectory) {
                        if (!file.canRead()) {
                            false
                        } else if (extension == null) {
                            true
                        } else {
                            file.name.toLowerCase(Locale.ROOT).endsWith(extension!!)
                        }
                    } else {
                        false
                    }
                }

                // convert to an array
                var i = 0
                val fileList: Array<String?>
                var filecount = 0
                var dircount = 0
                if(files != null) {
                    filecount = files.size
                }
                if(dirs != null) {
                    dircount = dirs.size
                }
                if (path.parentFile == null) {
                    fileList = arrayOfNulls(dircount + filecount)
                } else {
                    fileList = arrayOfNulls(dircount + filecount + 1)
                    fileList[i++] = PARENT_DIR
                }
                if(dirs != null) {
                    Arrays.sort(dirs)
                    for (dir in dirs) {
                        fileList[i++] = dir.name
                    }
                }
                if(files != null) {
                    Arrays.sort(files)
                    for (file in files) {
                        fileList[i++] = file.name
                    }
                }

                // refresh the user interface
                dialog.setTitle(currentPath!!.path)
                list.adapter = object : ArrayAdapter<Any?>(
                    activity,
                    R.layout.simple_list_item_1, fileList
                ) {
                    override fun getView(pos: Int, view: View?, parent: ViewGroup): View {
                        val thisview = super.getView(pos, view, parent)
                        (thisview as TextView).isSingleLine = true
                        return thisview
                    }
                }
            }
        }
    }

    /**
     * Convert a relative filename into an actual File object.
     */
    private fun getChosenFile(fileChosen: String): File? {
        return if (fileChosen == PARENT_DIR) {
            currentPath!!.parentFile
        } else {
            File(currentPath, fileChosen)
        }
    }

    companion object {
        private const val PARENT_DIR = ".."
    }

    init {
        list.onItemClickListener =
            OnItemClickListener { parent, view, which, id ->
                val fileChosen = list.getItemAtPosition(which) as String
                val chosenFile: File? = getChosenFile(fileChosen)
                if (chosenFile != null) {
                    if (chosenFile.isDirectory) {
                        refresh(chosenFile)
                    } else {
                        if (fileListener != null) {
                            fileListener!!.fileSelected(chosenFile)
                        }
                        dialog.dismiss()
                    }
                }
            }
        dialog.setContentView(list)
        dialog.window?.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
        refresh(Environment.getExternalStorageDirectory())
    }
}

// On Android we can't pre-create submenus...
// So create our own placeholder classes, and create the actual menus
// on demand when required by Android
class DWMenuItem
{
    var title: String? = null
    var menu: DWMenu? = null
    var submenu: DWMenu? = null
    var checked: Boolean = false
    var check: Boolean = false
    var enabled: Boolean = true
    var menuitem: MenuItem? = null
    var submenuitem: SubMenu? = null
    var id: Int = 0
}

class DWMenu {
    var menu: Menu? = null
    var children = mutableListOf<DWMenuItem>()
    var id: Int = 0

    fun createMenu(newmenu: Menu?) {
        if(menu == null) {
            menu = newmenu
        }
        if(menu != null) {
            var group: Int = 0

            // Enable group dividers for separators
            MenuCompat.setGroupDividerEnabled(menu, true);

            for (menuitem in children) {
                // Submenus on Android can't have submenus, so stop at depth 1
                if (menuitem.submenu != null && menu !is SubMenu) {
                    if(menuitem.submenuitem == null) {
                        menuitem.submenuitem = menu?.addSubMenu(group, menuitem.id, 0, menuitem.title)
                    }
                    menuitem.submenu!!.createMenu(menuitem.submenuitem)
                } else if(menuitem.submenu == null) {
                    if(menuitem.title!!.isEmpty()) {
                        group += 1
                    } else if(menuitem.menuitem == null) {
                        menuitem.menuitem = menu?.add(group, menuitem.id, 0, menuitem.title)
                        menuitem.menuitem!!.isCheckable = menuitem.check
                        menuitem.menuitem!!.isChecked = menuitem.checked
                        menuitem.menuitem!!.isEnabled = menuitem.enabled
                        menuitem.menuitem!!.setOnMenuItemClickListener(MenuItem.OnMenuItemClickListener { item: MenuItem? ->
                            eventHandlerSimple(menuitem, 8)
                            true
                        })
                    }
                }
            }
        }
    }

    external fun eventHandlerSimple(item: DWMenuItem, message: Int)
}

// Class for storing container data
class DWContainerModel {
    var columns = mutableListOf<String?>()
    var types = mutableListOf<Int>()
    var data = mutableListOf<Any?>()
    var rowdata = mutableListOf<Long>()
    var rowtitle = mutableListOf<String?>()
    var querypos: Int = -1

    fun numberOfColumns(): Int
    {
        return columns.size
    }

    fun numberOfRows(): Int
    {
        if(columns.size > 0) {
            return data.size / columns.size
        }
        return 0
    }

    fun getColumnType(column: Int): Int
    {
        if(column < types.size) {
            return types[column]
        }
        return -1
    }

    fun getRowAndColumn(row: Int, column: Int): Any?
    {
        var index: Int = (row * columns.size) + column

        if(index > -1 && index < data.size) {
            return data[index]
        }
        return null
    }

    fun setRowAndColumn(row: Int, column: Int, obj: Any?)
    {
        var index: Int = (row * columns.size) + column

        if(index > -1 && index < data.size && column < types.size) {
            // Verify the data matches the column type
            if((((types[column] and 1) != 0) && (obj is Drawable)) ||
                (((types[column] and (1 shl 1)) != 0) && (obj is String)) ||
                (((types[column] and (1 shl 2)) != 0) && (obj is Int))) {
                data[index] = obj
            }
        }
    }

    fun changeRowData(row: Int, rdata: Long)
    {
        if(row > -1 && row < rowdata.size) {
            rowdata[row] = rdata
        }
    }

    fun getRowData(row: Int): Long
    {
        if(row > -1 && row < rowdata.size) {
            return rowdata[row]
        }
        return 0
    }

    fun changeRowTitle(row: Int, title: String?)
    {
        if(row > -1 && row < rowtitle.size) {
            rowtitle[row] = title
        }
    }

    fun getRowTitle(row: Int): String?
    {
        if(row > -1 && row < rowtitle.size) {
            return rowtitle[row]
        }
        return null
    }

    fun addColumn(title: String?, type: Int)
    {
        columns.add(title)
        types.add(type)
        // If we change the columns we have to invalidate the data
        data.clear()
    }

    fun deleteRows(count: Int)
    {
        if(count < rowdata.size) {
            for(i in 0 until count) {
                for(j in 0 until columns.size) {
                    data.removeAt(0)
                }
                rowdata.removeAt(0)
                rowtitle.removeAt(0)
            }
        } else {
            data.clear()
            rowdata.clear()
            rowtitle.clear()
        }
    }

    fun deleteRowByTitle(title: String?)
    {
        for(i in 0 until rowtitle.size) {
            if(rowtitle[i] != null && rowtitle[i] == title) {
                for(j in 0 until columns.size) {
                    data.removeAt(i * columns.size)
                }
                rowdata.removeAt(i)
                rowtitle.removeAt(i)
            }
        }
    }

    fun deleteRowByData(rdata: Long)
    {
        for(i in 0 until rowdata.size) {
            if(rowdata[i] == rdata) {
                for(j in 0 until columns.size) {
                    data.removeAt(i * columns.size)
                }
                rowdata.removeAt(i)
                rowtitle.removeAt(i)
            }
        }
    }

    fun addRows(count: Int): Long
    {
        var startRow: Long = numberOfRows().toLong()

        for(i in 0 until count)
        {
            for(j in 0 until columns.size)
            {
                // Fill in with nulls to be set later
                data.add(null)
            }
            rowdata.add(0)
            rowtitle.add(null)
        }
        return startRow
    }

    fun clear()
    {
        data.clear()
        rowdata.clear()
        rowtitle.clear()
    }
}

class DWContainerAdapter(c: Context) : BaseAdapter()
{
    private var context = c
    var model = DWContainerModel()
    var selectedItem: Int = -1
    var simpleMode: Boolean = true

    override fun getCount(): Int {
        return model.numberOfRows()
    }

    override fun getItem(position: Int): Any? {
        return model.getRowAndColumn(position, 0)
    }

    override fun getItemId(position: Int): Long {
        return position.toLong()
    }

    override fun getView(position: Int, view: View?, parent: ViewGroup): View {
        var rowView: LinearLayout? = view as LinearLayout?
        var displayColumns = model.numberOfColumns()

        // In simple mode, limit the columns to 1 or 2
        if(simpleMode == true) {
            // If column 1 is bitmap and column 2 is text...
            if(displayColumns > 1 && (model.getColumnType(0) and 1) != 0 &&
                (model.getColumnType(1) and (1 shl 1)) != 0) {
                displayColumns = 2
            } else {
                displayColumns = 1
            }
        }

        // If the view passed in is null we need to create the layout
        if(rowView == null) {
            rowView = LinearLayout(context)
            var lastView: View? = null

            rowView.orientation = LinearLayout.HORIZONTAL

            for(i in 0 until displayColumns) {
                val content = model.getRowAndColumn(position, i)

                // Image
                if((model.getColumnType(i) and 1) != 0) {
                    val imageview = ImageView(context)
                    imageview.id = View.generateViewId()
                    imageview.layoutParams = LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                                                                       LinearLayout.LayoutParams.WRAP_CONTENT)
                    if (content is Drawable) {
                        imageview.setImageDrawable(content)
                    }
                    rowView.addView(imageview)
                    lastView = imageview
                } else  {
                    // Everything else id displayed as text
                    val textview = TextView(context)
                    textview.id = View.generateViewId()
                    textview.layoutParams = LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                                                      LinearLayout.LayoutParams.WRAP_CONTENT)
                    if (content is String) {
                        textview.text = content
                    } else if(content is Int) {
                        textview.text = content.toString()
                    }
                    rowView.addView(textview)
                    lastView = textview
                }
            }
            // TODO: Add code to optionally add other columns
        } else {
            // Otherwise we just need to update the existing layout

            for(i in 0 until displayColumns) {
                val content = model.getRowAndColumn(position, i)

                // Image
                if((model.getColumnType(i) and 1) != 0) {
                    val imageview = rowView.getChildAt(i)

                    if (imageview is ImageView && content is Drawable) {
                        imageview.setImageDrawable(content)
                    }
                } else {
                    // Text
                    val textview = rowView.getChildAt(i)

                    if (textview is TextView) {
                        if (content is String) {
                            textview.text = content
                        } else if (content is Int) {
                            textview.text = content.toString()
                        }
                    }
                }
            }
        }
        return rowView
    }
}

class DWindows : AppCompatActivity() {
    var firstWindow: Boolean = true
    var windowLayout: LinearLayout? = null
    var threadLock = ReentrantLock()
    var threadCond = threadLock.newCondition()
    var notificationID: Int = 0
    var darkMode: Int = -1
    private var paint = Paint()
    private var bgcolor: Int = 0
    private var menuBar: DWMenu? = null

    // Our version of runOnUiThread that waits for execution
    fun waitOnUiThread(runnable: Runnable)
    {
          if(Looper.myLooper() == Looper.getMainLooper()) {
              runnable.run()
          } else {
              threadLock.lock()
              val ourRunnable = Runnable {
                  threadLock.lock()
                  runnable.run()
                  threadCond.signal()
                  threadLock.unlock()
              }
              runOnUiThread(ourRunnable)
              threadCond.await()
              threadLock.unlock()
          }
    }

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
        dwindowsInit(s, this.getPackageName())
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)

        val currentNightMode = newConfig.uiMode and Configuration.UI_MODE_NIGHT_MASK
        when (currentNightMode) {
            Configuration.UI_MODE_NIGHT_NO -> { darkMode = 0 } // Night mode is not active, we're using the light theme
            Configuration.UI_MODE_NIGHT_YES -> { darkMode = 1 } // Night mode is active, we're using dark theme
        }

        // Send a DW_SIGNAL_CONFIGURE on orientation change
        if(windowLayout != null) {
            var width: Int = windowLayout!!.width
            var height: Int = windowLayout!!.height

            eventHandlerInt(windowLayout as View, 1, width, height, 0, 0)
        }
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        if(menuBar == null) {
            menuBar = DWMenu()
            menuBar!!.menu = menu
        }
        return super.onCreateOptionsMenu(menu)
    }

    override fun onPrepareOptionsMenu(menu: Menu?): Boolean {
        if(menuBar != null) {
            menuBar!!.createMenu(menu)
        } else {
            menuBar = DWMenu()
            menuBar!!.createMenu(menu)
        }
        return super.onPrepareOptionsMenu(menu)
    }

    // These are the Android calls to actually create the UI...
    // forwarded from the C Dynamic Windows API
    
    fun darkModeDetected(): Int
    {
        return darkMode
    }

    fun menuBarNew(location: View): DWMenu?
    {
        // TODO: Make sure location is this activity
        return menuBar
    }

    fun menuNew(cid: Int): DWMenu
    {
        val menu = DWMenu()
        menu.id = cid
        return menu
    }

    fun menuAppendItem(menu: DWMenu, title: String, cid: Int, flags: Int, end: Int, check: Int, submenu: DWMenu?): DWMenuItem
    {
        val menuitem = DWMenuItem()
        menuitem.id = cid
        menuitem.title = title
        menuitem.check = check != 0
        if(submenu != null) {
            menuitem.submenu = submenu
        }
        if((flags and (1 shl 1)) != 0) {
            menuitem.enabled = false
        }
        if((flags and (1 shl 2)) != 0) {
            menuitem.checked = true
        }
        if(end == 0) {
            menu.children.add(0, menuitem)
        } else {
            menu.children.add(menuitem)
        }
        return menuitem
    }

    fun menuDestroy(menu: DWMenu)
    {
        menu.children.clear()
        runOnUiThread {
            menu.menu!!.clear()
            invalidateOptionsMenu()
        }
    }

    fun menuDeleteItem(menu: DWMenu, cid: Int)
    {
        for(menuitem in menu.children) {
            if(menuitem.id == cid) {
                menu.children.remove(menuitem)
                runOnUiThread {
                    menu.menu!!.removeItem(menuitem.id)
                    invalidateOptionsMenu()
                }
            }
        }
    }

    fun menuSetState(menu: DWMenu, cid: Int, state: Int)
    {
        for(menuitem in menu.children) {
            if(menuitem.id == cid) {
                // Handle DW_MIS_ENABLED/DISABLED
                if((state and (1 or (1 shl 1))) != 0) {
                    var enabled = false

                    // Handle DW_MIS_ENABLED
                    if ((state and 1) != 0) {
                        enabled = true
                    }
                    menuitem.enabled = enabled
                    runOnUiThread {
                        menuitem.menuitem!!.isEnabled = enabled
                        invalidateOptionsMenu()
                    }
                }

                // Handle DW_MIS_CHECKED/UNCHECKED
                if((state and ((1 shl 2) or (1 shl 3))) != 0) {
                    var checked = false

                    // Handle DW_MIS_CHECKED
                    if ((state and (1 shl 2)) != 0) {
                        checked = true
                    }
                    menuitem.checked = checked
                    runOnUiThread {
                        menuitem.menuitem!!.isChecked = checked
                        invalidateOptionsMenu()
                    }
                }
            }
        }
    }

    fun windowNew(title: String, style: Int): LinearLayout? {
        if (firstWindow) {
            waitOnUiThread {
                var dataArrayMap = SimpleArrayMap<String, Long>()
                windowLayout = LinearLayout(this)

                windowLayout!!.visibility = View.GONE
                windowLayout!!.tag = dataArrayMap
                setContentView(windowLayout)
                this.title = title
                // For now we just return our DWindows' main activity layout...
                // in the future, later calls should create new activities
                firstWindow = false
            }
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

            if(dataArrayMap.containsKey(name)) {
                retval = dataArrayMap.get(name)!!
            }
        }
        return retval
    }

    fun windowSetEnabled(window: View, state: Boolean) {
        waitOnUiThread {
            window.setEnabled(state)
        }
    }

    fun typefaceFromFontName(fontname: String?): Typeface?
    {
        if(fontname != null) {
            val bold: Boolean = fontname.contains(" Bold")
            val italic: Boolean = fontname.contains(" Italic")
            val font = fontname.substringAfter('.')
            var fontFamily = font
            var typeface: Typeface? = null

            if (bold && font != null) {
                fontFamily = font.substringBefore(" Bold")
            } else if (italic && font != null) {
                fontFamily = font.substringBefore(" Italic")
            }

            if (fontFamily != null) {
                var style: Int = Typeface.NORMAL
                if (bold && italic) {
                    style = Typeface.BOLD_ITALIC
                } else if (bold) {
                    style = Typeface.BOLD
                } else if (italic) {
                    style = Typeface.ITALIC
                }
                typeface = Typeface.create(fontFamily, style)
            }
            return typeface
        }
        return Typeface.DEFAULT
    }

    fun windowSetFont(window: View, fontname: String?) {
        var typeface: Typeface? = typefaceFromFontName(fontname)
        var size: Float? = null

        if(fontname != null) {
            size = fontname.substringBefore('.').toFloatOrNull()
        }

        if(typeface != null) {
            waitOnUiThread {
                if (window is TextView) {
                    var textview: TextView = window
                    textview.typeface = typeface
                    if(size != null) {
                        textview.textSize = size
                    }
                } else if (window is Button) {
                    var button: Button = window
                    button.typeface = typeface
                    if(size != null) {
                        button.textSize = size
                    }
                } else if(window is DWRender) {
                    var render: DWRender = window
                    render.typeface = typeface
                    if(size != null) {
                        render.fontsize = size
                    }
                }
            }
        }
    }

    fun windowSetColor(window: View, fore: Int, falpha: Int, fred: Int, fgreen: Int, fblue: Int,
                       back: Int, balpha: Int, bred: Int, bgreen: Int, bblue: Int) {
        var colorfore: Int = Color.rgb(fred, fgreen, fblue)
        var colorback: Int = Color.rgb(bred, bgreen, bblue)

        // DW_CLR_DEFAULT on background sets it transparent...
        // so the background drawable shows through
        if(back == 16) {
            colorback = Color.TRANSPARENT
        }

        waitOnUiThread {
            if (window is TextView) {
                var textview: TextView = window

                // Handle DW_CLR_DEFAULT
                if(fore == 16) {
                    val value = TypedValue()
                    this.theme.resolveAttribute(R.attr.editTextColor, value, true)
                    colorfore = value.data
                }
                textview.setTextColor(colorfore)
                textview.setBackgroundColor(colorback)
            } else if (window is Button) {
                var button: Button = window

                // Handle DW_CLR_DEFAULT
                if(fore == 16) {
                    val value = TypedValue()
                    // colorButtonNormal requires API 21... use the editTextColor...
                    // on older versions as a placeholder... this is probably wrong
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                        this.theme.resolveAttribute(R.attr.colorButtonNormal, value, true)
                    } else {
                        this.theme.resolveAttribute(R.attr.editTextColor, value, true)
                    }
                    colorfore = value.data
                }
                button.setTextColor(colorfore)
                button.setBackgroundColor(colorback)
            } else if(window is LinearLayout) {
                var box: LinearLayout = window

                box.setBackgroundColor(colorback)
            }
        }
    }

    fun windowSetText(window: View, text: String) {
        waitOnUiThread {
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
    }

    fun windowGetText(window: View): String? {
        var text: String? = null

        waitOnUiThread {
            if (window is TextView) {
                var textview: TextView = window
                text = textview.text.toString()
            } else if (window is Button) {
                var button: Button = window
                text = button.text.toString()
            } else if (window is LinearLayout) {
                // TODO: Make sure this is actually the top-level layout, not just a box
                text = this.title.toString()
            }
        }
        return text
    }

    fun windowHideShow(window: View, state: Int)
    {
        waitOnUiThread {
            if(state == 0) {
                window.visibility = View.GONE
            } else {
                window.visibility = View.VISIBLE
            }
        }
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

    fun boxNew(type: Int, pad: Int): LinearLayout? {
        var box: LinearLayout? = null
        waitOnUiThread {
            box = LinearLayout(this)
            var dataArrayMap = SimpleArrayMap<String, Long>()

            box!!.tag = dataArrayMap
            box!!.layoutParams =
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT
                )
            if (type > 0) {
                box!!.orientation = LinearLayout.VERTICAL
            } else {
                box!!.orientation = LinearLayout.HORIZONTAL
            }
            box!!.setPadding(pad, pad, pad, pad)
        }
        return box
    }

    fun scrollBoxNew(type: Int, pad: Int) : ScrollView? {
        var scrollBox: ScrollView? = null

        waitOnUiThread {
            val box = LinearLayout(this)
            scrollBox = ScrollView(this)
            var dataArrayMap = SimpleArrayMap<String, Long>()

            scrollBox!!.tag = dataArrayMap
            box.layoutParams =
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.MATCH_PARENT
                )
            if (type > 0) {
                box.orientation = LinearLayout.VERTICAL
            } else {
                box.orientation = LinearLayout.HORIZONTAL
            }
            box.setPadding(pad, pad, pad, pad)
            // Add a pointer back to the ScrollView
            box.tag = scrollBox
            scrollBox!!.addView(box)
        }
        return scrollBox
    }

    fun boxPack(
        boxview: View,
        item: View,
        index: Int,
        width: Int,
        height: Int,
        hsize: Int,
        vsize: Int,
        pad: Int
    ) {
        waitOnUiThread {
            var w: Int = LinearLayout.LayoutParams.WRAP_CONTENT
            var h: Int = LinearLayout.LayoutParams.WRAP_CONTENT
            var box: LinearLayout? = null

            // Handle scrollboxes by pulling the LinearLayout
            // out of the ScrollView to pack into
            if (boxview is LinearLayout) {
                box = boxview
            } else if (boxview is ScrollView) {
                var sv: ScrollView = boxview

                if (sv.getChildAt(0) is LinearLayout) {
                    box = sv.getChildAt(0) as LinearLayout
                }
            }

            if (box != null) {
                var weight: Float = 1F

                // If it is a box, match parent based on direction
                if ((item is LinearLayout) || (item is ScrollView)) {
                    if (box.orientation == LinearLayout.VERTICAL) {
                        if (hsize != 0) {
                            w = LinearLayout.LayoutParams.MATCH_PARENT
                        }
                    } else {
                        if (vsize != 0) {
                            h = LinearLayout.LayoutParams.MATCH_PARENT
                        }
                    }
                // If it isn't a box... set or calculate the size as needed
                } else {
                    if(width != -1 || height != -1) {
                        item.measure(0, 0)
                    }
                    if(hsize == 0) {
                        if (width > 0) {
                            w = width
                        }
                    } else {
                        if (width > 0) {
                            weight = width.toFloat()
                        } else if (width == -1) {
                            val newwidth = item.getMeasuredWidth()

                            if(newwidth > 0) {
                                weight = newwidth.toFloat()
                            }
                        }
                    }
                    if(vsize == 0) {
                        if (height > 0) {
                            h = height
                        }
                    } else {
                        if (height > 0) {
                            weight = height.toFloat()
                        } else if (height == -1) {
                            val newheight = item.getMeasuredHeight()

                            if(newheight > 0) {
                                weight = newheight.toFloat()
                            }
                        }
                    }
                }

                var params: LinearLayout.LayoutParams = LinearLayout.LayoutParams(w, h)

                // Handle expandable items by giving them a weight...
                // in the direction of the box.
                if (box.orientation == LinearLayout.VERTICAL) {
                    if (vsize != 0) {
                        params.weight = weight
                    }
                } else {
                    if (hsize != 0) {
                        params.weight = weight
                    }
                }
                // Gravity needs to match the expandable settings
                var grav: Int = Gravity.CLIP_HORIZONTAL or Gravity.CLIP_VERTICAL
                if (hsize != 0 && vsize != 0) {
                    params.gravity = Gravity.FILL or grav
                } else if (hsize != 0) {
                    params.gravity = Gravity.FILL_HORIZONTAL or grav
                } else if (vsize != 0) {
                    params.gravity = Gravity.FILL_VERTICAL or grav
                }
                // Finally add the padding
                if (pad > 0) {
                    params.setMargins(pad, pad, pad, pad)
                }
                item.layoutParams = params
                box.addView(item, index)
            }
        }
    }

    fun boxUnpack(item: View) {
        waitOnUiThread {
            var box: LinearLayout = item.parent as LinearLayout
            box.removeView(item)
        }
    }

    fun boxUnpackAtIndex(box: LinearLayout, index: Int): View? {
        var item: View? = null

        waitOnUiThread {
            item = box.getChildAt(index)

            box.removeView(item)
        }
        return item
    }

    fun buttonNew(text: String, cid: Int): Button? {
        var button: Button? = null
        waitOnUiThread {
            button = Button(this)
            var dataArrayMap = SimpleArrayMap<String, Long>()

            button!!.tag = dataArrayMap
            button!!.text = text
            button!!.id = cid
            button!!.setOnClickListener {
                eventHandlerSimple(button!!, 8)
            }
        }
        return button
    }

    fun bitmapButtonNew(text: String, resid: Int): ImageButton? {
        var button: ImageButton? = null
        waitOnUiThread {
            button = ImageButton(this)
            var dataArrayMap = SimpleArrayMap<String, Long>()

            button!!.tag = dataArrayMap
            button!!.id = resid
            button!!.setImageResource(resid)
            button!!.setOnClickListener {
                eventHandlerSimple(button!!, 8)
            }
        }
        return button
    }

    fun bitmapButtonNewFromFile(text: String, cid: Int, filename: String): ImageButton? {
        var button: ImageButton? = null
        waitOnUiThread {
            button = ImageButton(this)
            var dataArrayMap = SimpleArrayMap<String, Long>()
            var exts = arrayOf("", ".png", ".webp", ".jpg", ".jpeg", ".gif")

            button!!.tag = dataArrayMap
            button!!.id = cid
            button!!.setOnClickListener {
                eventHandlerSimple(button!!, 8)
            }

            for (ext in exts) {
                // Try to load the image, and protect against exceptions
                try {
                    val f = this.assets.open(filename + ext)
                    val b = BitmapFactory.decodeStream(f)

                    if(b != null) {
                        button!!.setImageBitmap(b)
                        break
                    }
                } catch (e: IOException) {
                }
            }
        }
        return button
    }

    fun bitmapButtonNewFromData(text: String, cid: Int, data: ByteArray, length: Int): ImageButton? {
        var button: ImageButton? = null
        waitOnUiThread {
            button = ImageButton(this)
            var dataArrayMap = SimpleArrayMap<String, Long>()
            val b = BitmapFactory.decodeByteArray(data,0, length)

            button!!.tag = dataArrayMap
            button!!.id = cid
            button!!.setOnClickListener {
                eventHandlerSimple(button!!, 8)
            }
            button!!.setImageBitmap(b)
        }
        return button
    }

    fun entryfieldNew(text: String, cid: Int, password: Int): EditText? {
        var entryfield: EditText? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()
            entryfield = EditText(this)

            entryfield!!.tag = dataArrayMap
            entryfield!!.id = cid
            entryfield!!.isSingleLine = true
            if (password > 0) {
                entryfield!!.transformationMethod = PasswordTransformationMethod.getInstance()
            }
            entryfield!!.setText(text)
        }
        return entryfield
    }

    fun entryfieldSetLimit(entryfield: EditText, limit: Long) {
        waitOnUiThread {
            entryfield.filters = arrayOf<InputFilter>(LengthFilter(limit.toInt()))
        }
    }

    fun radioButtonNew(text: String, cid: Int): RadioButton? {
        var radiobutton: RadioButton? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()
            radiobutton = RadioButton(this)

            radiobutton!!.tag = dataArrayMap
            radiobutton!!.id = cid
            radiobutton!!.text = text
            radiobutton!!.setOnClickListener {
                eventHandlerSimple(radiobutton!!, 8)
            }
        }
        return radiobutton
    }

    fun checkboxNew(text: String, cid: Int): CheckBox? {
        var checkbox: CheckBox? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            checkbox = CheckBox(this)
            checkbox!!.tag = dataArrayMap
            checkbox!!.id = cid
            checkbox!!.text = text
            checkbox!!.setOnClickListener {
                eventHandlerSimple(checkbox!!, 8)
            }
        }
        return checkbox
    }

    fun checkOrRadioSetChecked(control: View, state: Int)
    {
        waitOnUiThread {
            if (control is CheckBox) {
                var checkbox: CheckBox = control
                checkbox.isChecked = state != 0
            } else if (control is RadioButton) {
                var radiobutton: RadioButton = control
                radiobutton.isChecked = state != 0
            }
        }
    }

    fun checkOrRadioGetChecked(control: View): Boolean
    {
        var retval: Boolean = false

        waitOnUiThread {
            if (control is CheckBox) {
                var checkbox: CheckBox = control
                retval = checkbox.isChecked
            } else if (control is RadioButton) {
                var radiobutton: RadioButton = control
                retval = radiobutton.isChecked
            }
        }
        return retval
    }

    fun textNew(text: String, cid: Int, status: Int): TextView? {
        var textview: TextView? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            textview = TextView(this)
            textview!!.tag = dataArrayMap
            textview!!.id = cid
            textview!!.text = text
            if (status != 0) {
                val border = GradientDrawable()

                // Set a black border on white background...
                // might need to change this to invisible...
                // or the color from windowSetColor
                border.setColor(-0x1)
                border.setStroke(1, -0x1000000)
                textview!!.background = border
            }
        }
        return textview
    }

    fun mleNew(cid: Int): EditText?
    {
        var mle: EditText? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            mle = EditText(this)
            mle!!.tag = dataArrayMap
            mle!!.id = cid
            mle!!.isSingleLine = false
            mle!!.imeOptions = EditorInfo.IME_FLAG_NO_ENTER_ACTION
            mle!!.inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_MULTI_LINE)
            mle!!.isVerticalScrollBarEnabled = true
            mle!!.scrollBarStyle = View.SCROLLBARS_INSIDE_INSET
            mle!!.setHorizontallyScrolling(true)
        }
        return mle
    }

    fun mleSetWordWrap(mle: EditText, state: Int)
    {
        waitOnUiThread {
            if (state != 0) {
                mle.setHorizontallyScrolling(false)
            } else {
                mle.setHorizontallyScrolling(true)
            }
        }
    }

    fun mleSetEditable(mle: EditText, state: Int)
    {
        waitOnUiThread {
            if (state != 0) {
                mle.inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_MULTI_LINE)
            } else {
                mle.inputType = InputType.TYPE_NULL
            }
        }
    }

    fun mleSetCursor(mle: EditText, point: Int)
    {
        waitOnUiThread {
            mle.setSelection(point)
        }
    }

    fun mleClear(mle: EditText)
    {
        waitOnUiThread {
            mle.setText("")
        }
    }

    fun mleImport(mle: EditText, text: String, startpoint: Int): Int
    {
        var retval: Int = startpoint

        waitOnUiThread {
            val origtext = mle.text
            val origlen = origtext.toString().length

            if(startpoint < 1) {
                val newtext = text + origtext.toString()

                mle.setText(newtext)
                retval = origlen + text.length
            } else if(startpoint >= origlen) {
                val newtext = origtext.toString() + text

                mle.setText(newtext)
                retval = origlen + text.length
            } else {
                val newtext = origtext.substring(0, startpoint) + text + origtext.substring(startpoint)

                mle.setText(newtext)
                retval = startpoint + text.length
            }
            mle.setSelection(retval)
        }
        return retval
    }

    fun mleDelete(mle: EditText, startpoint: Int, length: Int)
    {
        waitOnUiThread {
            val origtext = mle.text
            val newtext = origtext.substring(0, startpoint) + origtext.substring(startpoint + length)

            mle.setText(newtext)
        }
    }

    fun notebookNew(cid: Int, top: Int): RelativeLayout?
    {
        var notebook: RelativeLayout? = null

        waitOnUiThread {
            val pager = ViewPager2(this)
            val tabs = TabLayout(this)
            var w: Int = RelativeLayout.LayoutParams.MATCH_PARENT
            var h: Int = RelativeLayout.LayoutParams.WRAP_CONTENT
            var dataArrayMap = SimpleArrayMap<String, Long>()

            notebook = RelativeLayout(this)
            notebook!!.tag = dataArrayMap
            notebook!!.id = cid
            tabs.id = View.generateViewId()
            pager.id = View.generateViewId()
            pager.adapter = DWTabViewPagerAdapter()
            TabLayoutMediator(tabs, pager) { tab, position ->
                // This code never gets called?
            }.attach()

            var params: RelativeLayout.LayoutParams = RelativeLayout.LayoutParams(w, h)
            if (top != 0) {
                params.addRule(RelativeLayout.ALIGN_PARENT_TOP)
            } else {
                params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM)
            }
            tabs.tabGravity = TabLayout.GRAVITY_FILL
            tabs.tabMode = TabLayout.MODE_FIXED
            notebook!!.addView(tabs, params)
            params = RelativeLayout.LayoutParams(w, w)
            if (top != 0) {
                params.addRule(RelativeLayout.BELOW, tabs.id)
            } else {
                params.addRule(RelativeLayout.ABOVE, tabs.id)
            }
            notebook!!.addView(pager, params)
            // TODO: Not sure if we want this all the time...
            // Might want to make a flag for this
            pager.isUserInputEnabled = false
            tabs.addOnTabSelectedListener(object : OnTabSelectedListener {
                override fun onTabSelected(tab: TabLayout.Tab) {
                    val adapter = pager.adapter as DWTabViewPagerAdapter

                    pager.currentItem = tab.position
                    eventHandlerNotebook(notebook!!, 15, adapter.pageList[tab.position])
                }

                override fun onTabUnselected(tab: TabLayout.Tab) {}
                override fun onTabReselected(tab: TabLayout.Tab) {}
            })
        }
        return notebook
    }

    fun notebookPageNew(notebook: RelativeLayout, flags: Long, front: Int): Long
    {
        var pageID = 0L

        waitOnUiThread {
            var pager: ViewPager2? = null
            var tabs: TabLayout? = null

            if (notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
                pager = notebook.getChildAt(0) as ViewPager2
                tabs = notebook.getChildAt(1) as TabLayout
            } else if (notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
                pager = notebook.getChildAt(1) as ViewPager2
                tabs = notebook.getChildAt(0) as TabLayout
            }

            if (pager != null && tabs != null) {
                var adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
                var tab = tabs.newTab()

                // Increment our page ID... making sure no duplicates exist
                do {
                    adapter.currentPageID += 1
                } while (adapter.currentPageID == 0L || adapter.pageList.contains(adapter.currentPageID))
                pageID = adapter.currentPageID
                // Temporarily add a black tab with an empty layout/box
                if (front != 0) {
                    adapter.viewList.add(0, LinearLayout(this))
                    adapter.pageList.add(0, pageID)
                    tabs.addTab(tab, 0)
                } else {
                    adapter.viewList.add(LinearLayout(this))
                    adapter.pageList.add(pageID)
                    tabs.addTab(tab)
                }
            }
        }
        return pageID
    }

    fun notebookPageDestroy(notebook: RelativeLayout, pageID: Long)
    {
        waitOnUiThread {
            var pager: ViewPager2? = null
            var tabs: TabLayout? = null

            if (notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
                pager = notebook.getChildAt(0) as ViewPager2
                tabs = notebook.getChildAt(1) as TabLayout
            } else if (notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
                pager = notebook.getChildAt(1) as ViewPager2
                tabs = notebook.getChildAt(0) as TabLayout
            }

            if (pager != null && tabs != null) {
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
    }

    fun notebookPageSetText(notebook: RelativeLayout, pageID: Long, text: String)
    {
        waitOnUiThread {
            var pager: ViewPager2? = null
            var tabs: TabLayout? = null

            if (notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
                pager = notebook.getChildAt(0) as ViewPager2
                tabs = notebook.getChildAt(1) as TabLayout
            } else if (notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
                pager = notebook.getChildAt(1) as ViewPager2
                tabs = notebook.getChildAt(0) as TabLayout
            }

            if (pager != null && tabs != null) {
                val adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
                val index = adapter.pageList.indexOf(pageID)
                val tab = tabs.getTabAt(index)

                if (tab != null) {
                    tab.text = text
                }
            }
        }
    }

    fun notebookPagePack(notebook: RelativeLayout, pageID: Long, box: LinearLayout)
    {
        waitOnUiThread {
            var pager: ViewPager2? = null
            var tabs: TabLayout? = null

            if (notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
                pager = notebook.getChildAt(0) as ViewPager2
                tabs = notebook.getChildAt(1) as TabLayout
            } else if (notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
                pager = notebook.getChildAt(1) as ViewPager2
                tabs = notebook.getChildAt(0) as TabLayout
            }

            if (pager != null && tabs != null) {
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
    }

    fun notebookPageGet(notebook: RelativeLayout): Long
    {
        var retval: Long = 0L

        waitOnUiThread {
            var pager: ViewPager2? = null
            var tabs: TabLayout? = null

            if (notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
                pager = notebook.getChildAt(0) as ViewPager2
                tabs = notebook.getChildAt(1) as TabLayout
            } else if (notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
                pager = notebook.getChildAt(1) as ViewPager2
                tabs = notebook.getChildAt(0) as TabLayout
            }

            if (pager != null && tabs != null) {
                var adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
                retval = adapter.pageList.get(tabs.selectedTabPosition)
            }
        }
        return retval
    }

    fun notebookPageSet(notebook: RelativeLayout, pageID: Long)
    {
        waitOnUiThread {
            var pager: ViewPager2? = null
            var tabs: TabLayout? = null

            if (notebook.getChildAt(0) is ViewPager2 && notebook.getChildAt(1) is TabLayout) {
                pager = notebook.getChildAt(0) as ViewPager2
                tabs = notebook.getChildAt(1) as TabLayout
            } else if (notebook.getChildAt(1) is ViewPager2 && notebook.getChildAt(0) is TabLayout) {
                pager = notebook.getChildAt(1) as ViewPager2
                tabs = notebook.getChildAt(0) as TabLayout
            }

            if (pager != null && tabs != null) {
                val adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
                val index = adapter.pageList.indexOf(pageID)
                val tab = tabs.getTabAt(index)

                tabs.selectTab(tab)
            }
        }
    }

    fun sliderNew(vertical: Int, increments: Int, cid: Int): SeekBar?
    {
        var slider: SeekBar? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            slider = SeekBar(this)
            slider!!.tag = dataArrayMap
            slider!!.id = cid
            slider!!.max = increments
            if (vertical != 0) {
                slider!!.rotation = 270F
            }
            slider!!.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
                override fun onStopTrackingTouch(seekBar: SeekBar) {
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {
                }

                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    eventHandlerInt(slider as View, 14, slider!!.progress, 0, 0, 0)
                }
            })
        }
        return slider
    }

    fun percentNew(cid: Int): ProgressBar?
    {
        var percent: ProgressBar? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            percent = ProgressBar(this)
            percent!!.tag = dataArrayMap
            percent!!.id = cid
            percent!!.max = 100
        }
        return percent
    }

    fun percentGetPos(percent: ProgressBar): Int
    {
        var retval: Int = 0

        waitOnUiThread {
            retval = percent.progress
        }
        return retval
    }

    fun percentSetPos(percent: ProgressBar, position: Int)
    {
        waitOnUiThread {
            percent.progress = position
        }
    }

    fun percentSetRange(percent: ProgressBar, range: Int)
    {
        waitOnUiThread {
            percent.max = range
        }
    }

    fun htmlNew(cid: Int): WebView?
    {
        var html: WebView? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            html = WebView(this)
            html!!.tag = dataArrayMap
            html!!.id = cid
            // Configure a few settings to make it behave as we expect
            html!!.webViewClient = DWWebViewClient()
            html!!.settings.javaScriptEnabled = true
        }
        return html
    }

    fun htmlLoadURL(html: WebView, url: String)
    {
        waitOnUiThread {
            html.loadUrl(url)
        }
    }

    fun htmlRaw(html: WebView, data: String)
    {
        waitOnUiThread {
            val encodedHtml: String = Base64.encodeToString(data.toByteArray(), Base64.NO_PADDING)
            html.loadData(encodedHtml, "text/html", "base64")
        }
    }

    fun htmlJavascriptRun(html: WebView, javascript: String, data: Long)
    {
        waitOnUiThread {
            html.evaluateJavascript(javascript) { value ->
                // Execute onReceiveValue's code
                eventHandlerHTMLResult(html, 18, value, data)
            }
        }
    }

    fun htmlAction(html: WebView, action: Int)
    {
        waitOnUiThread {
            when (action) {
                0 -> html.goBack()
                1 -> html.goForward()
                2 -> html.loadUrl("http://dwindows.netlabs.org")
                4 -> html.reload()
                5 -> html.stopLoading()
            }
        }
    }

    fun spinButtonNew(text: String, cid: Int): DWSpinButton?
    {
        var spinbutton: DWSpinButton? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()
            val newval = text.toLongOrNull()

            spinbutton = DWSpinButton(this)
            spinbutton!!.tag = dataArrayMap
            spinbutton!!.id = cid
            spinbutton!!.setText(text)
            if(newval != null) {
                spinbutton!!.value = newval
            }
        }
        return spinbutton
    }

    fun spinButtonSetPos(spinbutton: DWSpinButton, position: Long)
    {
        waitOnUiThread {
            spinbutton.value = position
            spinbutton.setText(position.toString())
        }
    }

    fun spinButtonSetLimits(spinbutton: DWSpinButton, upper: Long, lower: Long)
    {
        waitOnUiThread {
            spinbutton.maximum = upper
            spinbutton.minimum = lower
            if(spinbutton.value > upper) {
                spinbutton.value = upper
            }
            if(spinbutton.value < lower) {
                spinbutton.value = lower
            }
            spinbutton.setText(spinbutton.value.toString())
        }
    }

    fun spinButtonGetPos(spinbutton: DWSpinButton): Long
    {
        var retval: Long = 0

        waitOnUiThread {
            val newvalue = spinbutton.text.toString().toLongOrNull()

            if(newvalue == null) {
                retval = spinbutton.value
            } else {
                retval = newvalue
            }
        }
        return retval
    }

    fun comboBoxNew(text: String, cid: Int): DWComboBox?
    {
        var combobox: DWComboBox? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            combobox = DWComboBox(this)
            combobox!!.tag = dataArrayMap
            combobox!!.id = cid
            combobox!!.setText(text)
        }
        return combobox
    }

    fun containerNew(cid: Int, multi: Int): ListView?
    {
        var cont: ListView? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()
            var adapter = DWContainerAdapter(this)

            cont = ListView(this)
            cont!!.tag = dataArrayMap
            cont!!.id = cid
            cont!!.adapter = adapter
            if(multi != 0) {
                cont!!.choiceMode = ListView.CHOICE_MODE_MULTIPLE;
            }
            cont!!.setOnItemClickListener { parent, view, position, id ->
                val title = adapter.model.getRowTitle(position)
                val data = adapter.model.getRowData(position)

                view.isSelected = !view.isSelected
                adapter.selectedItem = position
                eventHandlerContainer(cont!!, 12, title, 0, 0, data)
            }
            cont!!.setOnContextClickListener {
                if(adapter.selectedItem > -1 && adapter.selectedItem < adapter.model.numberOfRows()) {
                    val title = adapter.model.getRowTitle(adapter.selectedItem)
                    val data = adapter.model.getRowData(adapter.selectedItem)

                    eventHandlerContainer(cont!!, 10, title, 0, 0, data)
                }
                true
            }
            cont!!.setOnItemLongClickListener { parent, view, position, id ->
                val title = adapter.model.getRowTitle(position)
                val data = adapter.model.getRowData(position)

                eventHandlerContainer(cont!!, 9, title, 0, 0, data)
                true
            }
        }
        return cont
    }

    fun containerGetTitleStart(cont: ListView, flags: Int): String?
    {
        var retval: String? = null

        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            // Handle DW_CRA_SELECTED
            if((flags and 1) != 0) {
                val checked: SparseBooleanArray = cont.getCheckedItemPositions()
                val position = checked.keyAt(0)

                if(position != null) {
                    adapter.model.querypos = position
                    retval = adapter.model.getRowTitle(position)
                } else {
                    adapter.model.querypos = -1
                }
            } else {
                if(adapter.model.rowdata.size == 0) {
                    adapter.model.querypos = -1
                } else {
                    retval = adapter.model.getRowTitle(0)
                    adapter.model.querypos = 0
                }
            }
        }
        return retval
    }

    fun containerGetTitleNext(cont: ListView, flags: Int): String?
    {
        var retval: String? = null

        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            if(adapter.model.querypos != -1) {
                // Handle DW_CRA_SELECTED
                if ((flags and 1) != 0) {
                    val checked: SparseBooleanArray = cont.getCheckedItemPositions()

                    // Otherwise loop until we find our current place
                    for (i in 0 until checked.size()) {
                        // Item position in adapter
                        val position: Int = checked.keyAt(i)

                        if (position != null) {
                            // If we are at our current point... check to see
                            // if there is another one, and return it...
                            // otherwise we will return -1 to indicated we are done.
                            if (adapter.model.querypos == position && (i + 1) < checked.size()) {
                                val newpos = checked.keyAt(i + 1)

                                if (newpos != null) {
                                    adapter.model.querypos = newpos
                                    retval = adapter.model.getRowTitle(newpos)
                                } else {
                                    adapter.model.querypos = -1
                                }
                            }
                        } else {
                            adapter.model.querypos = -1
                        }
                    }
                } else {
                    if (adapter.model.rowtitle.size > adapter.model.querypos) {
                        adapter.model.querypos += 1
                        retval = adapter.model.getRowTitle(adapter.model.querypos)
                    } else {
                        adapter.model.querypos = -1
                    }
                }
            }
        }
        return retval
    }

    fun containerGetDataStart(cont: ListView, flags: Int): Long
    {
        var retval: Long = 0

        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            // Handle DW_CRA_SELECTED
            if((flags and 1) != 0) {
                val checked: SparseBooleanArray = cont.getCheckedItemPositions()
                val position = checked.keyAt(0)

                if(position != null) {
                    adapter.model.querypos = position
                    retval = adapter.model.getRowData(position)
                } else {
                    adapter.model.querypos = -1
                }
            } else {
                if(adapter.model.rowdata.size == 0) {
                    adapter.model.querypos = -1
                } else {
                    retval = adapter.model.getRowData(0)
                    adapter.model.querypos = 0
                }
            }
        }
        return retval
    }

    fun containerGetDataNext(cont: ListView, flags: Int): Long
    {
        var retval: Long = 0

        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            if(adapter.model.querypos != -1) {
                // Handle DW_CRA_SELECTED
                if ((flags and 1) != 0) {
                    val checked: SparseBooleanArray = cont.getCheckedItemPositions()

                    // Otherwise loop until we find our current place
                    for (i in 0 until checked.size()) {
                        // Item position in adapter
                        val position: Int = checked.keyAt(i)

                        if (position != null) {
                            // If we are at our current point... check to see
                            // if there is another one, and return it...
                            // otherwise we will return -1 to indicated we are done.
                            if (adapter.model.querypos == position && (i + 1) < checked.size()) {
                                val newpos = checked.keyAt(i + 1)

                                if (newpos != null) {
                                    adapter.model.querypos = newpos
                                    retval = adapter.model.getRowData(newpos)
                                } else {
                                    adapter.model.querypos = -1
                                }
                            }
                        } else {
                            adapter.model.querypos = -1
                        }
                    }
                } else {
                    if (adapter.model.rowdata.size > adapter.model.querypos) {
                        adapter.model.querypos += 1
                        retval = adapter.model.getRowData(adapter.model.querypos)
                    } else {
                        adapter.model.querypos = -1
                    }
                }
            }
        }
        return retval
    }

    fun containerAddColumn(cont: ListView, title: String, flags: Int)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.addColumn(title, flags)
        }
    }

    fun containerAlloc(cont: ListView, rowcount: Int): ListView
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter
            val rowStart = adapter.model.addRows(rowcount)

            windowSetData(cont, "_dw_rowstart", rowStart)
        }
        return cont
    }

    fun containerChangeItemString(cont: ListView, column: Int, row: Int, text: String)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.setRowAndColumn(row, column, text)
        }
    }

    fun containerChangeItemIcon(cont: ListView, column: Int, row: Int, icon: Drawable)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.setRowAndColumn(row, column, icon)
        }
    }

    fun containerChangeItemInt(cont: ListView, column: Int, row: Int, num: Int)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.setRowAndColumn(row, column, num)
        }
    }

    fun containerChangeRowData(cont: ListView, row: Int, data: Long)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.changeRowData(row, data)
        }
    }

    fun containerChangeRowTitle(cont: ListView, row: Int, title: String?)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.changeRowTitle(row, title)
        }
    }

    fun containerRefresh(cont: ListView)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.notifyDataSetChanged()
        }
    }

    fun containerGetColumnType(cont: ListView, column: Int): Int
    {
        var type: Int = 0

        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            type = adapter.model.getColumnType(column)
        }
        return type
    }

    fun containerDelete(cont: ListView, rowcount: Int)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.deleteRows(rowcount)
        }
    }

    fun containerRowDeleteByTitle(cont: ListView, title: String?)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.deleteRowByTitle(title)
        }
    }

    fun containerRowDeleteByData(cont: ListView, data: Long)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.deleteRowByData(data)
        }
    }

    fun containerClear(cont: ListView)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.clear()
        }
    }

    fun listBoxNew(cid: Int, multi: Int): DWListBox?
    {
        var listbox: DWListBox? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            listbox = DWListBox(this)
            listbox!!.tag = dataArrayMap
            listbox!!.id = cid
            if(multi != 0) {
                listbox!!.choiceMode = ListView.CHOICE_MODE_MULTIPLE;
            }
        }
        return listbox
    }

    fun listOrComboBoxAppend(window: View, text: String)
    {
        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                combobox.list.add(text)
            } else if(window is DWListBox) {
                val listbox = window

                listbox.list.add(text)
            }
        }
    }

    fun listOrComboBoxInsert(window: View, text: String, pos: Int)
    {
        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                combobox.list.add(pos, text)
            } else if(window is DWListBox) {
                val listbox = window

                listbox.list.add(pos, text)
            }
        }
    }

    fun listOrComboBoxClear(window: View)
    {
        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                combobox.list.clear()
            } else if(window is DWListBox) {
                val listbox = window

                listbox.list.clear()
            }
        }
    }

    fun listOrComboBoxCount(window: View): Int
    {
        var retval: Int = 0

        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                retval = combobox.list.count()
            } else if(window is DWListBox) {
                val listbox = window

                retval = listbox.list.count()
            }
        }
        return retval
    }

    fun listOrComboBoxSetText(window: View, index: Int, text: String)
    {
        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                if(index > -1 && index < combobox.list.count())
                    combobox.list[index] = text
            } else if(window is DWListBox) {
                val listbox = window

                if(index > -1 && index < listbox.list.count())
                    listbox.list[index] = text
            }
        }
    }

    fun listOrComboBoxGetText(window: View, index: Int): String?
    {
        var retval: String? = null

        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                if(index > -1 && index < combobox.list.count())
                    retval = combobox.list[index]
            } else if(window is DWListBox) {
                val listbox = window

                if(index > -1 && index < listbox.list.count())
                    retval = listbox.list[index]
            }
        }
        return retval
    }

    fun listOrComboBoxGetSelected(window: View): Int
    {
        var retval: Int = -1

        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                retval = combobox.selected
            } else if(window is DWListBox) {
                val listbox = window

                retval = listbox.selected
            }
        }
        return retval
    }

    fun listOrComboBoxSelect(window: View, index: Int, state: Int)
    {
        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                if(index < combobox.list.count() && state != 0) {
                    combobox.selected = index
                    combobox.setText(combobox.list[index])
                }
            } else if(window is DWListBox) {
                val listbox = window

                if(index < listbox.list.count()) {
                    if(state != 0) {
                        listbox.selected = index
                        listbox.setItemChecked(index, true);
                    } else {
                        listbox.setItemChecked(index, false);
                    }
                }
            }
        }
    }

    fun listOrComboBoxDelete(window: View, index: Int)
    {
        waitOnUiThread {
            if(window is DWComboBox) {
                val combobox = window

                if(index < combobox.list.count()) {
                    combobox.list.removeAt(index)
                }
            } else if(window is DWListBox) {
                val listbox = window

                if(index < listbox.list.count()) {
                    listbox.list.removeAt(index)
                }
            }
        }
    }

    fun listBoxSetTop(window: View, top: Int)
    {
        waitOnUiThread {
            if(window is DWListBox) {
                val listbox = window

                if(top < listbox.list.count()) {
                    listbox.smoothScrollToPosition(top)
                }
            }
        }
    }

    fun listBoxSelectedMulti(window: View, where: Int): Int
    {
        var retval: Int = -1

        waitOnUiThread {
            if(window is DWListBox) {
                val listbox = window
                val checked: SparseBooleanArray = listbox.getCheckedItemPositions()

                // If we are starting over....
                if(where == -1 && checked.size() > 0) {
                    retval = checked.keyAt(0)
                } else {
                    // Otherwise loop until we find our current place
                    for (i in 0 until checked.size()) {
                        // Item position in adapter
                        val position: Int = checked.keyAt(i)
                        // If we are at our current point... check to see
                        // if there is another one, and return it...
                        // otherwise we will return -1 to indicated we are done.
                        if (where == position && (i+1) < checked.size()) {
                            retval = checked.keyAt(i+1)
                        }
                    }
                }
            }
        }
        return retval
    }

    fun calendarNew(cid: Int): CalendarView?
    {
        var calendar: CalendarView? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            calendar = CalendarView(this)
            calendar!!.tag = dataArrayMap
            calendar!!.id = cid
            calendar!!.setOnDateChangeListener { calendar, year, month, day ->
                val c: Calendar = Calendar.getInstance();
                c.set(year, month, day);
                calendar.date = c.timeInMillis
            }
        }

        return calendar
    }

    fun calendarSetDate(calendar: CalendarView, date: Long)
    {
        waitOnUiThread {
            // Convert from seconds to milliseconds
            calendar.setDate(date * 1000, true, true)
        }
    }

    fun calendarGetDate(calendar: CalendarView): Long
    {
        var date: Long = 0

        waitOnUiThread {
            // Convert from milliseconds to seconds
            date = calendar.date / 1000
        }
        return date
    }

    fun bitmapNew(cid: Int): ImageView?
    {
        var imageview: ImageView? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            imageview = ImageView(this)
            imageview!!.tag = dataArrayMap
            imageview!!.id = cid
        }

        return imageview
    }

    fun windowSetBitmap(window: View, resID: Int, filename: String?)
    {
        waitOnUiThread {
            if(resID != 0) {
                if(window is ImageButton) {
                    val button = window

                    button.setImageResource(resID)
                } else if(window is ImageView) {
                    val imageview = window

                    imageview.setImageResource(resID)
                }
            } else if(filename != null) {
                var exts = arrayOf("", ".png", ".webp", ".jpg", ".jpeg", ".gif")

                for (ext in exts) {
                    // Try to load the image, and protect against exceptions
                    try {
                        val f = this.assets.open(filename + ext)
                        val b = BitmapFactory.decodeStream(f)

                        if(b != null) {
                            if (window is ImageButton) {
                                val button = window

                                button.setImageBitmap(b)
                            } else if (window is ImageView) {
                                val imageview = window

                                imageview.setImageBitmap(b)
                            }
                            break
                        }
                    } catch (e: IOException) {
                    }
                }
            }
        }
    }

    fun windowSetBitmapFromData(window: View, resID: Int, data: ByteArray?, length: Int)
    {
        waitOnUiThread {
            if(resID != 0) {
                if (window is ImageButton) {
                    val button = window

                    button.setImageResource(resID)
                } else if (window is ImageView) {
                    val imageview = window

                    imageview.setImageResource(resID)
                }
            } else if(data != null) {
                val b = BitmapFactory.decodeByteArray(data, 0, length)

                if (window is ImageButton) {
                    val button = window

                    button.setImageBitmap(b)
                } else if (window is ImageView) {
                    val imageview = window

                    imageview.setImageBitmap(b)
                }
            }
        }
    }

    fun iconNew(filename: String?, data: ByteArray?, length: Int, resID: Int): Drawable?
    {
        var icon: Drawable? = null

        waitOnUiThread {
            if(resID != 0) {
                icon = ResourcesCompat.getDrawable(resources, resID, null);
            } else if(filename != null) {
                var exts = arrayOf("", ".png", ".webp", ".jpg", ".jpeg", ".gif")

                for (ext in exts) {
                    // Try to load the image, and protect against exceptions
                    try {
                        val f = this.assets.open(filename + ext)
                        icon = Drawable.createFromStream(f, null)
                    } catch (e: IOException) {
                    }
                    if(icon != null) {
                        break
                    }

                }
            } else if(data != null) {
                icon = BitmapDrawable(resources, BitmapFactory.decodeByteArray(data, 0, length))
            }
        }
        return icon
    }

    fun pixmapNew(width: Int, height: Int, filename: String?, data: ByteArray?, length: Int, resID: Int): Bitmap?
    {
        var pixmap: Bitmap? = null

        waitOnUiThread {
            if(width > 0 && height > 0) {
                pixmap = Bitmap.createBitmap(null, width, height, Bitmap.Config.ARGB_8888)
            } else if(resID != 0) {
                pixmap = BitmapFactory.decodeResource(resources, resID);
            } else if(filename != null) {
                var exts = arrayOf("", ".png", ".webp", ".jpg", ".jpeg", ".gif")

                for (ext in exts) {
                    // Try to load the image, and protect against exceptions
                    try {
                        val f = this.assets.open(filename + ext)
                        pixmap = BitmapFactory.decodeStream(f)
                    } catch (e: IOException) {
                    }
                    if(pixmap != null) {
                        break
                    }
                }
            } else if(data != null) {
                pixmap = BitmapFactory.decodeByteArray(data, 0, length)
            }
        }
        return pixmap
    }

    fun pixmapGetDimensions(pixmap: Bitmap): Long
    {
        var dimensions: Long = 0

        waitOnUiThread {
            dimensions = pixmap.width.toLong() or (pixmap.height.toLong() shl 32)
        }
        return dimensions
    }

    fun screenGetDimensions(): Long
    {
        val dm = resources.displayMetrics
        return dm.widthPixels.toLong() or (dm.heightPixels.toLong() shl 32)
    }

    fun renderNew(cid: Int): DWRender?
    {
        var render: DWRender? = null

        waitOnUiThread {
            var dataArrayMap = SimpleArrayMap<String, Long>()

            render = DWRender(this)
            render!!.tag = dataArrayMap
            render!!.id = cid
        }
        return render
    }

    fun renderRedraw(render: DWRender)
    {
        runOnUiThread {
            render.invalidate()
        }
    }

    fun pixmapBitBlt(dstr: DWRender?, dstp: Bitmap?, dstx: Int, dsty: Int, dstw: Int, dsth: Int,
                     srcr: DWRender?, srcp: Bitmap?, srcy: Int, srcx: Int, srcw: Int, srch: Int): Int
    {
        val dst = Rect(dstx, dsty, dstx + dstw, dsty + dsth)
        var src = Rect(srcx, srcy, srcx + srcw, srcy + srch)
        var retval: Int = 1

        if(srcw == -1) {
            src.right = srcx + dstw
        }
        if(srch == -1) {
            src.bottom = srcy + dsth
        }

        waitOnUiThread {
            var canvas: Canvas? = null
            var bitmap: Bitmap? = null

            if(dstr != null) {
                canvas = dstr.cachedCanvas
            } else if(dstp != null) {
                canvas = Canvas(dstp)
            }

            if(srcp != null) {
                bitmap = srcp
            } else if(srcr != null) {
                bitmap = Bitmap.createBitmap(srcr.layoutParams.width,
                                             srcr.layoutParams.height, Bitmap.Config.ARGB_8888)
                val c = Canvas(bitmap)
                srcr.layout(srcr.left, srcr.top, srcr.right, srcr.bottom)
                srcr.draw(c)
            }

            if(canvas != null && bitmap != null) {
                canvas.drawBitmap(bitmap, src, dst, null)
                retval = 0
            }
        }
        return retval
    }

    fun drawPoint(render: DWRender?, bitmap: Bitmap?, x: Int, y: Int)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                canvas.drawPoint(x.toFloat(), y.toFloat(), Paint())
            }
        }
    }

    fun drawLine(render: DWRender?, bitmap: Bitmap?, x1: Int, y1: Int, x2: Int, y2: Int)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                paint.flags = 0
                paint.style = Paint.Style.STROKE
                canvas.drawLine(x1.toFloat(), y1.toFloat(), x2.toFloat(), y2.toFloat(), paint)
            }
        }
    }

    fun fontTextExtentsGet(render: DWRender?, bitmap: Bitmap?, text:String, typeface: Typeface?, fontsize: Int, window: View?): Long
    {
        var dimensions: Long = 0

        waitOnUiThread {
            var rect = Rect()

            if (render != null) {
                if (render.typeface != null) {
                    paint.typeface = render.typeface
                    if (render.fontsize != null && render.fontsize!! > 0F) {
                        paint.textSize = render.fontsize!!
                    }
                }
            } else if (bitmap != null) {
                if (typeface != null) {
                    paint.typeface = typeface
                    if (fontsize > 0) {
                        paint.textSize = fontsize.toFloat()
                    }
                } else if (window != null && window is DWRender) {
                    val secondary: DWRender = window as DWRender

                    if (secondary.typeface != null) {
                        paint.typeface = secondary.typeface
                        if (secondary.fontsize != null && secondary.fontsize!! > 0F) {
                            paint.textSize = secondary.fontsize!!
                        }
                    }
                }
            }
            paint.getTextBounds(text, 0, text.length, rect)
            val textheight = rect.bottom - rect.top
            val textwidth = rect.right - rect.left
            dimensions = textwidth.toLong() or (textheight.toLong() shl 32)
        }
        return dimensions
    }

    fun drawText(render: DWRender?, bitmap: Bitmap?, x: Int, y: Int, text:String, typeface: Typeface?, fontsize: Int, window: View?)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null && render.cachedCanvas != null) {
                canvas = render.cachedCanvas
                if(render.typeface != null) {
                    paint.typeface = render.typeface
                    if(render.fontsize != null && render.fontsize!! > 0F) {
                        paint.textSize = render.fontsize!!
                    }
                }
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
                if(typeface != null) {
                    paint.typeface = typeface
                    if(fontsize > 0) {
                        paint.textSize = fontsize.toFloat()
                    }
                } else if(window != null && window is DWRender) {
                    val secondary: DWRender = window as DWRender

                    if(secondary.typeface != null) {
                        paint.typeface = secondary.typeface
                        if(secondary.fontsize != null && secondary.fontsize!! > 0F) {
                            paint.textSize = secondary.fontsize!!
                        }
                    }
                }
            }

            if(canvas != null) {
                // Save the old color for later...
                var rect = Rect()
                val oldcolor = paint.color
                // Prepare to draw the background rect
                paint.color = bgcolor
                paint.flags = 0
                paint.style = Paint.Style.FILL_AND_STROKE
                paint.textAlign = Paint.Align.LEFT
                paint.getTextBounds(text, 0, text.length, rect)
                val textheight = rect.bottom - rect.top
                rect.top += y + textheight
                rect.bottom += y + textheight
                rect.left += x
                rect.right += x
                canvas.drawRect(rect, paint)
                // Restore the color and prepare to draw text
                paint.color = oldcolor
                paint.style = Paint.Style.STROKE
                canvas.drawText(text, x.toFloat(), y.toFloat() + textheight.toFloat(), paint)
            }
        }
    }

    fun drawRect(render: DWRender?, bitmap: Bitmap?, x: Int, y: Int, width: Int, height: Int)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                paint.flags = 0
                paint.style = Paint.Style.FILL_AND_STROKE
                canvas.drawRect(x.toFloat(), y.toFloat(), x.toFloat() + width.toFloat(), y.toFloat() + height.toFloat(), paint)
            }
        }
    }

    fun drawPolygon(render: DWRender?, bitmap: Bitmap?, flags: Int, npoints: Int, x: IntArray, y: IntArray)
    {
        // Create a path with all our points
        val path = Path()

        path.moveTo(x[0].toFloat(), y[0].toFloat())
        for (i in 1 until npoints) {
            path.lineTo(x[i].toFloat(), y[i].toFloat())
        }

        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                // Handle the DW_DRAW_NOAA flag
                if((flags and (1 shl 2)) == 0) {
                    paint.flags = Paint.ANTI_ALIAS_FLAG
                } else {
                    paint.flags = 0
                }
                // Handle the DW_DRAW_FILL flag
                if((flags and 1)  == 1) {
                    paint.style = Paint.Style.FILL_AND_STROKE
                } else {
                    paint.style = Paint.Style.STROKE
                }
                canvas.drawPath(path, paint)
            }
        }
    }

    fun drawArc(render: DWRender?, bitmap: Bitmap?, flags: Int, xorigin: Int, yorigin: Int,
                x1: Int, y1: Int, x2: Int, y2: Int)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                var a1: Double = Math.atan2((y1 - yorigin).toDouble(), (x1 - xorigin).toDouble())
                var a2: Double = Math.atan2((y2 - yorigin).toDouble(), (x2 - xorigin).toDouble())
                val dx = (xorigin - x1).toDouble()
                val dy = (yorigin - y1).toDouble()
                val r: Double = Math.sqrt(dx * dx + dy * dy)
                val left = (xorigin-r).toFloat()
                val top = (yorigin-r).toFloat()
                val rect = RectF(left, top, (left + (r*2)).toFloat(), (top + (r*2)).toFloat())

                /* Convert to degrees */
                a1 *= 180.0 / Math.PI
                a2 *= 180.0 / Math.PI
                val sweep = Math.abs(a1 - a2)

                // Handle the DW_DRAW_NOAA flag
                if((flags and (1 shl 2)) == 0) {
                    paint.flags = Paint.ANTI_ALIAS_FLAG
                } else {
                    paint.flags = 0
                }
                // Handle the DW_DRAW_FILL flag
                if((flags and 1)  == 1) {
                    paint.style = Paint.Style.FILL_AND_STROKE
                } else {
                    paint.style = Paint.Style.STROKE
                }
                // Handle the DW_DRAW_FULL flag
                if((flags and (1 shl 1)) != 0) {
                    canvas.drawOval(rect, paint)
                } else {
                    canvas.drawArc(rect, a1.toFloat(), sweep.toFloat(), false, paint)
                }
            }
        }
    }

    fun colorSet(alpha: Int, red: Int, green: Int, blue: Int)
    {
        waitOnUiThread {
            if(alpha != 0) {
                paint.color = Color.argb(alpha, red, green, blue)
            } else {
                paint.color = Color.rgb(red, green, blue)
            }
        }
    }

    fun bgColorSet(alpha: Int, red: Int, green: Int, blue: Int)
    {
        if(alpha != 0) {
            this.bgcolor = Color.argb(alpha, red, green, blue)
        } else {
            this.bgcolor = Color.rgb(red, green, blue)
        }
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

    fun fileBrowse(title: String, defpath: String?, ext: String?, flags: Int): String?
    {
        var retval: String? = null

        waitOnUiThread {
            val fc = DWFileChooser(this)
            fc.setFileListener(object: DWFileChooser.FileSelectedListener {
                    override fun fileSelected(file: File?) {
                        // do something with the file
                        retval = file!!.absolutePath
                        throw java.lang.RuntimeException()
                    }
                })
            if(ext != null) {
                fc.setExtension(ext)
            }
            fc.showDialog()
        }

        // loop till a runtime exception is triggered.
        try {
            Looper.loop()
        } catch (e2: RuntimeException) {
        }

        return retval
    }

    fun messageBox(title: String, body: String, flags: Int): Int
    {
        var retval: Int = 0

        waitOnUiThread {
            // make a text input dialog and show it
            var alert = AlertDialog.Builder(this)

            alert.setTitle(title)
            alert.setMessage(body)
            if ((flags and (1 shl 3)) != 0) {
                alert.setPositiveButton("Yes",
                    //android.R.string.yes,
                    DialogInterface.OnClickListener { _: DialogInterface, _: Int ->
                        retval = 1
                        throw java.lang.RuntimeException()
                    });
            }
            if ((flags and ((1 shl 1) or (1 shl 2))) != 0) {
                alert.setNegativeButton(
                    android.R.string.ok,
                    DialogInterface.OnClickListener { _: DialogInterface, _: Int ->
                        retval = 0
                        throw java.lang.RuntimeException()
                    });
            }
            if ((flags and ((1 shl 3) or (1 shl 4))) != 0) {
                alert.setNegativeButton("No",
                    //android.R.string.no,
                    DialogInterface.OnClickListener { _: DialogInterface, _: Int ->
                        retval = 0
                        throw java.lang.RuntimeException()
                    });
            }
            if ((flags and ((1 shl 2) or (1 shl 4))) != 0) {
                alert.setNeutralButton(
                    android.R.string.cancel,
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
        }
        return retval
    }

    fun isUIThread(): Boolean
    {
        if(Looper.getMainLooper() == Looper.myLooper()) {
            return true
        }
        return false
    }

    fun mainSleep(milliseconds: Int)
    {
        // If we are on the main UI thread... add an idle handler
        // Then loop until we throw an exception when the time expires
        // in the idle handler, if we are already thrown... remove the handler
        if(Looper.getMainLooper() == Looper.myLooper()) {
            val starttime = System.currentTimeMillis()

            // Waiting for Idle to make sure Toast gets rendered.
            Looper.myQueue().addIdleHandler(object : MessageQueue.IdleHandler {
                var thrown: Boolean = false

                override fun queueIdle(): Boolean {
                    if(System.currentTimeMillis() - starttime >= milliseconds) {
                        if (thrown == false) {
                            thrown = true
                            throw java.lang.RuntimeException()
                        }
                        return false
                    }
                    return true
                }
            })

            // loop till a runtime exception is triggered.
            try {
                Looper.loop()
            } catch (e2: RuntimeException) {
            }
        }
        else
        {
            // If we are in a different thread just sleep
            Thread.sleep(milliseconds.toLong())
        }
    }

    fun dwindowsExit(exitcode: Int)
    {
        waitOnUiThread {
            this.finishAffinity()
            System.exit(exitcode)
        }
    }

    fun dwindowsShutdown()
    {
        waitOnUiThread {
            this.finishAffinity()
        }
    }

    fun dwInit(appid: String, appname: String): Int
    {
        waitOnUiThread {
            // Create the notification channel in dw_init()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                // Create the NotificationChannel
                val importance = NotificationManager.IMPORTANCE_DEFAULT
                val mChannel = NotificationChannel(appid, appname, importance)
                // Register the channel with the system; you can't change the importance
                // or other notification behaviors after this
                val notificationManager =
                    getSystemService(NOTIFICATION_SERVICE) as NotificationManager
                notificationManager.createNotificationChannel(mChannel)
            }
        }
        return Build.VERSION.SDK_INT
    }

    fun dwMain()
    {
        runOnUiThread {
            // Trigger the options menu to update when dw_main() is called
            invalidateOptionsMenu()
        }
    }

    fun androidGetRelease(): String
    {
        return Build.VERSION.RELEASE
    }

    fun notificationNew(title: String, imagepath: String, text: String, appid: String): NotificationCompat.Builder?
    {
        var builder: NotificationCompat.Builder? = null

        waitOnUiThread {
            builder = NotificationCompat.Builder(this, appid)
                .setContentTitle(title)
                .setContentText(text)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
        }
        return builder
    }

    fun notificationSend(builder: NotificationCompat.Builder)
    {
        waitOnUiThread {
            notificationID += 1
            with(NotificationManagerCompat.from(this)) {
                // notificationId is a unique int for each notification that you must define
                notify(notificationID, builder.build())
            }
        }
    }

    /*
     * Native methods that are implemented by the 'dwindows' native library,
     * which is packaged with this application.
     */
    external fun dwindowsInit(dataDir: String, appid: String)
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
    external fun eventHandlerHTMLResult(obj1: View, message: Int, result: String, data: Long)
    external fun eventHandlerContainer(obj1: View, message: Int, title: String?, x: Int, y: Int, data: Long)

    companion object
    {
        // Used to load the 'dwindows' library on application startup.
        init
        {
            System.loadLibrary("dwindows")
        }
    }
}