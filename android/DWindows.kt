package org.dbsoft.dwindows

import android.R
import android.annotation.SuppressLint
import android.app.Activity
import android.app.Dialog
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.*
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.content.res.Resources
import android.database.Cursor
import android.graphics.*
import android.graphics.drawable.BitmapDrawable
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.media.AudioManager
import android.media.ToneGenerator
import android.net.Uri
import android.os.*
import android.provider.DocumentsContract
import android.provider.MediaStore
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
import androidx.constraintlayout.widget.Placeholder
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.res.ResourcesCompat
import androidx.core.view.MenuCompat
import androidx.recyclerview.widget.RecyclerView
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayout.OnTabSelectedListener
import com.google.android.material.tabs.TabLayoutMediator
import java.io.BufferedInputStream
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.util.*
import java.util.concurrent.locks.ReentrantLock
import java.util.zip.ZipEntry
import java.util.zip.ZipFile

object DWEvent {
    const val TIMER = 0
    const val CONFIGURE = 1
    const val KEY_PRESS = 2
    const val BUTTON_PRESS = 3
    const val BUTTON_RELEASE = 4
    const val MOTION_NOTIFY = 5
    const val DELETE = 6
    const val EXPOSE = 7
    const val CLICKED = 8
    const val ITEM_ENTER = 9
    const val ITEM_CONTEXT = 10
    const val LIST_SELECT = 11
    const val ITEM_SELECT = 12
    const val SET_FOCUS = 13
    const val VALUE_CHANGED = 14
    const val SWITCH_PAGE = 15
    const val TREE_EXPAND = 16
    const val COLUMN_CLICK = 17
    const val HTML_RESULT = 18
    const val HTML_CHANGED = 19
}

val DWImageExts = arrayOf("", ".png", ".webp", ".jpg", ".jpeg", ".gif")

class DWTabViewPagerAdapter : RecyclerView.Adapter<DWTabViewPagerAdapter.DWEventViewHolder>() {
    val viewList = mutableListOf<LinearLayout>()
    val pageList = mutableListOf<Long>()
    var currentPageID = 0L
    var recyclerView: RecyclerView? = null

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
            DWEventViewHolder(viewList.get(viewType))

    override fun getItemCount() = viewList.count()
    override fun getItemViewType(position: Int): Int {
        return position
    }
    override fun onAttachedToRecyclerView(rv: RecyclerView) {
        recyclerView = rv
    }
    override fun onBindViewHolder(holder: DWEventViewHolder, position: Int) {
        holder.setIsRecyclable(false)
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
        eventHandlerHTMLChanged(view, DWEvent.HTML_CHANGED, url, 1)
    }

    override fun onPageFinished(view: WebView, url: String) {
        // Handle the DW_HTML_CHANGE_COMPLETE event
        eventHandlerHTMLChanged(view, DWEvent.HTML_CHANGED, url, 4)
    }

    external fun eventHandlerHTMLChanged(obj1: View, message: Int, URI: String, status: Int)
}

class DWSpinButton(context: Context) : AppCompatEditText(context), OnTouchListener {
    var value: Long = 0
    var minimum: Long = 0
    var maximum: Long = 65535

    init {
        setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_media_previous, 0, R.drawable.ic_media_next, 0)
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
                eventHandlerInt(DWEvent.VALUE_CHANGED, value.toInt(), 0, 0, 0)
                return true
            } else if (event.x <= v.compoundDrawables[DRAWABLE_LEFT].bounds.width()) {
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
                eventHandlerInt(DWEvent.VALUE_CHANGED, value.toInt(), 0, 0, 0)
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
        setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.arrow_down_float, 0)
        setOnTouchListener(this)
        lpw = ListPopupWindow(context)
        lpw!!.setAdapter(
            ArrayAdapter(
                context,
                R.layout.simple_list_item_1, list
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
        eventHandlerInt(DWEvent.LIST_SELECT, position, 0, 0, 0)
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
    var colorFore: Int? = null
    var colorBack: Int? = null

    init {
        setAdapter(
            object : ArrayAdapter<String>(
                context,
                R.layout.simple_list_item_1, list
            ) {
                override fun getView(pos: Int, view: View?, parent: ViewGroup): View {
                    val thisview = super.getView(pos, view, parent)
                    val textview = thisview as TextView
                    if (colorFore != null) {
                        textview.setTextColor(colorFore!!)
                    }
                    if (colorBack != null) {
                        textview.setBackgroundColor(colorBack!!)
                    }
                    return thisview
                }
            }
        )
        onItemClickListener = this
    }

    override fun onItemClick(parent: AdapterView<*>?, view: View, position: Int, id: Long) {
        selected = position
        eventHandlerInt(DWEvent.LIST_SELECT, position, 0, 0, 0)
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
    var evx: Float = 0f
    var evy: Float = 0f
    var button: Int = 1

    override fun onSizeChanged(width: Int, height: Int, oldWidth: Int, oldHeight: Int) {
        super.onSizeChanged(width, height, oldWidth, oldHeight)
        // Send DW_SIGNAL_CONFIGURE
        eventHandlerInt(DWEvent.CONFIGURE, width, height, 0, 0)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        cachedCanvas = canvas
        // Send DW_SIGNAL_EXPOSE
        eventHandlerInt(DWEvent.EXPOSE, 0, 0, this.width, this.height)
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

    fun createMenu(newmenu: Menu?, recreate: Boolean) {
        var refresh = recreate

        if(newmenu != null) {
            if(newmenu != menu) {
                menu = newmenu
                refresh = true
            }
        }
        if(menu != null) {
            var group = 0

            if(refresh) {
                menu!!.clear()
            }

            // Enable group dividers for separators
            MenuCompat.setGroupDividerEnabled(menu, true)

            for (menuitem in children) {
                // Submenus on Android can't have submenus, so stop at depth 1
                if (menuitem.submenu != null && menu !is SubMenu) {
                    if(menuitem.submenuitem == null || refresh) {
                        menuitem.submenuitem = menu?.addSubMenu(group, menuitem.id, 0, menuitem.title)
                    }
                    menuitem.submenu!!.createMenu(menuitem.submenuitem, refresh)
                } else if(menuitem.submenu == null) {
                    if(menuitem.title!!.isEmpty()) {
                        group += 1
                    } else if(menuitem.menuitem == null || refresh) {
                        menuitem.menuitem = menu?.add(group, menuitem.id, 0, menuitem.title)
                        menuitem.menuitem!!.isCheckable = menuitem.check
                        menuitem.menuitem!!.isChecked = menuitem.checked
                        menuitem.menuitem!!.isEnabled = menuitem.enabled
                        menuitem.menuitem!!.setOnMenuItemClickListener { item: MenuItem? ->
                            eventHandlerSimple(menuitem, DWEvent.CLICKED)
                            true
                        }
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
        val index: Int = (row * columns.size) + column

        if(index > -1 && index < data.size) {
            return data[index]
        }
        return null
    }

    fun setRowAndColumn(row: Int, column: Int, obj: Any?)
    {
        val index: Int = (row * columns.size) + column

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
        val startRow: Long = numberOfRows().toLong()

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
    var oddColor: Int? = null
    var evenColor: Int? = null
    var foreColor: Int? = null
    var backColor: Int? = null
    var lastClick: Long = 0
    var lastClickRow: Int = -1

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
            rowView.orientation = LinearLayout.HORIZONTAL

            for(i in 0 until displayColumns) {
                val content = model.getRowAndColumn(position, i)

                // Image
                if((model.getColumnType(i) and 1) != 0) {
                    val imageview = ImageView(context)
                    val params = LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                                                           LinearLayout.LayoutParams.WRAP_CONTENT)
                    params.gravity = Gravity.CENTER
                    imageview.layoutParams = params
                    imageview.id = View.generateViewId()
                    if (content is Drawable) {
                        imageview.setImageDrawable(content)
                    }
                    rowView.addView(imageview)
                } else  {
                    // Everything else id displayed as text
                    val textview = TextView(context)
                    val params = LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                                           LinearLayout.LayoutParams.WRAP_CONTENT)
                    params.gravity = Gravity.CENTER
                    textview.layoutParams = params
                    textview.id = View.generateViewId()
                    if (content is String) {
                        textview.text = content
                    } else if(content is Int) {
                        textview.text = content.toString()
                    }
                    rowView.addView(textview)
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
                        if(foreColor != null) {
                            textview.setTextColor(foreColor!!)
                        }
                    }
                }
            }
        }
        // Handle row stripe
        if (position % 2 == 0) {
            if(evenColor != null) {
                rowView.setBackgroundColor(evenColor!!)
            } else if(backColor != null) {
                rowView.setBackgroundColor(backColor!!)
            }
        } else {
            if(oddColor != null) {
                rowView.setBackgroundColor(oddColor!!)
            } else if(backColor != null) {
                rowView.setBackgroundColor(backColor!!)
            }
        }
        return rowView
    }
}

class DWindows : AppCompatActivity() {
    var windowLayout: ViewPager2? = null
    var threadLock = ReentrantLock()
    var threadCond = threadLock.newCondition()
    var notificationID: Int = 0
    var darkMode: Int = -1
    var lastClickView: View? = null
    private var paint = Paint()
    private var bgcolor: Int? = null
    private var fileURI: Uri? = null
    private var fileLock = ReentrantLock()
    private var fileCond = threadLock.newCondition()
    // Lists of data for our Windows
    private var windowTitles = mutableListOf<String?>()
    private var windowMenuBars = mutableListOf<DWMenu?>()
    private var windowStyles = mutableListOf<Int>()
    private var windowDefault = mutableListOf<View?>()

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

    // Returns true if the filename is a resource ID (non-zero number)
    // with a image file extension in our DWImageExts list
    private fun isDWResource(filename: String): Boolean {
        val length = filename.length

        for (ext in DWImageExts) {
            if (ext.isNotEmpty() && filename.endsWith(ext)) {
                val filebody: String = filename.substring(7, length - ext.length)
                try {
                    if (filebody.toInt() > 0) {
                        return true
                    }
                } catch(e: NumberFormatException) {
                }
            }
        }
        return false
    }

    // Extracts assets/ in the APK to the application cache directory
    private fun extractAssets() {
        var zipFile: ZipFile? = null
        val targetDir = cacheDir

        try {
            zipFile = ZipFile(this.applicationInfo.sourceDir)
            val e: Enumeration<out ZipEntry?> = zipFile.entries()
            while (e.hasMoreElements()) {
                val entry: ZipEntry? = e.nextElement()
                if (entry == null || entry.isDirectory || !entry.name.startsWith("assets/") ||
                        isDWResource(entry.name)) {
                    continue
                }
                val targetFile = File(targetDir, entry.name.substring("assets/".length))
                targetFile.parentFile!!.mkdirs()
                val tempBuffer = ByteArray(entry.size.toInt())
                var ais: BufferedInputStream? = null
                var aos: FileOutputStream? = null
                try {
                    ais = BufferedInputStream(zipFile.getInputStream(entry))
                    aos = FileOutputStream(targetFile)
                    ais.read(tempBuffer)
                    aos.write(tempBuffer)
                } catch (e: IOException) {
                } finally {
                    ais?.close()
                    aos?.close()
                }
            }
        } catch (e: IOException) {
        } finally {
            zipFile?.close()
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
        val c = cacheDir.path

        // Extract any non-resource assets to the cache directory
        // So that our C code can access them as files, like on
        // other Dynamic Windows platforms
        extractAssets()

        // Setup our ViewPager2 as our activty window container
        windowLayout = ViewPager2(this)
        windowLayout!!.id = View.generateViewId()
        windowLayout!!.adapter = DWTabViewPagerAdapter()
        windowLayout!!.isUserInputEnabled = false
        windowLayout!!.layoutParams =
            ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )

        // Initialize the Dynamic Windows code...
        // This will start a new thread that calls the app's dwmain()
        dwindowsInit(s, c, this.getPackageName())
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
            val width: Int = windowLayout!!.width
            val height: Int = windowLayout!!.height
            val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
            val index = windowLayout!!.currentItem
            val count = adapter.viewList.count()

            if(count > 0 && index < count) {
                val window = adapter.viewList[index]

                eventHandlerInt(window, DWEvent.CONFIGURE, width, height, 0, 0)
            }
        }
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        if(windowLayout != null) {
            val index = windowLayout!!.currentItem
            val count = windowMenuBars.count()

            if (count > 0 && index < count) {
                var menuBar = windowMenuBars[index]

                if(menuBar == null) {
                    menuBar = DWMenu()
                    windowMenuBars[index] = menuBar
                }
                menuBar!!.menu = menu
                return super.onCreateOptionsMenu(menu)
            }
        }
        return false
    }

    override fun onPrepareOptionsMenu(menu: Menu?): Boolean {
        if(windowLayout != null) {
            val index = windowLayout!!.currentItem
            val count = windowMenuBars.count()

            if (count > 0 && index < count) {
                var menuBar = windowMenuBars[index]

                if(menuBar != null) {
                    menuBar!!.createMenu(menu, true)
                } else {
                    menuBar = DWMenu()
                    menuBar!!.createMenu(menu, true)
                    windowMenuBars[index] = menuBar
                }
                return super.onPrepareOptionsMenu(menu)
            }
        }
        return false
    }

    override fun onBackPressed() {
        if(windowLayout != null) {
            val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
            val index = windowLayout!!.currentItem
            val count = windowStyles.count()

            // If the current window has a close button...
            if (count > 1 && index > 0 && index < count && (windowStyles[index] and 1) == 1) {
                // Send the DW_SIGNAL_DELETE to the event handler
                eventHandlerSimple(adapter.viewList[index], DWEvent.DELETE)
            }
        }
    }

    // These are the Android calls to actually create the UI...
    // forwarded from the C Dynamic Windows API
    
    fun darkModeDetected(): Int
    {
        return darkMode
    }

    fun menuPopup(menu: DWMenu, parent: View, x: Int, y: Int)
    {
        var anchor: View? = parent

        // If lastClickView is valid, use that instead of parent
        if(lastClickView != null) {
            anchor = lastClickView
        }

        runOnUiThread {
            val popup = PopupMenu(this, anchor)

            menu.createMenu(popup.menu, false)
            popup.show()
        }
    }

    fun menuBarNew(location: View): DWMenu?
    {
        var menuBar: DWMenu? = null

        if(windowLayout != null) {
            waitOnUiThread {
                val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
                val index = adapter.viewList.indexOf(location)

                if (index != -1) {
                    menuBar = DWMenu()
                    windowMenuBars[index] = menuBar
                }
            }
        }
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
                    if(menuitem.menuitem != null) {
                        runOnUiThread {
                            menuitem.menuitem!!.isEnabled = enabled
                            invalidateOptionsMenu()
                        }
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
                    if(menuitem.menuitem != null) {
                        runOnUiThread {
                            menuitem.menuitem!!.isChecked = checked
                            invalidateOptionsMenu()
                        }
                    }
                }
            }
        }
    }

    fun windowNew(title: String, style: Int): LinearLayout? {
        var window: LinearLayout? = null

        if(windowLayout != null) {
            waitOnUiThread {
                val dataArrayMap = SimpleArrayMap<String, Long>()
                val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter

                setContentView(windowLayout)

                window = LinearLayout(this)
                window!!.visibility = View.GONE
                window!!.tag = dataArrayMap
                window!!.layoutParams =
                    LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        LinearLayout.LayoutParams.MATCH_PARENT
                    )

                // Update our window list
                adapter.viewList.add(window!!)
                windowTitles.add(title)
                windowMenuBars.add(null)
                windowStyles.add(style)
                windowDefault.add(null)

                // If this is our first/only window...
                // We can set stuff immediately
                if (adapter.viewList.count() == 1) {
                    this.title = title
                    windowLayout!!.setCurrentItem(0, false)
                    if((windowStyles[0] and 2) != 2) {
                        supportActionBar?.hide()
                    }
                }
            }
        }
        return window
    }

    fun windowSetFocus(window: View)
    {
        waitOnUiThread {
            window.requestFocus()
        }
    }

    fun windowDefault(window: View, default: View)
    {
        if(windowLayout != null) {
            val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
            val index = adapter.viewList.indexOf(window)

            if (index != -1) {
                windowDefault[index] = default
            }
        }
    }

    fun windowSetStyle(window: View, style: Int, mask: Int)
    {
        waitOnUiThread {
            if (window is TextView && window !is EditText) {
                val ourmask = (Gravity.HORIZONTAL_GRAVITY_MASK or Gravity.VERTICAL_GRAVITY_MASK) and mask

                if (ourmask != 0) {
                    // Gravity with the masked parts zeroed out
                    val newgravity = style and ourmask

                    window.gravity = newgravity
                }
            }
        }
    }

    fun windowFromId(window: View, cid: Int): View {
        return window.findViewById(cid)
    }

    fun windowSetData(window: View, name: String, data: Long) {
        if (window.tag != null) {
            val dataArrayMap: SimpleArrayMap<String, Long> = window.tag as SimpleArrayMap<String, Long>

            if (data != 0L) {
                dataArrayMap.put(name, data)
            } else {
                dataArrayMap.remove(name)
            }
        }
    }

    fun windowGetPreferredSize(window: View): Long
    {
        var retval: Long = 0

        waitOnUiThread {
            window.measure(0, 0)
            val width = window.measuredWidth
            val height = window.measuredHeight
            retval = width.toLong() or (height.toLong() shl 32)
            if(window is SeekBar) {
                val slider = window as SeekBar

                // If the widget is rotated, swap width and height
                if(slider.rotation == 270F || slider.rotation == 90F) {
                    retval = height.toLong() or (width.toLong() shl 32)
                }
            }
        }
        return retval
    }

    fun windowGetData(window: View, name: String): Long {
        var retval = 0L

        if (window.tag != null) {
            val dataArrayMap: SimpleArrayMap<String, Long> = window.tag as SimpleArrayMap<String, Long>

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
            val typeface: Typeface?

            if (bold) {
                fontFamily = font.substringBefore(" Bold")
            } else if (italic) {
                fontFamily = font.substringBefore(" Italic")
            }

            var style: Int = Typeface.NORMAL
            if (bold && italic) {
                style = Typeface.BOLD_ITALIC
            } else if (bold) {
                style = Typeface.BOLD
            } else if (italic) {
                style = Typeface.ITALIC
            }
            typeface = Typeface.create(fontFamily, style)

            return typeface
        }
        return Typeface.DEFAULT
    }

    fun windowSetFont(window: View, fontname: String?) {
        val typeface: Typeface? = typefaceFromFontName(fontname)
        var size: Float? = null

        if(fontname != null) {
            size = fontname.substringBefore('.').toFloatOrNull()
        }

        if(typeface != null) {
            waitOnUiThread {
                if (window is TextView) {
                    val textview: TextView = window
                    textview.typeface = typeface
                    if(size != null) {
                        textview.textSize = size
                    }
                } else if (window is Button) {
                    val button: Button = window
                    button.typeface = typeface
                    if(size != null) {
                        button.textSize = size
                    }
                } else if(window is DWRender) {
                    val render: DWRender = window
                    render.typeface = typeface
                    if(size != null) {
                        render.fontsize = size
                    }
                }
            }
        }
    }

    fun windowGetFont(window: View): String?
    {
        var fontname: String? = null

        waitOnUiThread {
            var typeface: Typeface? = null
            var fontsize: Float? = null

            if(window is DWRender) {
                typeface = window.typeface
                fontsize = window.fontsize
            } else if(window is TextView) {
                typeface = window.typeface
                fontsize = window.textSize
            } else if(window is Button) {
                typeface = window.typeface
                fontsize = window.textSize
            }

            if(typeface != null && fontsize != null) {
                val isize = fontsize.toInt()
                val name = typeface.toString()

                fontname = "$isize.$name"
            }
        }
        return null
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
                val textview: TextView = window

                // Handle DW_CLR_DEFAULT
                if(fore == 16) {
                    val value = TypedValue()
                    this.theme.resolveAttribute(R.attr.editTextColor, value, true)
                    colorfore = value.data
                }
                textview.setTextColor(colorfore)
                textview.setBackgroundColor(colorback)
            } else if (window is Button) {
                val button: Button = window

                // Handle DW_CLR_DEFAULT
                if(fore == 16) {
                    val value = TypedValue()
                    this.theme.resolveAttribute(R.attr.colorButtonNormal, value, true)
                    colorfore = value.data
                }
                button.setTextColor(colorfore)
                button.setBackgroundColor(colorback)
            } else if(window is LinearLayout) {
                val box: LinearLayout = window

                box.setBackgroundColor(colorback)
            } else if(window is DWListBox) {
                val listbox = window as DWListBox

                // Handle DW_CLR_DEFAULT
                if(fore == 16) {
                    val value = TypedValue()
                    this.theme.resolveAttribute(R.attr.editTextColor, value, true)
                    colorfore = value.data
                }

                listbox.colorFore = colorfore
                listbox.colorBack = colorback

                listbox.setBackgroundColor(colorback)
            } else if(window is ListView) {
                val cont = window as ListView
                val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

                // Handle DW_CLR_DEFAULT
                if(fore == 16) {
                    val value = TypedValue()
                    this.theme.resolveAttribute(R.attr.editTextColor, value, true)
                    colorfore = value.data
                }

                adapter.foreColor = colorfore
                adapter.backColor = colorback

                cont.setBackgroundColor(colorback)
            }
        }
    }

    fun windowSetText(window: View, text: String) {
        if(windowLayout != null) {
            waitOnUiThread {
                if (window is TextView) {
                    val textview: TextView = window
                    textview.text = text
                } else if (window is Button) {
                    val button: Button = window
                    button.text = text
                } else if (window is LinearLayout) {
                    val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
                    val index = adapter.viewList.indexOf(window)

                    if(index != -1) {
                        windowTitles[index] = text
                        if(index == windowLayout!!.currentItem) {
                            this.title = text
                        }
                    }
                }
            }
        }
    }

    fun windowGetText(window: View): String? {
        var text: String? = null

        if(windowLayout != null) {
            waitOnUiThread {
                if (window is TextView) {
                    val textview: TextView = window
                    text = textview.text.toString()
                } else if (window is Button) {
                    val button: Button = window
                    text = button.text.toString()
                } else if (window is LinearLayout) {
                    val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
                    val index = adapter.viewList.indexOf(window)

                    if(index != -1) {
                        text = windowTitles[index]
                    }
                }
            }
        }
        return text
    }

    private fun windowSwitchWindow(index: Int)
    {
        val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter

        if (index != -1) {
            val window = adapter.viewList[index] as View

            // Only allow a window to become active if it is shown
            if(window.visibility == View.VISIBLE) {
                // Update the action bar title
                this.title = windowTitles[index]
                // Switch the visible view in the pager
                if(adapter.recyclerView != null) {
                    adapter.recyclerView!!.scrollToPosition(index)
                }
                // This is how I prefered to do it, but it doesn't work...
                // So using RecyclerView.scrollToPosition() also
                windowLayout!!.setCurrentItem(index, true)

                // Hide or show the actionbar based on the titlebar flag
                if((windowStyles[index] and 2) == 2) {
                    supportActionBar?.show()
                } else {
                    supportActionBar?.hide()
                }
                // If the new view has a default item, focus it
                if(windowDefault[index] != null) {
                    windowDefault[index]?.requestFocus()
                }
                // Add or remove a back button depending on the visible window
                if(index > 0 && (windowStyles[index] and 1) == 1) {
                    this.actionBar?.setDisplayHomeAsUpEnabled(true)
                } else {
                    this.actionBar?.setDisplayHomeAsUpEnabled(false)
                }
                // Invalidate the menu, so it gets updated for the new window
                invalidateOptionsMenu()
            }
        }
    }

    fun windowHideShow(window: View, state: Int)
    {
        if(windowLayout != null) {
            waitOnUiThread {
                val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
                val index = adapter.viewList.indexOf(window)

                if(state == 0) {
                    window.visibility = View.GONE
                } else {
                    window.visibility = View.VISIBLE
                }
                windowSwitchWindow(index)
            }
        }
    }

    fun windowDestroy(window: View): Int {
        var retval: Int = 1 // DW_ERROR_GENERAL

        if(windowLayout != null) {
            waitOnUiThread {
                val adapter: DWTabViewPagerAdapter = windowLayout!!.adapter as DWTabViewPagerAdapter
                val index = adapter.viewList.indexOf(window)

                // We need to have at least 1 window...
                // so only destroy secondary windows
                if(index > 0) {
                    val newindex = index - 1
                    val newwindow = adapter.viewList[newindex]

                    // Make sure the previous window is visible...
                    // not sure if we should search the list for a visible
                    // window or force it visible.  Forcing visible for now.
                    if(newwindow.visibility != View.VISIBLE) {
                        newwindow.visibility = View.VISIBLE
                    }
                    // Switch to the previous window
                    windowSwitchWindow(newindex)

                    // Update our window list
                    adapter.viewList.removeAt(index)
                    windowTitles.removeAt(index)
                    windowMenuBars.removeAt(index)
                    windowStyles.removeAt(index)
                    windowDefault.removeAt(index)

                    retval = 0 // DW_ERROR_NONE
                } else {
                    // If we are removing an individual widget,
                    // find the parent layout and remove it.
                    if(window.parent is ViewGroup) {
                        val group = window.parent as ViewGroup

                        group.removeView(window)
                        retval = 0 // DW_ERROR_NONE
                    }
                }
            }
        }
        return retval
    }

    fun clipboardGetText(): String {
        val cm: ClipboardManager = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
        val clipdata = cm.primaryClip

        if (clipdata != null && clipdata.itemCount > 0) {
            return clipdata.getItemAt(0).coerceToText(this).toString()
        }
        return ""
    }

    fun clipboardSetText(text: String) {
        val cm: ClipboardManager = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
        val clipdata = ClipData.newPlainText("text", text)

        cm.setPrimaryClip(clipdata)
    }

    fun boxNew(type: Int, pad: Int): LinearLayout? {
        var box: LinearLayout? = null
        waitOnUiThread {
            box = LinearLayout(this)
            val dataArrayMap = SimpleArrayMap<String, Long>()

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
            val dataArrayMap = SimpleArrayMap<String, Long>()

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

    // Update the layoutParams of a box after a change
    private fun boxUpdate(box: LinearLayout)
    {
        val parent = box.parent

        if(parent is LinearLayout) {
            val params = box.layoutParams as LinearLayout.LayoutParams

            if(parent.orientation == LinearLayout.VERTICAL) {
                if(params.height == 0) {
                    box.measure(0, 0)
                    val calch = box.measuredHeight

                    if(calch > 0) {
                        params.weight = calch.toFloat()
                    }
                }
            } else {
                if(params.width == 0) {
                    box.measure(0, 0)
                    val calcw = box.measuredWidth

                    if(calcw > 0) {
                        params.weight = calcw.toFloat()
                    }
                }
            }
        }
    }

    fun boxPack(
        boxview: View,
        packitem: View?,
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
            var item: View? = packitem

            // We can't pack nothing, so create an empty placeholder to pack
            if(item == null) {
                item = Placeholder(this)
                item.emptyVisibility = View.VISIBLE
            }

            // Handle scrollboxes by pulling the LinearLayout
            // out of the ScrollView to pack into
            if (boxview is LinearLayout) {
                box = boxview
            } else if (boxview is ScrollView) {
                val sv: ScrollView = boxview

                if (sv.getChildAt(0) is LinearLayout) {
                    box = sv.getChildAt(0) as LinearLayout
                }
            }

            if (box != null) {
                var weight = 1F

                // If it is a box, match parent based on direction
                if ((item is LinearLayout) || (item is ScrollView)) {
                    item.measure(0, 0)
                    if (box.orientation == LinearLayout.VERTICAL) {
                        if (hsize != 0) {
                            w = LinearLayout.LayoutParams.MATCH_PARENT
                        }
                        if (vsize != 0) {
                            val calch = item.measuredHeight

                            if(calch > 0) {
                                weight = calch.toFloat()
                            } else {
                                weight = 1F
                            }
                            h = 0
                        }
                    } else {
                        if (vsize != 0) {
                            h = LinearLayout.LayoutParams.MATCH_PARENT
                        }
                        if (hsize != 0) {
                            val calcw = item.measuredWidth

                            if(calcw > 0) {
                                weight = calcw.toFloat()
                            } else {
                                weight = 1F
                            }
                            w = 0
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
                        if (box.orientation == LinearLayout.VERTICAL) {
                            w = LinearLayout.LayoutParams.MATCH_PARENT
                        } else {
                            if (width > 0) {
                                weight = width.toFloat()
                            } else if (width == -1) {
                                val newwidth = item.getMeasuredWidth()

                                if (newwidth > 0) {
                                    weight = newwidth.toFloat()
                                }
                            }
                        }
                    }
                    if(vsize == 0) {
                        if (height > 0) {
                            h = height
                        }
                    } else {
                        if (box.orientation == LinearLayout.HORIZONTAL) {
                            h = LinearLayout.LayoutParams.MATCH_PARENT
                        } else {
                            if (height > 0) {
                                weight = height.toFloat()
                            } else if (height == -1) {
                                val newheight = item.getMeasuredHeight()

                                if (newheight > 0) {
                                    weight = newheight.toFloat()
                                }
                            }
                        }
                    }
                }

                val params: LinearLayout.LayoutParams = LinearLayout.LayoutParams(w, h)

                // Handle expandable items by giving them a weight...
                // in the direction of the box.
                if (box.orientation == LinearLayout.VERTICAL) {
                    if (vsize != 0 && weight > 0F) {
                        params.weight = weight
                        params.height = 0
                    }
                } else {
                    if (hsize != 0 && weight > 0F) {
                        params.weight = weight
                        params.width = 0
                    }
                }
                // Gravity needs to match the expandable settings
                val grav: Int = Gravity.CLIP_HORIZONTAL or Gravity.CLIP_VERTICAL
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
                boxUpdate(box)
            }
        }
    }

    fun boxUnpack(item: View) {
        waitOnUiThread {
            val box: LinearLayout = item.parent as LinearLayout
            box.removeView(item)
            boxUpdate(box)
        }
    }

    fun boxUnpackAtIndex(box: LinearLayout, index: Int): View? {
        var item: View? = null

        waitOnUiThread {
            item = box.getChildAt(index)
            box.removeView(item)
            boxUpdate(box)
        }
        return item
    }

    fun buttonNew(text: String, cid: Int): Button? {
        var button: Button? = null
        waitOnUiThread {
            button = Button(this)
            val dataArrayMap = SimpleArrayMap<String, Long>()

            button!!.tag = dataArrayMap
            button!!.isAllCaps = false
            button!!.text = text
            button!!.id = cid
            button!!.setOnClickListener {
                lastClickView = button!!
                eventHandlerSimple(button!!, DWEvent.CLICKED)
            }
        }
        return button
    }

    fun bitmapButtonNew(text: String, resid: Int): ImageButton? {
        var button: ImageButton? = null
        waitOnUiThread {
            button = ImageButton(this)
            val dataArrayMap = SimpleArrayMap<String, Long>()
            var filename: String? = null

            button!!.tag = dataArrayMap
            button!!.id = resid
            button!!.setImageResource(resid)
            button!!.setOnClickListener {
                lastClickView = button!!
                eventHandlerSimple(button!!, DWEvent.CLICKED)
            }

            if(resid > 0 && resid < 65536) {
                filename = resid.toString()
            }

            if(filename != null) {
                for (ext in DWImageExts) {
                    // Try to load the image, and protect against exceptions
                    try {
                        val f = this.assets.open(filename + ext)
                        val b = BitmapFactory.decodeStream(f)

                        if (b != null) {
                            button!!.setImageBitmap(b)
                            break
                        }
                    } catch (e: IOException) {
                    }
                }
            }
        }
        return button
    }

    fun bitmapButtonNewFromFile(text: String, cid: Int, filename: String): ImageButton? {
        var button: ImageButton? = null
        waitOnUiThread {
            button = ImageButton(this)
            val dataArrayMap = SimpleArrayMap<String, Long>()

            button!!.tag = dataArrayMap
            button!!.id = cid
            button!!.setOnClickListener {
                lastClickView = button!!
                eventHandlerSimple(button!!, DWEvent.CLICKED)
            }

            for (ext in DWImageExts) {
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
            val dataArrayMap = SimpleArrayMap<String, Long>()
            val b = BitmapFactory.decodeByteArray(data,0, length)

            button!!.tag = dataArrayMap
            button!!.id = cid
            button!!.setOnClickListener {
                lastClickView = button!!
                eventHandlerSimple(button!!, DWEvent.CLICKED)
            }
            button!!.setImageBitmap(b)
        }
        return button
    }

    fun entryfieldNew(text: String, cid: Int, password: Int): EditText? {
        var entryfield: EditText? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()
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
            val dataArrayMap = SimpleArrayMap<String, Long>()
            radiobutton = RadioButton(this)

            radiobutton!!.tag = dataArrayMap
            radiobutton!!.id = cid
            radiobutton!!.text = text
            radiobutton!!.setOnClickListener {
                lastClickView = radiobutton!!
                eventHandlerSimple(radiobutton!!, DWEvent.CLICKED)
            }
        }
        return radiobutton
    }

    fun checkboxNew(text: String, cid: Int): CheckBox? {
        var checkbox: CheckBox? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()

            checkbox = CheckBox(this)
            checkbox!!.tag = dataArrayMap
            checkbox!!.id = cid
            checkbox!!.text = text
            checkbox!!.setOnClickListener {
                lastClickView = checkbox!!
                eventHandlerSimple(checkbox!!, DWEvent.CLICKED)
            }
        }
        return checkbox
    }

    fun checkOrRadioSetChecked(control: View, state: Int)
    {
        waitOnUiThread {
            if (control is CheckBox) {
                val checkbox: CheckBox = control
                checkbox.isChecked = state != 0
            } else if (control is RadioButton) {
                val radiobutton: RadioButton = control
                radiobutton.isChecked = state != 0
            }
        }
    }

    fun checkOrRadioGetChecked(control: View): Boolean
    {
        var retval = false

        waitOnUiThread {
            if (control is CheckBox) {
                val checkbox: CheckBox = control
                retval = checkbox.isChecked
            } else if (control is RadioButton) {
                val radiobutton: RadioButton = control
                retval = radiobutton.isChecked
            }
        }
        return retval
    }

    fun textNew(text: String, cid: Int, status: Int): TextView? {
        var textview: TextView? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()

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
            val dataArrayMap = SimpleArrayMap<String, Long>()

            mle = EditText(this)
            mle!!.tag = dataArrayMap
            mle!!.id = cid
            mle!!.isSingleLine = false
            mle!!.maxLines = Integer.MAX_VALUE
            mle!!.imeOptions = EditorInfo.IME_FLAG_NO_ENTER_ACTION
            mle!!.inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_MULTI_LINE)
            mle!!.isVerticalScrollBarEnabled = true
            mle!!.scrollBarStyle = View.SCROLLBARS_INSIDE_INSET
            mle!!.setHorizontallyScrolling(true)
            mle!!.isHorizontalScrollBarEnabled = true
            mle!!.gravity = Gravity.TOP or Gravity.LEFT
        }
        return mle
    }

    fun mleSetWordWrap(mle: EditText, state: Int)
    {
        waitOnUiThread {
            if (state != 0) {
                mle.setHorizontallyScrolling(false)
                mle.isHorizontalScrollBarEnabled = false
            } else {
                mle.setHorizontallyScrolling(true)
                mle.isHorizontalScrollBarEnabled = true
            }
        }
    }

    fun mleSetEditable(mle: EditText, state: Int)
    {
        waitOnUiThread {
            mle.isFocusable = state != 0
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
            val w: Int = RelativeLayout.LayoutParams.MATCH_PARENT
            val h: Int = RelativeLayout.LayoutParams.WRAP_CONTENT
            val dataArrayMap = SimpleArrayMap<String, Long>()

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
                    eventHandlerNotebook(notebook!!, DWEvent.SWITCH_PAGE, adapter.pageList[tab.position])
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
                val adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
                val tab = tabs.newTab()

                // Increment our page ID... making sure no duplicates exist
                do {
                    adapter.currentPageID += 1
                } while (adapter.currentPageID == 0L || adapter.pageList.contains(adapter.currentPageID))
                pageID = adapter.currentPageID
                // Temporarily add a black tab with an empty layout/box
                val placeholder = LinearLayout(this)
                placeholder.layoutParams = LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.MATCH_PARENT
                )
                if (front != 0) {
                    adapter.viewList.add(0, placeholder)
                    adapter.pageList.add(0, pageID)
                    tabs.addTab(tab, 0)
                } else {
                    adapter.viewList.add(placeholder)
                    adapter.pageList.add(pageID)
                    tabs.addTab(tab)
                }
            }
        }
        return pageID
    }

    fun notebookCapsOff(view: View?) {
        if (view !is ViewGroup) {
            return
        }
        for (i in 0 until view.childCount) {
            val child = view.getChildAt(i)
            if (child is TextView) {
                child.isAllCaps = false
            } else {
                notebookCapsOff(child)
            }
        }
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
                val adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
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

                notebookCapsOff(tabs)
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
                val adapter: DWTabViewPagerAdapter = pager.adapter as DWTabViewPagerAdapter
                val index = adapter.pageList.indexOf(pageID)

                // Make sure the box is MATCH_PARENT
                box.layoutParams = LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.MATCH_PARENT
                )

                adapter.viewList[index] = box
            }
        }
    }

    fun notebookPageGet(notebook: RelativeLayout): Long
    {
        var retval = 0L

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

    fun splitBarNew(type: Int, topleft: View?, bottomright: View?, cid: Int): ConstraintLayout?
    {
        var splitbar: ConstraintLayout? = null

        waitOnUiThread {
            splitbar = ConstraintLayout(this)
            if(splitbar != null) {
                val constraintSet = ConstraintSet()
                val dataArrayMap = SimpleArrayMap<String, Long>()

                constraintSet.clone(splitbar)

                splitbar!!.tag = dataArrayMap
                splitbar!!.id = cid

                // Add the special data to the array map
                dataArrayMap.put("_dw_type", type.toLong())
                dataArrayMap.put("_dw_percent", 50000000L)

                // Place the top/left item
                if(topleft != null) {
                    if(topleft.id < 1) {
                        topleft.id = View.generateViewId()
                    }
                    splitbar!!.addView(topleft)
                    constraintSet.connect(
                        topleft.id,
                        ConstraintLayout.LayoutParams.TOP,
                        ConstraintLayout.LayoutParams.PARENT_ID,
                        ConstraintLayout.LayoutParams.TOP
                    )
                    constraintSet.connect(
                        topleft.id,
                        ConstraintLayout.LayoutParams.LEFT,
                        ConstraintLayout.LayoutParams.PARENT_ID,
                        ConstraintLayout.LayoutParams.LEFT
                    )

                    if (type == 0) {
                        // Horizontal
                        constraintSet.connect(
                            topleft.id,
                            ConstraintLayout.LayoutParams.BOTTOM,
                            ConstraintLayout.LayoutParams.PARENT_ID,
                            ConstraintLayout.LayoutParams.BOTTOM
                        )
                        constraintSet.constrainPercentWidth(topleft.id, 0.5F)
                    } else {
                        // Vertical
                        constraintSet.connect(
                            topleft.id,
                            ConstraintLayout.LayoutParams.RIGHT,
                            ConstraintLayout.LayoutParams.PARENT_ID,
                            ConstraintLayout.LayoutParams.RIGHT
                        )
                        constraintSet.constrainPercentHeight(topleft.id, 0.5F)
                    }
                }

                // Place the bottom/right item
                if(bottomright != null) {
                    if (bottomright.id < 1) {
                        bottomright.id = View.generateViewId()
                    }
                    splitbar!!.addView(bottomright)
                    constraintSet.connect(
                        bottomright.id,
                        ConstraintLayout.LayoutParams.BOTTOM,
                        ConstraintLayout.LayoutParams.PARENT_ID,
                        ConstraintLayout.LayoutParams.BOTTOM
                    )
                    constraintSet.connect(
                        bottomright.id,
                        ConstraintLayout.LayoutParams.RIGHT,
                        ConstraintLayout.LayoutParams.PARENT_ID,
                        ConstraintLayout.LayoutParams.RIGHT
                    )

                    if (type == 0) {
                        // Horizontal
                        constraintSet.connect(
                            bottomright.id,
                            ConstraintLayout.LayoutParams.TOP,
                            ConstraintLayout.LayoutParams.PARENT_ID,
                            ConstraintLayout.LayoutParams.TOP
                        )
                        constraintSet.constrainPercentWidth(bottomright.id, 0.5F)
                    } else {
                        // Vertical
                        constraintSet.connect(
                            bottomright.id,
                            ConstraintLayout.LayoutParams.LEFT,
                            ConstraintLayout.LayoutParams.PARENT_ID,
                            ConstraintLayout.LayoutParams.LEFT
                        )
                        constraintSet.constrainPercentHeight(bottomright.id, 0.5F)
                    }
                }

                // finally, apply the constraint set to layout
                constraintSet.applyTo(splitbar)
            }
        }
        return splitbar
    }

    fun splitBarGet(splitbar: ConstraintLayout): Float {
        var position: Float = 50.0F

        waitOnUiThread {
            val dataArrayMap: SimpleArrayMap<String, Long> = splitbar.tag as SimpleArrayMap<String, Long>
            var percent: Long = 50000000L

            if(dataArrayMap.containsKey("_dw_percent")) {
                percent = dataArrayMap.get("_dw_percent")!!
            }

            position = percent.toFloat() / 1000000.0F
        }
        return position
    }

    fun splitBarSet(splitbar: ConstraintLayout, position: Float) {
        waitOnUiThread {
            val dataArrayMap: SimpleArrayMap<String, Long> = splitbar.tag as SimpleArrayMap<String, Long>
            var percent: Float = position * 1000000.0F

            if(percent > 0F) {
                val topleft: View? = splitbar.getChildAt(0)
                val bottomright: View? = splitbar.getChildAt(1)
                val constraintSet = ConstraintSet()
                var type: Long = 0L

                if (dataArrayMap.containsKey("_dw_type")) {
                    type = dataArrayMap.get("_dw_type")!!
                }
                dataArrayMap.put("_dw_percent", percent.toLong())

                constraintSet.clone(splitbar)
                if (topleft != null) {
                    if (type == 1L) {
                        constraintSet.constrainPercentHeight(topleft.id, position / 100.0F)
                    } else {
                        constraintSet.constrainPercentWidth(topleft.id, position / 100.0F)
                    }
                }
                if (bottomright != null) {
                    val altper: Float = (100.0F - position) / 100.0F
                    if (type == 1L) {
                        constraintSet.constrainPercentHeight(bottomright.id, altper)
                    } else {
                        constraintSet.constrainPercentWidth(bottomright.id, altper)
                    }
                }
                constraintSet.applyTo(splitbar)
            }
        }
    }

    fun sliderNew(vertical: Int, increments: Int, cid: Int): SeekBar?
    {
        var slider: SeekBar? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()

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
                    eventHandlerInt(slider as View, DWEvent.VALUE_CHANGED, slider!!.progress, 0, 0, 0)
                }
            })
        }
        return slider
    }

    fun percentNew(cid: Int): ProgressBar?
    {
        var percent: ProgressBar? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()

            percent = ProgressBar(this,null, R.attr.progressBarStyleHorizontal)
            percent!!.tag = dataArrayMap
            percent!!.id = cid
            percent!!.max = 100
        }
        return percent
    }

    fun percentGetPos(percent: ProgressBar): Int
    {
        var retval = 0

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
            val dataArrayMap = SimpleArrayMap<String, Long>()

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
                eventHandlerHTMLResult(html, DWEvent.HTML_RESULT, value, data)
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
            val dataArrayMap = SimpleArrayMap<String, Long>()
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
            val dataArrayMap = SimpleArrayMap<String, Long>()

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
            val dataArrayMap = SimpleArrayMap<String, Long>()
            val adapter = DWContainerAdapter(this)

            cont = ListView(this)
            cont!!.tag = dataArrayMap
            cont!!.id = cid
            cont!!.adapter = adapter
            if(multi != 0) {
                cont!!.choiceMode = ListView.CHOICE_MODE_MULTIPLE
            }
            cont!!.setOnItemClickListener { parent, view, position, id ->
                val title = adapter.model.getRowTitle(position)
                val data = adapter.model.getRowData(position)
                val now = System.currentTimeMillis()

                view.isSelected = !view.isSelected
                adapter.selectedItem = position
                lastClickView = cont!!
                // If we are single select or we got a double tap...
                // Generate an ENTER event
                if(cont!!.choiceMode != ListView.CHOICE_MODE_MULTIPLE ||
                    (position == adapter.lastClickRow &&
                    (now - adapter.lastClick) < ViewConfiguration.getDoubleTapTimeout())) {
                    eventHandlerContainer(cont!!, DWEvent.ITEM_ENTER, title, 0, 0, data)
                } else {
                    // If we are mutiple select, generate a SELECT event
                    eventHandlerContainer(cont!!, DWEvent.ITEM_SELECT, title, 0, 0, data)
                }
                adapter.lastClick = now
                adapter.lastClickRow = position
            }
            cont!!.setOnContextClickListener {
                if(adapter.selectedItem > -1 && adapter.selectedItem < adapter.model.numberOfRows()) {
                    val title = adapter.model.getRowTitle(adapter.selectedItem)
                    val data = adapter.model.getRowData(adapter.selectedItem)

                    lastClickView = cont!!
                    eventHandlerContainer(cont!!, DWEvent.ITEM_CONTEXT, title, 0, 0, data)
                }
                true
            }
            cont!!.setOnItemLongClickListener { parent, view, position, id ->
                val title = adapter.model.getRowTitle(position)
                val data = adapter.model.getRowData(position)

                lastClickView = cont!!
                eventHandlerContainer(cont!!, DWEvent.ITEM_CONTEXT, title, 0, 0, data)
                true
            }
        }
        return cont
    }

    fun containerSetStripe(cont: ListView, oddcolor: Long, evencolor: Long)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            if(oddcolor == -1L) {
                adapter.oddColor = null
            } else if(evencolor == -2L) {
                if(darkMode == 1) {
                    adapter.oddColor = Color.rgb(100, 100, 100)
                } else {
                    adapter.oddColor = Color.rgb(230, 230, 230)
                }
            } else {
                adapter.oddColor = colorFromDW(oddcolor)
            }
            if(evencolor == -1L || evencolor == -2L) {
                adapter.evenColor = null
            } else {
                adapter.evenColor = colorFromDW(evencolor)
            }
        }
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

                adapter.model.querypos = position
                retval = adapter.model.getRowTitle(position)
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

                        if (adapter.model.querypos == position && (i + 1) < checked.size()) {
                            val newpos = checked.keyAt(i + 1)

                            adapter.model.querypos = newpos
                            retval = adapter.model.getRowTitle(newpos)
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

                adapter.model.querypos = position
                retval = adapter.model.getRowData(position)
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

                        if (adapter.model.querypos == position && (i + 1) < checked.size()) {
                            val newpos = checked.keyAt(i + 1)

                            adapter.model.querypos = newpos
                            retval = adapter.model.getRowData(newpos)
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
        var type = 0

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
            val dataArrayMap = SimpleArrayMap<String, Long>()

            listbox = DWListBox(this)
            listbox!!.tag = dataArrayMap
            listbox!!.id = cid
            if(multi != 0) {
                listbox!!.choiceMode = ListView.CHOICE_MODE_MULTIPLE
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
        var retval = 0

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
                        listbox.setItemChecked(index, true)
                    } else {
                        listbox.setItemChecked(index, false)
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
            val dataArrayMap = SimpleArrayMap<String, Long>()

            calendar = CalendarView(this)
            calendar!!.tag = dataArrayMap
            calendar!!.id = cid
            calendar!!.setOnDateChangeListener { calendar, year, month, day ->
                val c: Calendar = Calendar.getInstance()
                c.set(year, month, day)
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
            val dataArrayMap = SimpleArrayMap<String, Long>()

            imageview = ImageView(this)
            imageview!!.tag = dataArrayMap
            imageview!!.id = cid
        }

        return imageview
    }

    fun windowSetBitmap(window: View, resID: Int, file: String?)
    {
        waitOnUiThread {
            var filename: String? = file

            if(resID > 0 && resID < 65536) {
                filename = resID.toString()
            } else if(resID != 0) {
                if (window is ImageButton) {
                    val button = window

                    button.setImageResource(resID)
                } else if (window is ImageView) {
                    val imageview = window

                    imageview.setImageResource(resID)
                }
            }
            if(filename != null) {
                for (ext in DWImageExts) {
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
            }
            if(data != null) {
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

    fun iconNew(file: String?, data: ByteArray?, length: Int, resID: Int): Drawable?
    {
        var icon: Drawable? = null

        waitOnUiThread {
            var filename: String? = null

            // Handle Dynamic Windows resource IDs
            if(resID > 0 && resID < 65536) {
                filename = resID.toString()
            // Handle Android resource IDs
            } else if(resID != 0) {
                try {
                    icon = ResourcesCompat.getDrawable(resources, resID, null)
                } catch (e: Resources.NotFoundException) {
                }
            // Handle bitmap data
            } else if(data != null) {
                icon = BitmapDrawable(resources, BitmapFactory.decodeByteArray(data, 0, length))
            } else {
                filename = file
            }
            // Handle filename or DW resource IDs
            // these will be located in the assets folder
            if(filename != null) {
                for (ext in DWImageExts) {
                    // Try to load the image, and protect against exceptions
                    try {
                        val f = this.assets.open(filename + ext)
                        icon = Drawable.createFromStream(f, null)
                    } catch (e: IOException) {
                    }
                    if (icon != null) {
                        break
                    }
                }
            }
        }
        return icon
    }

    fun pixmapNew(width: Int, height: Int, file: String?, data: ByteArray?, length: Int, resID: Int): Bitmap?
    {
        var pixmap: Bitmap? = null

        waitOnUiThread {
            var filename: String? = null

            if(width > 0 && height > 0) {
                pixmap = Bitmap.createBitmap(null, width, height, Bitmap.Config.ARGB_8888)
            } else if(resID > 0 && resID < 65536) {
                filename = resID.toString()
            } else if(resID != 0) {
                pixmap = BitmapFactory.decodeResource(resources, resID)
            } else if(data != null) {
                pixmap = BitmapFactory.decodeByteArray(data, 0, length)
            } else {
                filename = file
            }
            if(filename != null) {
                for (ext in DWImageExts) {
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
            val dataArrayMap = SimpleArrayMap<String, Long>()

            render = DWRender(this)
            render!!.tag = dataArrayMap
            render!!.id = cid
            render!!.setOnTouchListener(object : View.OnTouchListener {
                @SuppressLint("ClickableViewAccessibility")
                override fun onTouch(v: View, event: MotionEvent): Boolean {
                    when (event.action) {
                        MotionEvent.ACTION_DOWN -> {
                            render!!.evx = event.x
                            render!!.evy = event.y
                            // Event will be generated in the onClickListener or
                            // onLongClickListener below, just save the location
                        }
                        MotionEvent.ACTION_UP -> {
                            render!!.evx = event.x
                            render!!.evy = event.y
                            lastClickView = render!!
                            eventHandlerInt(render!!, DWEvent.BUTTON_RELEASE, event.x.toInt(), event.y.toInt(), render!!.button, 0)
                        }
                        MotionEvent.ACTION_MOVE -> {
                            render!!.evx = event.x
                            render!!.evy = event.y
                            lastClickView = render!!
                            eventHandlerInt(render!!, DWEvent.MOTION_NOTIFY, event.x.toInt(), event.y.toInt(), 1, 0)
                        }
                    }
                    return false
                }
            })
            render!!.setOnLongClickListener{
                // Long click functions as button 2
                render!!.button = 2
                lastClickView = render!!
                eventHandlerInt(render!!, DWEvent.BUTTON_PRESS, render!!.evx.toInt(), render!!.evy.toInt(), 2, 0)
                true
            }
            render!!.setOnClickListener{
                // Normal click functions as button 1
                render!!.button = 1
                lastClickView = render!!
                eventHandlerInt(render!!, DWEvent.BUTTON_PRESS, render!!.evx.toInt(), render!!.evy.toInt(), 1, 0)
            }
            render!!.setOnKeyListener(View.OnKeyListener { v, keyCode, event ->
                if (event.action == KeyEvent.ACTION_DOWN) {
                    eventHandlerKey(render!!, DWEvent.KEY_PRESS, keyCode, event.unicodeChar, event.modifiers, event.characters)
                }
                false
            })
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
        val src = Rect(srcx, srcy, srcx + srcw, srcy + srch)
        var retval = 1

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

    fun drawPoint(render: DWRender?, bitmap: Bitmap?, x: Int, y: Int, fgColor: Long, bgColor: Long)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                colorsSet(fgColor, bgColor)
                canvas.drawPoint(x.toFloat(), y.toFloat(), Paint())
            }
        }
    }

    fun drawLine(render: DWRender?, bitmap: Bitmap?, x1: Int, y1: Int, x2: Int, y2: Int, fgColor: Long, bgColor: Long)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                colorsSet(fgColor, bgColor)
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
            val rect = Rect()

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
                    val secondary: DWRender = window

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

    fun drawText(render: DWRender?, bitmap: Bitmap?, x: Int, y: Int, text:String, typeface: Typeface?,
                 fontsize: Int, window: View?, fgColor: Long, bgColor: Long)
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
                    val secondary: DWRender = window

                    if(secondary.typeface != null) {
                        paint.typeface = secondary.typeface
                        if(secondary.fontsize != null && secondary.fontsize!! > 0F) {
                            paint.textSize = secondary.fontsize!!
                        }
                    }
                }
            }

            if(canvas != null) {
                colorsSet(fgColor, bgColor)
                // Save the old color for later...
                val rect = Rect()
                paint.flags = 0
                paint.style = Paint.Style.FILL_AND_STROKE
                paint.textAlign = Paint.Align.LEFT
                paint.getTextBounds(text, 0, text.length, rect)
                val textheight = rect.bottom - rect.top
                if(bgcolor != null) {
                    val oldcolor = paint.color
                    // Prepare to draw the background rect
                    paint.color = bgcolor as Int
                    rect.top += y + textheight
                    rect.bottom += y + textheight
                    rect.left += x
                    rect.right += x
                    canvas.drawRect(rect, paint)
                    // Restore the color and prepare to draw text
                    paint.color = oldcolor
                }
                paint.style = Paint.Style.STROKE
                canvas.drawText(text, x.toFloat(), y.toFloat() + textheight.toFloat(), paint)
            }
        }
    }

    fun drawRect(render: DWRender?, bitmap: Bitmap?, x: Int, y: Int, width: Int, height: Int, fgColor: Long, bgColor: Long)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                colorsSet(fgColor, bgColor)
                paint.flags = 0
                paint.style = Paint.Style.FILL_AND_STROKE
                canvas.drawRect(x.toFloat(), y.toFloat(), x.toFloat() + width.toFloat(), y.toFloat() + height.toFloat(), paint)
            }
        }
    }

    fun drawPolygon(render: DWRender?, bitmap: Bitmap?, flags: Int, npoints: Int,
                    x: IntArray, y: IntArray, fgColor: Long, bgColor: Long)
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
                colorsSet(fgColor, bgColor)
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
                x1: Int, y1: Int, x2: Int, y2: Int, fgColor: Long, bgColor: Long)
    {
        waitOnUiThread {
            var canvas: Canvas? = null

            if(render != null) {
                canvas = render.cachedCanvas
            } else if(bitmap != null) {
                canvas = Canvas(bitmap)
            }

            if(canvas != null) {
                colorsSet(fgColor, bgColor)

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
                    var left: Float = x1.toFloat()
                    var top: Float = y1.toFloat()
                    var right: Float = x2.toFloat()
                    var bottom: Float = y2.toFloat()

                    if(x2 < x1) {
                        left = x2.toFloat()
                        right = x1.toFloat()
                    }
                    if(y2 < y1) {
                        top = y2.toFloat()
                        bottom = y1.toFloat()
                    }

                    canvas.drawOval(left, top, right, bottom, paint)
                } else {
                    var a1: Double = Math.atan2((y1 - yorigin).toDouble(), (x1 - xorigin).toDouble())
                    var a2: Double = Math.atan2((y2 - yorigin).toDouble(), (x2 - xorigin).toDouble())
                    val dx = (xorigin - x1).toDouble()
                    val dy = (yorigin - y1).toDouble()
                    val r: Double = Math.sqrt(dx * dx + dy * dy)
                    val left = (xorigin-r).toFloat()
                    val top = (yorigin-r).toFloat()
                    val rect = RectF(left, top, (left + (r*2)).toFloat(), (top + (r*2)).toFloat())

                    // Convert to degrees
                    a1 *= 180.0 / Math.PI
                    a2 *= 180.0 / Math.PI
                    val sweep = Math.abs(a1 - a2)

                    canvas.drawArc(rect, a1.toFloat(), sweep.toFloat(), false, paint)
                }
            }
        }
    }

    fun colorFromDW(color: Long): Int
    {
        val red: Int = (color and 0x000000FF).toInt()
        val green: Int = ((color and 0x0000FF00) shr 8).toInt()
        val blue: Int = ((color and 0x00FF0000) shr 16).toInt()

        return Color.rgb(red, green, blue)
    }

    fun colorsSet(fgColor: Long, bgColor: Long)
    {
        waitOnUiThread {
            paint.color = colorFromDW(fgColor)
            if(bgColor != -1L) {
                this.bgcolor = colorFromDW(bgColor)
            } else {
                this.bgcolor = null
            }
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

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if(requestCode == 100) {
            fileLock.lock()
            if(resultCode == Activity.RESULT_OK) {
                fileURI = data!!.data
            } else {
                fileURI = null
            }
            fileCond.signal()
            fileLock.unlock()
        }
    }

    fun fileBrowseNew(title: String, defpath: String?, ext: String?, flags: Int): String?
    {
        var retval: String? = null

        // This can't be called from the main thread
        if(Looper.getMainLooper() != Looper.myLooper()) {
            fileLock.lock()
            waitOnUiThread {
                val fileintent = Intent(Intent.ACTION_GET_CONTENT)
                fileintent.type = "text/plain"
                fileintent.addCategory(Intent.CATEGORY_OPENABLE)
                startActivityForResult(fileintent, 100)
            }

            // Wait until the intent finishes.
            fileCond.await()
            fileLock.unlock()

            if(fileURI != null) {
                retval = getUriRealPath(this, fileURI)
            }
        }
        return retval
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
        var retval = 0

        waitOnUiThread {
            // make a text input dialog and show it
            val alert = AlertDialog.Builder(this)

            alert.setTitle(title)
            alert.setMessage(body)
            if ((flags and (1 shl 3)) != 0) {
                alert.setPositiveButton("Yes"
                )
                //R.string.yes,
                { _: DialogInterface, _: Int ->
                    retval = 1
                    throw java.lang.RuntimeException()
                }
            }
            if ((flags and ((1 shl 1) or (1 shl 2))) != 0) {
                alert.setNegativeButton(
                    R.string.ok
                ) { _: DialogInterface, _: Int ->
                    retval = 0
                    throw java.lang.RuntimeException()
                }
            }
            if ((flags and ((1 shl 3) or (1 shl 4))) != 0) {
                alert.setNegativeButton("No"
                )
                //R.string.no,
                { _: DialogInterface, _: Int ->
                    retval = 0
                    throw java.lang.RuntimeException()
                }
            }
            if ((flags and ((1 shl 2) or (1 shl 4))) != 0) {
                alert.setNeutralButton(
                    R.string.cancel
                ) { _: DialogInterface, _: Int ->
                    retval = 2
                    throw java.lang.RuntimeException()
                }
            }
            alert.setCancelable(false)
            alert.show()

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

            // Waiting for Idle to check for sleep expiration
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
     * This method will parse out the real local file path from the file content URI.
     */
    private fun getUriRealPath(ctx: Context?, uri: Uri?): String? {
        var ret: String? = ""
        if (ctx != null && uri != null) {
            if (isContentUri(uri)) {
                if (isGooglePhotoDoc(uri.authority)) {
                    ret = uri.lastPathSegment
                } else {
                    ret = getImageRealPath(contentResolver, uri, null)
                }
            } else if (isFileUri(uri)) {
                ret = uri.path
            } else if (isDocumentUri(ctx, uri)) {

                // Get uri related document id.
                val documentId = DocumentsContract.getDocumentId(uri)

                // Get uri authority.
                val uriAuthority: String? = uri.authority
                if (isMediaDoc(uriAuthority)) {
                    val idArr = documentId.split(":").toTypedArray()
                    if (idArr.size == 2) {
                        // First item is document type.
                        val docType = idArr[0]

                        // Second item is document real id.
                        val realDocId = idArr[1]

                        // Get content uri by document type.
                        var mediaContentUri: Uri? = MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                        if ("image" == docType) {
                            mediaContentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                        } else if ("video" == docType) {
                            mediaContentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                        } else if ("audio" == docType) {
                            mediaContentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
                        }

                        // Get where clause with real document id.
                        val whereClause = MediaStore.Images.Media._ID + " = " + realDocId
                        ret = getImageRealPath(contentResolver, mediaContentUri, whereClause)
                    }
                } else if (isDownloadDoc(uriAuthority)) {
                    // Build download uri.
                    val downloadUri: Uri = Uri.parse("content://downloads/public_downloads")

                    // Append download document id at uri end.
                    val downloadUriAppendId: Uri =
                        ContentUris.withAppendedId(downloadUri, java.lang.Long.valueOf(documentId))
                    ret = getImageRealPath(contentResolver, downloadUriAppendId, null)
                } else if (isExternalStoreDoc(uriAuthority)) {
                    val idArr = documentId.split(":").toTypedArray()
                    if (idArr.size == 2) {
                        val type = idArr[0]
                        val realDocId = idArr[1]
                        if ("primary".equals(type, ignoreCase = true)) {
                            ret = Environment.getExternalStorageDirectory()
                                .toString() + "/" + realDocId
                        }
                    }
                }
            }
        }
        return ret
    }

    /* Check whether this uri represent a document or not. */
    private fun isDocumentUri(ctx: Context?, uri: Uri?): Boolean {
        var ret = false
        if (ctx != null && uri != null) {
            ret = DocumentsContract.isDocumentUri(ctx, uri)
        }
        return ret
    }

    /* Check whether this uri is a content uri or not.
     *  content uri like content://media/external/images/media/1302716
     */
    private fun isContentUri(uri: Uri?): Boolean {
        var ret = false
        if (uri != null) {
            val uriSchema: String? = uri.scheme
            if ("content".equals(uriSchema, ignoreCase = true)) {
                ret = true
            }
        }
        return ret
    }

    /* Check whether this uri is a file uri or not.
     *  file uri like file:///storage/41B7-12F1/DCIM/Camera/IMG_20180211_095139.jpg
     */
    private fun isFileUri(uri: Uri?): Boolean {
        var ret = false
        if (uri != null) {
            val uriSchema: String? = uri.scheme
            if ("file".equals(uriSchema, ignoreCase = true)) {
                ret = true
            }
        }
        return ret
    }


    /* Check whether this document is provided by ExternalStorageProvider. Return true means the file is saved in external storage. */
    private fun isExternalStoreDoc(uriAuthority: String?): Boolean {
        var ret = false
        if (uriAuthority != null && "com.android.externalstorage.documents" == uriAuthority) {
            ret = true
        }
        return ret
    }

    /* Check whether this document is provided by DownloadsProvider. return true means this file is a downloaed file. */
    private fun isDownloadDoc(uriAuthority: String?): Boolean {
        var ret = false
        if (uriAuthority != null && "com.android.providers.downloads.documents" == uriAuthority) {
            ret = true
        }
        return ret
    }

    /*
     * Check if MediaProvider provide this document, if true means this image is created in android media app.
     */
    private fun isMediaDoc(uriAuthority: String?): Boolean {
        var ret = false
        if (uriAuthority != null && "com.android.providers.media.documents" == uriAuthority) {
            ret = true
        }
        return ret
    }

    /*
     * Check whether google photos provide this document, if true means this image is created in google photos app.
     */
    private fun isGooglePhotoDoc(uriAuthority: String?): Boolean {
        var ret = false
        if (uriAuthority != null && "com.google.android.apps.photos.content" == uriAuthority) {
            ret = true
        }
        return ret
    }

    /* Return uri represented document file real local path.*/
    private fun getImageRealPath(
        contentResolver: ContentResolver,
        uri: Uri?,
        whereClause: String?
    ): String {
        var ret = ""

        if(uri != null) {
            // Query the uri with condition.
            val cursor: Cursor? = contentResolver.query(uri, null, whereClause, null, null)
            if (cursor != null) {
                val moveToFirst: Boolean = cursor.moveToFirst()
                if (moveToFirst) {

                    // Get columns name by uri type.
                    var columnName = MediaStore.Images.Media.DATA
                    if (uri === MediaStore.Images.Media.EXTERNAL_CONTENT_URI) {
                        columnName = MediaStore.Images.Media.DATA
                    } else if (uri === MediaStore.Audio.Media.EXTERNAL_CONTENT_URI) {
                        columnName = MediaStore.Audio.Media.DATA
                    } else if (uri === MediaStore.Video.Media.EXTERNAL_CONTENT_URI) {
                        columnName = MediaStore.Video.Media.DATA
                    }

                    // Get column index.
                    val imageColumnIndex: Int = cursor.getColumnIndex(columnName)

                    // Get column value which is the uri related file local path.
                    ret = cursor.getString(imageColumnIndex)
                    // Clean up
                    cursor.close()
                }
            }
        }
        return ret
    }

    /*
     * Native methods that are implemented by the 'dwindows' native library,
     * which is packaged with this application.
     */
    external fun dwindowsInit(dataDir: String, cacheDir: String, appid: String)
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
    external fun eventHandlerKey(obj1: View, message: Int, character: Int, vk: Int, modifiers: Int, str: String)

    companion object
    {
        // Used to load the 'dwindows' library on application startup.
        init
        {
            System.loadLibrary("dwindows")
        }
    }
}