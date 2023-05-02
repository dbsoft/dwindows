// (C) 2021-2023 Brian Smith <brian@dbsoft.org>
// (C) 2019 Anton Popov (Color Picker)
// (C) 2022 Amr Hesham (Tree View)
package org.dbsoft.dwindows

import android.Manifest
import android.R
import android.annotation.SuppressLint
import android.app.Activity
import android.app.Dialog
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.*
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.content.res.Resources
import android.database.Cursor
import android.graphics.*
import android.graphics.drawable.BitmapDrawable
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.graphics.pdf.PdfDocument
import android.media.AudioManager
import android.media.ToneGenerator
import android.net.Uri
import android.os.*
import android.print.*
import android.print.pdf.PrintedPdfDocument
import android.provider.DocumentsContract
import android.provider.MediaStore
import android.system.OsConstants
import android.text.InputFilter
import android.text.InputFilter.LengthFilter
import android.text.InputType
import android.text.format.DateFormat
import android.text.method.PasswordTransformationMethod
import android.util.*
import android.util.Base64
import android.view.*
import android.view.View.OnTouchListener
import android.view.inputmethod.EditorInfo
import android.webkit.JavascriptInterface
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.*
import android.widget.AdapterView.OnItemClickListener
import android.widget.SeekBar.OnSeekBarChangeListener
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatEditText
import androidx.collection.SimpleArrayMap
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.constraintlayout.widget.ConstraintSet
import androidx.constraintlayout.widget.Placeholder
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.core.content.res.ResourcesCompat
import androidx.core.view.MenuCompat
import androidx.core.view.setMargins
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayout.OnTabSelectedListener
import com.google.android.material.tabs.TabLayoutMediator
import java.io.*
import java.text.ParseException
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.locks.ReentrantLock
import java.util.zip.ZipEntry
import java.util.zip.ZipFile
import kotlin.math.*


// Tree View section
class DWTreeItem(title: String, icon: Drawable?, data: Long, parent: DWTreeItem?) {
    private var title: String
    private var parent: DWTreeItem?
    private val children: LinkedList<DWTreeItem>
    private var level: Int
    private var isExpanded: Boolean
    private var isSelected: Boolean
    private var data: Long = 0
    private var icon: Drawable? = null

    fun addChild(child: DWTreeItem) {
        child.setParent(this)
        child.setLevel(level + 1)
        children.add(child)
        updateNodeChildrenDepth(child)
    }

    fun setTitle(title: String) {
        this.title = title
    }

    fun getTitle(): String {
        return title
    }

    fun setIcon(icon: Drawable?) {
        this.icon = icon
    }

    fun getIcon(): Drawable? {
        return icon
    }

    fun setData(data: Long) {
        this.data = data
    }

    fun getData(): Long {
        return data
    }

    fun getParent(): DWTreeItem? {
        return parent
    }

    fun setParent(parent: DWTreeItem?) {
        this.parent = parent
    }

    fun getChildren(): LinkedList<DWTreeItem> {
        return children
    }

    fun setLevel(level: Int) {
        this.level = level
    }

    fun getLevel(): Int {
        return level
    }

    fun setExpanded(expanded: Boolean) {
        isExpanded = expanded
    }

    fun isExpanded(): Boolean {
        return isExpanded
    }

    fun setSelected(selected: Boolean) {
        isSelected = selected
    }

    fun isSelected(): Boolean {
        return isSelected
    }

    private fun updateNodeChildrenDepth(node: DWTreeItem) {
        if (node.getChildren().isEmpty()) return
        for (child in node.getChildren()) {
            child.setLevel(node.getLevel() + 1)
        }
    }

    init {
        this.title = title
        this.icon = icon
        this.data = data
        this.parent = parent
        children = LinkedList()
        level = 0
        isExpanded = false
        isSelected = false
    }
}

class DWTreeItemView : LinearLayout, Checkable {
    private var mChecked = false
    private var colorSelection = Color.DKGRAY
    var expandCollapseView: ImageView = ImageView(context)
    var iconView: ImageView = ImageView(context)
    var textView: TextView = TextView(context)

    fun updateBackground() {
        if(mChecked) {
            this.setBackgroundColor(colorSelection)
        } else {
            this.setBackgroundColor(Color.TRANSPARENT)
        }
    }

    override fun setChecked(b: Boolean) {
        mChecked = b
        updateBackground()
    }

    override fun isChecked(): Boolean {
        return mChecked
    }

    override fun toggle() {
        mChecked = !mChecked
        updateBackground()
    }

    fun setup(context: Context?) {
        var params = LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT)
        this.orientation = LinearLayout.HORIZONTAL
        params.gravity = Gravity.CENTER
        expandCollapseView.layoutParams = params
        expandCollapseView.id = View.generateViewId()
        this.addView(expandCollapseView)
        iconView.layoutParams = params
        iconView.id = View.generateViewId()
        this.addView(iconView)
        params = LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT)
        params.gravity = Gravity.CENTER
        textView.layoutParams = params
        textView.id = View.generateViewId()
        this.addView(textView)
        colorSelection = context?.let { getPlatformSelectionColor(it) }!!
    }

    constructor(context: Context?) : super(context) {
        setup(context)
    }
    constructor(context: Context?, attrs: AttributeSet?) : super(context, attrs) {
        setup(context)
    }
    constructor(context: Context?, attrs: AttributeSet?, defStyle: Int) : super(context, attrs, defStyle) {
        setup(context)
    }
}

class DWTreeItemManager {
    // Collection to save the current tree nodes
    private val rootsNodes: LinkedList<DWTreeItem>

    // Get DWTreeItem from the current nodes by index
    // @param index of node to get it
    // @return DWTreeItem from by index from current tree nodes if exists
    operator fun get(index: Int): DWTreeItem {
        return rootsNodes[index]
    }

    // Add new node to the current tree nodes
    // @param node to add it to the current tree nodes
    // @return true of this node is added
    fun addItem(node: DWTreeItem): Boolean {
        return rootsNodes.add(node)
    }

    // Clear the current nodes and insert new nodes
    // @param newNodes to update the current nodes with them
    fun updateItems(newNodes: List<DWTreeItem>?) {
        rootsNodes.clear()
        rootsNodes.addAll(newNodes!!)
    }

    // Delete one node from the visible nodes
    // @param node to delete it from the current nodes
    // @return true of this node is deleted
    fun removeItem(node: DWTreeItem): Boolean {
        return rootsNodes.remove(node)
    }

    // Clear the current nodes
    fun clearItems() {
        rootsNodes.clear()
    }

    // Get the current number of visible nodes
    // @return the size of visible nodes
    fun size(): Int {
        return rootsNodes.size
    }

    // Collapsing node and all of his children
    // @param node The node to collapse it
    // @return the index of this node if it exists in the list
    fun collapseItem(node: DWTreeItem): Int {
        val position = rootsNodes.indexOf(node)
        if (position != -1 && node.isExpanded()) {
            node.setExpanded(false)
            val deletedParents: LinkedList<DWTreeItem> =
                LinkedList(node.getChildren())
            rootsNodes.removeAll(node.getChildren())
            for (i in position + 1 until rootsNodes.size) {
                val iNode: DWTreeItem = rootsNodes[i]
                if (deletedParents.contains(iNode.getParent())) {
                    deletedParents.add(iNode)
                    deletedParents.addAll(iNode.getChildren())
                }
            }
            rootsNodes.removeAll(deletedParents)
        }
        return position
    }

    // Expanding node and all of his children
    // @param node The node to expand it
    // @return the index of this node if it exists in the list
    fun expandItem(node: DWTreeItem): Int {
        val position = rootsNodes.indexOf(node)
        if (position != -1 && !node.isExpanded()) {
            node.setExpanded(true)
            rootsNodes.addAll(position + 1, node.getChildren())
            for (child in node.getChildren()) {
                if (child.isExpanded()) updateExpandedItemChildren(child)
            }
        }
        return position
    }

    // Update the list for expanded node
    // to expand any child of his children that is already expanded before
    // @param node that just expanded now
    private fun updateExpandedItemChildren(node: DWTreeItem) {
        val position = rootsNodes.indexOf(node)
        if (position != -1 && node.isExpanded()) {
            rootsNodes.addAll(position + 1, node.getChildren())
            for (child in node.getChildren()) {
                if (child.isExpanded()) updateExpandedItemChildren(child)
            }
        }
    }

    // @param  node The node to collapse the branch of it
    // @return the index of this node if it exists in the list
    fun collapseItemBranch(node: DWTreeItem): Int {
        val position = rootsNodes.indexOf(node)
        if (position != -1 && node.isExpanded()) {
            node.setExpanded(false)
            for (child in node.getChildren()) {
                if (!child.getChildren().isEmpty()) collapseItemBranch(child)
                rootsNodes.remove(child)
            }
        }
        return position
    }

    // Expanding node full branches
    // @param  node The node to expand the branch of it
    // @return the index of this node if it exists in the list
    fun expandItemBranch(node: DWTreeItem): Int {
        val position = rootsNodes.indexOf(node)
        if (position != -1 && !node.isExpanded()) {
            node.setExpanded(true)
            var index = position + 1
            for (child in node.getChildren()) {
                val before: Int = rootsNodes.size
                rootsNodes.add(index, child)
                expandItemBranch(child)
                val after: Int = rootsNodes.size
                val diff = after - before
                index += diff
            }
        }
        return position
    }

    // Expanding one node branch to until specific level
    // @param node to expand branch of it until level
    // @param level to expand node branches to it
    fun expandItemToLevel(node: DWTreeItem, level: Int) {
        if (node.getLevel() <= level) expandItem(node)
        for (child in node.getChildren()) {
            expandItemToLevel(child, level)
        }
    }

    //Expanding all tree nodes branches to until specific level
    //@param level to expand all nodes branches to it
    fun expandItemsAtLevel(level: Int) {
        for (i in 0 until rootsNodes.size) {
            val node: DWTreeItem = rootsNodes[i]
            expandItemToLevel(node, level)
        }
    }

    // Collapsing all nodes in the tree with their children
    fun collapseAll() {
        val treeItems: MutableList<DWTreeItem> = LinkedList()
        for (i in 0 until rootsNodes.size) {
            val root: DWTreeItem = rootsNodes[i]
            if (root.getLevel() === 0) {
                collapseItemBranch(root)
                treeItems.add(root)
            } else {
                root.setExpanded(false)
            }
        }
        updateItems(treeItems)
    }

    // Expanding all nodes in the tree with their children
    fun expandAll() {
        for (i in 0 until rootsNodes.size) {
            val root: DWTreeItem = rootsNodes[i]
            expandItemBranch(root)
        }
    }

    // Simple constructor
    init {
        rootsNodes = LinkedList()
    }
}

open class DWTreeViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    // Return the current DWTreeItem padding value
    // @return The current padding value

    // Modify the current node padding value
    // @param padding the new padding value

    // The default padding value for the DWTreeItem item
    var nodePadding = 50

    // Bind method that provide padding and bind DWTreeItem to the view list item
    // @param node the current DWTreeItem
    fun bindTreeItem(node: DWTreeItem) {
        val padding: Int = node.getLevel() * nodePadding
        val treeItemView = itemView as DWTreeItemView

        treeItemView.setPadding(
            padding,
            treeItemView.paddingTop,
            treeItemView.paddingRight,
            treeItemView.paddingBottom
        )
        treeItemView.textView.text = node.getTitle()
        treeItemView.iconView.setImageDrawable(node.getIcon())
        if(node.getChildren().size == 0) {
            treeItemView.expandCollapseView.setImageDrawable(null)
        } else {
            if(node.isExpanded()) {
                treeItemView.expandCollapseView.setImageResource(R.drawable.ic_menu_more)
            } else {
                treeItemView.expandCollapseView.setImageResource(R.drawable.ic_menu_add)
            }
        }
        treeItemView.isChecked = node.isSelected()
    }
}

interface DWTreeViewHolderFactory {
    // Provide a TreeViewHolder class depend on the current view
    // @param view The list item view
    // @param layout The layout xml file id for current view
    // @return A TreeViewHolder instance
    fun getTreeViewHolder(view: View?, layout: Int): DWTreeViewHolder
}

class DWTreeCustomViewHolder(itemView: View) : DWTreeViewHolder(itemView) {
    fun bindTreeNode(node: DWTreeItem) {
        super.bindTreeItem(node)
        // Here you can bind your node and check if it selected or not
    }
}

class DWTreeViewAdapter : RecyclerView.Adapter<DWTreeViewHolder> {
    // Manager class for TreeItems to easily apply operations on them
    // and to make it easy for testing and extending
    private val treeItemManager: DWTreeItemManager

    // A ViewHolder Factory to get DWTreeViewHolder object that mapped with layout
    private val treeViewHolderFactory: DWTreeViewHolderFactory

    // The current selected Tree Item
    private var currentSelectedItem: DWTreeItem? = null

    // The current selected Tree Item
    private var currentSelectedItemView: DWTreeItemView? = null

    // Custom OnClickListener to be invoked when a DWTreeItem has been clicked.
    private var treeItemClickListener: ((DWTreeItem?, View?) -> Boolean)? = null

    // Custom OnLongClickListener to be invoked when a DWTreeItem has been clicked and hold.
    private var treeItemLongClickListener: ((DWTreeItem?, View?) -> Boolean)? = null

    // Custom OnListener to be invoked when a DWTreeItem has been expanded.
    private var treeItemExpandListener: ((DWTreeItem?, View?) -> Boolean)? = null

    // Simple constructor
    // @param factory a View Holder Factory mapped with layout id's
    constructor(factory: DWTreeViewHolderFactory) {
        treeViewHolderFactory = factory
        treeItemManager = DWTreeItemManager()
    }

    // Constructor used to accept user custom DWTreeItemManager class
    // @param factory a View Holder Factory mapped with layout id's
    // @param manager a custom tree node manager class
    constructor(factory: DWTreeViewHolderFactory, manager: DWTreeItemManager) {
        treeViewHolderFactory = factory
        treeItemManager = manager
    }

    override fun onCreateViewHolder(parent: ViewGroup, layoutId: Int): DWTreeViewHolder {
        val view = DWTreeItemView(parent.context)
        return treeViewHolderFactory.getTreeViewHolder(view, layoutId)
    }

    override fun onBindViewHolder(holder: DWTreeViewHolder, position: Int) {
        val currentNode: DWTreeItem = treeItemManager.get(position)
        holder.bindTreeItem(currentNode)
        val treeItemView = holder.itemView as DWTreeItemView

        // Handle touch on the expand/collapse image
        treeItemView.expandCollapseView.setOnClickListener { v ->
            // Handle node expand and collapse event
            if (!currentNode.getChildren().isEmpty()) {
                val isNodeExpanded: Boolean = currentNode.isExpanded()
                if (isNodeExpanded) collapseNode(currentNode) else expandNode(currentNode)
                currentNode.setExpanded(!isNodeExpanded)

                notifyDataSetChanged()

                // Handle DWTreeItem expand listener event
                if (!isNodeExpanded && treeItemExpandListener != null) treeItemExpandListener!!(
                    currentNode,
                    v
                )
            }
        }
        // Handle node selection
        holder.itemView.setOnClickListener { v ->
            // If touched anywhere else, change the selection
            currentNode.setSelected(true)
            treeItemView.isChecked = true
            currentSelectedItem?.setSelected(false)
            currentSelectedItemView?.isChecked = false
            currentSelectedItem = currentNode
            currentSelectedItemView = treeItemView

            notifyDataSetChanged()

            // Handle DWTreeItem click listener event
            if (treeItemClickListener != null) treeItemClickListener!!(
                currentNode,
                v
            )
        }

        // Handle DWTreeItem long click listener event
        holder.itemView.setOnLongClickListener { v ->
            if (treeItemLongClickListener != null) {
                return@setOnLongClickListener treeItemLongClickListener!!(
                    currentNode,
                    v
                )
            }
            true
        }
    }

    override fun getItemViewType(position: Int): Int {
        return 1
    }

    override fun getItemCount(): Int {
        return treeItemManager.size()
    }

    // Collapsing node and all of his children
    // @param node The node to collapse it
    fun collapseNode(node: DWTreeItem) {
        val position: Int = treeItemManager.collapseItem(node)
        if (position != -1) {
            notifyDataSetChanged()
        }
    }

    // Expanding node and all of his children
    // @param node The node to expand it
    fun expandNode(node: DWTreeItem) {
        val position: Int = treeItemManager.expandItem(node)
        if (position != -1) {
            notifyDataSetChanged()
        }
    }

    // Collapsing full node branches
    // @param node The node to collapse it
    fun collapseNodeBranch(node: DWTreeItem) {
        treeItemManager.collapseItemBranch(node)
        notifyDataSetChanged()
    }

    // Expanding node full branches
    // @param node The node to expand it
    fun expandNodeBranch(node: DWTreeItem) {
        treeItemManager.expandItemBranch(node)
        notifyDataSetChanged()
    }

    // Expanding one node branch to until specific level
    // @param node to expand branch of it until level
    // @param level to expand node branches to it
    fun expandNodeToLevel(node: DWTreeItem, level: Int) {
        treeItemManager.expandItemToLevel(node, level)
        notifyDataSetChanged()
    }

    // Expanding all tree nodes branches to until specific level
    // @param level to expand all nodes branches to it
    fun expandNodesAtLevel(level: Int) {
        treeItemManager.expandItemsAtLevel(level)
        notifyDataSetChanged()
    }

    // Collapsing all nodes in the tree with their children
    fun collapseAll() {
        treeItemManager.collapseAll()
        notifyDataSetChanged()
    }

    // Expanding all nodes in the tree with their children
    fun expandAll() {
        treeItemManager.expandAll()
        notifyDataSetChanged()
    }

    // Update the list of tree nodes
    // @param treeItems The new tree nodes
    fun updateTreeItems(treeItems: List<DWTreeItem>) {
        treeItemManager.updateItems(treeItems)
        notifyItemRangeInserted(0, treeItems.size)
    }

    // Clear all the items from the tree
    fun clear() {
        treeItemManager.clearItems()
        notifyDataSetChanged()
    }

    // Register a callback to be invoked when this DWTreeItem is clicked
    // @param listener The callback that will run
    fun setTreeItemClickListener(listener: (DWTreeItem?, View?) -> Boolean) {
        treeItemClickListener = listener
    }

    // Register a callback to be invoked when this DWTreeItem is clicked and held
    // @param listener The callback that will run
    fun setTreeItemLongClickListener(listener: (DWTreeItem?, View?) -> Boolean) {
        treeItemLongClickListener = listener
    }

    // Register a callback to be invoked when this DWTreeItem is expanded
    // @param listener The callback that will run
    fun setTreeItemExpandListener(listener: (DWTreeItem?, View?) -> Boolean) {
        treeItemExpandListener = listener
    }

    // @return The current selected DWTreeItem
    val selectedNode: DWTreeItem?
        get() = currentSelectedItem
}

class DWTree(context: Context) : RecyclerView(context)
{
    var roots: MutableList<DWTreeItem> = ArrayList()

    fun updateTree()
    {
        val treeViewAdapter = this.adapter as DWTreeViewAdapter

        treeViewAdapter.updateTreeItems(roots)
        treeViewAdapter.notifyDataSetChanged()
    }
}

// Color Wheel section
private val HUE_COLORS = intArrayOf(
    Color.RED,
    Color.YELLOW,
    Color.GREEN,
    Color.CYAN,
    Color.BLUE,
    Color.MAGENTA,
    Color.RED
)

private val SATURATION_COLORS = intArrayOf(
    Color.WHITE,
    setAlpha(Color.WHITE, 0)
)

open class ColorWheel @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private val hueGradient = GradientDrawable().apply {
        gradientType = GradientDrawable.SWEEP_GRADIENT
        shape = GradientDrawable.OVAL
        colors = HUE_COLORS
    }

    private val saturationGradient = GradientDrawable().apply {
        gradientType = GradientDrawable.RADIAL_GRADIENT
        shape = GradientDrawable.OVAL
        colors = SATURATION_COLORS
    }

    private val thumbDrawable = ThumbDrawable()
    private val hsvColor = HsvColor(value = 1f)

    private var wheelCenterX = 0
    private var wheelCenterY = 0
    private var wheelRadius = 0
    private var downX = 0f
    private var downY = 0f

    var rgb
        get() = hsvColor.rgb
        set(rgb) {
            hsvColor.rgb = rgb
            hsvColor.set(value = 1f)
            fireColorListener()
            invalidate()
        }

    var thumbRadius
        get() = thumbDrawable.radius
        set(value) {
            thumbDrawable.radius = value
            invalidate()
        }

    var thumbColor
        get() = thumbDrawable.thumbColor
        set(value) {
            thumbDrawable.thumbColor = value
            invalidate()
        }

    var thumbStrokeColor
        get() = thumbDrawable.strokeColor
        set(value) {
            thumbDrawable.strokeColor = value
            invalidate()
        }

    var thumbColorCircleScale
        get() = thumbDrawable.colorCircleScale
        set(value) {
            thumbDrawable.colorCircleScale = value
            invalidate()
        }

    var colorChangeListener: ((Int) -> Unit)? = null

    var interceptTouchEvent = true

    init {
        thumbRadius = 13
        thumbColor = Color.WHITE
        thumbStrokeColor = Color.DKGRAY
        thumbColorCircleScale = 0.7f
    }

    fun setRgb(r: Int, g: Int, b: Int) {
        rgb = Color.rgb(r, g, b)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val minDimension = minOf(
            MeasureSpec.getSize(widthMeasureSpec),
            MeasureSpec.getSize(heightMeasureSpec)
        )

        setMeasuredDimension(
            resolveSize(minDimension, widthMeasureSpec),
            resolveSize(minDimension, heightMeasureSpec)
        )
    }

    override fun onDraw(canvas: Canvas) {
        drawColorWheel(canvas)
        drawThumb(canvas)
    }

    private fun drawColorWheel(canvas: Canvas) {
        val hSpace = width - paddingLeft - paddingRight
        val vSpace = height - paddingTop - paddingBottom

        wheelCenterX = paddingLeft + hSpace / 2
        wheelCenterY = paddingTop + vSpace / 2
        wheelRadius = maxOf(minOf(hSpace, vSpace) / 2, 0)

        val left = wheelCenterX - wheelRadius
        val top = wheelCenterY - wheelRadius
        val right = wheelCenterX + wheelRadius
        val bottom = wheelCenterY + wheelRadius

        hueGradient.setBounds(left, top, right, bottom)
        saturationGradient.setBounds(left, top, right, bottom)
        saturationGradient.gradientRadius = wheelRadius.toFloat()

        hueGradient.draw(canvas)
        saturationGradient.draw(canvas)
    }

    private fun drawThumb(canvas: Canvas) {
        val r = hsvColor.saturation * wheelRadius
        val hueRadians = toRadians(hsvColor.hue)
        val x = cos(hueRadians) * r + wheelCenterX
        val y = sin(hueRadians) * r + wheelCenterY

        thumbDrawable.indicatorColor = hsvColor.rgb
        thumbDrawable.setCoordinates(x, y)
        thumbDrawable.draw(canvas)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> onActionDown(event)
            MotionEvent.ACTION_MOVE -> updateColorOnMotionEvent(event)
            MotionEvent.ACTION_UP -> {
                updateColorOnMotionEvent(event)
                if (isTap(event, downX, downY)) performClick()
            }
        }

        return true
    }

    private fun onActionDown(event: MotionEvent) {
        parent.requestDisallowInterceptTouchEvent(interceptTouchEvent)
        updateColorOnMotionEvent(event)
        downX = event.x
        downY = event.y
    }

    override fun performClick() = super.performClick()

    private fun updateColorOnMotionEvent(event: MotionEvent) {
        calculateColor(event)
        fireColorListener()
        invalidate()
    }

    private fun calculateColor(event: MotionEvent) {
        val legX = event.x - wheelCenterX
        val legY = event.y - wheelCenterY
        val hypot = minOf(hypot(legX, legY), wheelRadius.toFloat())
        val hue = (toDegrees(atan2(legY, legX)) + 360) % 360
        val saturation = hypot / wheelRadius
        hsvColor.set(hue, saturation, 1f)
    }

    private fun fireColorListener() {
        colorChangeListener?.invoke(hsvColor.rgb)
    }

    override fun onSaveInstanceState(): Parcelable {
        val superState = super.onSaveInstanceState()
        val thumbState = thumbDrawable.saveState()
        return ColorWheelState(superState, this, thumbState)
    }

    override fun onRestoreInstanceState(state: Parcelable) {
        if (state is ColorWheelState) {
            super.onRestoreInstanceState(state.superState)
            readColorWheelState(state)
        } else {
            super.onRestoreInstanceState(state)
        }
    }

    private fun readColorWheelState(state: ColorWheelState) {
        thumbDrawable.restoreState(state.thumbState)
        interceptTouchEvent = state.interceptTouchEvent
        rgb = state.rgb
    }
}

internal class ColorWheelState : View.BaseSavedState {

    val thumbState: ThumbDrawableState
    val interceptTouchEvent: Boolean
    val rgb: Int

    constructor(
        superState: Parcelable?,
        view: ColorWheel,
        thumbState: ThumbDrawableState
    ) : super(superState) {
        this.thumbState = thumbState
        interceptTouchEvent = view.interceptTouchEvent
        rgb = view.rgb
    }

    constructor(source: Parcel) : super(source) {
        thumbState = source.readThumbState()
        interceptTouchEvent = source.readBooleanCompat()
        rgb = source.readInt()
    }

    override fun writeToParcel(out: Parcel, flags: Int) {
        super.writeToParcel(out, flags)
        out.writeThumbState(thumbState, flags)
        out.writeBooleanCompat(interceptTouchEvent)
        out.writeInt(rgb)
    }

    companion object CREATOR : Parcelable.Creator<ColorWheelState> {

        override fun createFromParcel(source: Parcel) = ColorWheelState(source)

        override fun newArray(size: Int) = arrayOfNulls<ColorWheelState>(size)
    }
}
internal fun Parcel.writeBooleanCompat(value: Boolean) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        this.writeBoolean(value)
    } else {
        this.writeInt(if (value) 1 else 0)
    }
}

internal fun Parcel.readBooleanCompat(): Boolean {
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        this.readBoolean()
    } else {
        this.readInt() == 1
    }
}
private const val MAX_ALPHA = 255

open class GradientSeekBar @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private val gradientColors = IntArray(2)
    private val thumbDrawable = ThumbDrawable()
    private val gradientDrawable = GradientDrawable()
    private val argbEvaluator = android.animation.ArgbEvaluator()

    private lateinit var orientationStrategy: OrientationStrategy
    private var downX = 0f
    private var downY = 0f

    var startColor
        get() = gradientColors[0]
        set(color) { setColors(start = color) }

    var endColor
        get() = gradientColors[1]
        set(color) { setColors(end = color) }

    var offset = 0f
        set(offset) {
            field = ensureOffsetWithinRange(offset)
            calculateArgb()
        }

    var barSize = 0
        set(width) {
            field = width
            requestLayout()
        }

    var cornersRadius = 0f
        set(radius) {
            field = radius
            invalidate()
        }

    var orientation = Orientation.VERTICAL
        set(orientation) {
            field = orientation
            orientationStrategy = createOrientationStrategy()
            requestLayout()
        }

    var thumbColor
        get() = thumbDrawable.thumbColor
        set(value) {
            thumbDrawable.thumbColor = value
            invalidate()
        }

    var thumbStrokeColor
        get() = thumbDrawable.strokeColor
        set(value) {
            thumbDrawable.strokeColor = value
            invalidate()
        }

    var thumbColorCircleScale
        get() = thumbDrawable.colorCircleScale
        set(value) {
            thumbDrawable.colorCircleScale = value
            invalidate()
        }

    var thumbRadius
        get() = thumbDrawable.radius
        set(radius) {
            thumbDrawable.radius = radius
            requestLayout()
        }

    var argb = 0
        private set

    var colorChangeListener: ((Float, Int) -> Unit)? = null

    var interceptTouchEvent = true

    init {
        thumbColor = Color.WHITE
        thumbStrokeColor = Color.DKGRAY
        thumbColorCircleScale = 0.7f
        thumbRadius = 13
        barSize = 10
        cornersRadius = 5.0f
        offset = 0f
        orientation = Orientation.VERTICAL
        setColors(Color.TRANSPARENT, Color.BLACK)
    }

    private fun createOrientationStrategy(): OrientationStrategy  {
        return when (orientation) {
            Orientation.VERTICAL -> VerticalStrategy()
            Orientation.HORIZONTAL -> HorizontalStrategy()
        }
    }

    fun setColors(start: Int = startColor, end: Int = endColor) {
        updateGradientColors(start, end)
        calculateArgb()
    }

    private fun updateGradientColors(start: Int, end: Int) {
        gradientColors[0] = start
        gradientColors[1] = end
        gradientDrawable.colors = gradientColors
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val dimens = orientationStrategy.measure(this, widthMeasureSpec, heightMeasureSpec)
        setMeasuredDimension(dimens.width(), dimens.height())
    }

    override fun onDraw(canvas: Canvas) {
        drawGradientRect(canvas)
        drawThumb(canvas)
    }

    private fun drawGradientRect(canvas: Canvas) {
        gradientDrawable.orientation = orientationStrategy.gradientOrientation
        gradientDrawable.bounds = orientationStrategy.getGradientBounds(this)
        gradientDrawable.cornerRadius = cornersRadius
        gradientDrawable.draw(canvas)
    }

    private fun drawThumb(canvas: Canvas) {
        val coordinates = orientationStrategy.getThumbPosition(this, gradientDrawable.bounds)
        thumbDrawable.indicatorColor = argb
        thumbDrawable.setCoordinates(coordinates.x, coordinates.y)
        thumbDrawable.draw(canvas)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> onActionDown(event)
            MotionEvent.ACTION_MOVE -> calculateOffsetOnMotionEvent(event)
            MotionEvent.ACTION_UP -> {
                calculateOffsetOnMotionEvent(event)
                if (isTap(event, downX, downY)) performClick()
            }
        }

        return true
    }

    private fun onActionDown(event: MotionEvent) {
        parent.requestDisallowInterceptTouchEvent(interceptTouchEvent)
        calculateOffsetOnMotionEvent(event)
        downX = event.x
        downY = event.y
    }

    override fun performClick() = super.performClick()

    private fun calculateOffsetOnMotionEvent(event: MotionEvent) {
        offset = orientationStrategy.getOffset(this, event, gradientDrawable.bounds)
    }

    private fun calculateArgb() {
        argb = argbEvaluator.evaluate(offset, startColor, endColor) as Int
        fireListener()
        invalidate()
    }

    private fun fireListener() {
        colorChangeListener?.invoke(offset, argb)
    }

    override fun onSaveInstanceState(): Parcelable {
        val superState = super.onSaveInstanceState()
        val thumbState = thumbDrawable.saveState()
        return GradientSeekBarState(superState, this, thumbState)
    }

    override fun onRestoreInstanceState(state: Parcelable) {
        if (state is GradientSeekBarState) {
            super.onRestoreInstanceState(state.superState)
            readGradientSeekBarState(state)
        } else {
            super.onRestoreInstanceState(state)
        }
    }

    private fun readGradientSeekBarState(state: GradientSeekBarState) {
        updateGradientColors(state.startColor, state.endColor)
        offset = state.offset
        barSize = state.barSize
        cornersRadius = state.cornerRadius
        orientation = Orientation.values()[state.orientation]
        interceptTouchEvent = state.interceptTouchEvent
        thumbDrawable.restoreState(state.thumbState)
    }

    private fun ensureOffsetWithinRange(offset: Float) = ensureWithinRange(offset, 0f, 1f)

    enum class Orientation { VERTICAL, HORIZONTAL }
}

val GradientSeekBar.currentColorAlpha get() = Color.alpha(argb)

fun GradientSeekBar.setTransparentToColor(color: Int, respectAlpha: Boolean = true) {
    if (respectAlpha) {
        this.offset = Color.alpha(color) / MAX_ALPHA.toFloat()
    }
    this.setColors(
        setAlpha(color, 0),
        setAlpha(color, MAX_ALPHA)
    )
}

inline fun GradientSeekBar.setAlphaChangeListener(
    crossinline listener: (Float, Int, Int) -> Unit
) {
    this.colorChangeListener = { offset, color ->
        listener(offset, color, this.currentColorAlpha)
    }
}

fun GradientSeekBar.setBlackToColor(color: Int) {
    this.setColors(Color.BLACK, color)
}

internal class GradientSeekBarState : View.BaseSavedState {

    val startColor: Int
    val endColor: Int
    val offset: Float
    val barSize: Int
    val cornerRadius: Float
    val orientation: Int
    val interceptTouchEvent: Boolean
    val thumbState: ThumbDrawableState

    constructor(
        superState: Parcelable?,
        view: GradientSeekBar,
        thumbState: ThumbDrawableState
    ) : super(superState) {
        startColor = view.startColor
        endColor = view.endColor
        offset = view.offset
        barSize = view.barSize
        cornerRadius = view.cornersRadius
        orientation = view.orientation.ordinal
        interceptTouchEvent = view.interceptTouchEvent
        this.thumbState = thumbState
    }

    constructor(source: Parcel) : super(source) {
        startColor = source.readInt()
        endColor = source.readInt()
        offset = source.readFloat()
        barSize = source.readInt()
        cornerRadius = source.readFloat()
        orientation = source.readInt()
        interceptTouchEvent = source.readBooleanCompat()
        thumbState = source.readThumbState()
    }

    override fun writeToParcel(out: Parcel, flags: Int) {
        super.writeToParcel(out, flags)
        out.writeInt(startColor)
        out.writeInt(endColor)
        out.writeFloat(offset)
        out.writeInt(barSize)
        out.writeFloat(cornerRadius)
        out.writeInt(orientation)
        out.writeBooleanCompat(interceptTouchEvent)
        out.writeThumbState(thumbState, flags)
    }

    companion object CREATOR : Parcelable.Creator<GradientSeekBarState> {

        override fun createFromParcel(source: Parcel) = GradientSeekBarState(source)

        override fun newArray(size: Int) = arrayOfNulls<GradientSeekBarState>(size)
    }
}

internal class HorizontalStrategy : OrientationStrategy {

    private val rect = Rect()
    private val point = PointF()

    override val gradientOrientation = GradientDrawable.Orientation.LEFT_RIGHT

    override fun measure(view: GradientSeekBar, widthSpec: Int, heightSpec: Int): Rect {
        val widthSize = View.MeasureSpec.getSize(widthSpec)
        val maxHeight = maxOf(view.barSize, view.thumbRadius * 2)
        val preferredWidth = widthSize + view.paddingLeft + view.paddingRight
        val preferredHeight = maxHeight + view.paddingTop + view.paddingBottom
        val finalWidth = View.resolveSize(preferredWidth, widthSpec)
        val finalHeight = View.resolveSize(preferredHeight, heightSpec)
        return rect.apply { set(0, 0, finalWidth, finalHeight) }
    }

    override fun getGradientBounds(view: GradientSeekBar): Rect {
        val availableHeight = view.height - view.paddingTop - view.paddingRight
        val left = view.paddingLeft + view.thumbRadius
        val right = view.width - view.paddingRight - view.thumbRadius
        val top = view.paddingTop + (availableHeight - view.barSize) / 2
        val bottom = top + view.barSize
        return rect.apply { set(left, top, right, bottom) }
    }

    override fun getThumbPosition(view: GradientSeekBar, gradient: Rect): PointF {
        val x = (gradient.left + view.offset * gradient.width())
        val y = view.height / 2f
        return point.apply { set(x, y) }
    }

    override fun getOffset(view: GradientSeekBar, event: MotionEvent, gradient: Rect): Float {
        val checkedX = ensureWithinRange(event.x.roundToInt(), gradient.left, gradient.right)
        val relativeX = (checkedX - gradient.left).toFloat()
        return relativeX / gradient.width()
    }
}

internal fun View.isTap(lastEvent: MotionEvent, initialX: Float, initialY: Float): Boolean {
    val config = ViewConfiguration.get(context)
    val duration = lastEvent.eventTime - lastEvent.downTime
    val distance = hypot(lastEvent.x - initialX, lastEvent.y - initialY)
    return duration < ViewConfiguration.getTapTimeout() && distance < config.scaledTouchSlop
}

internal const val PI = Math.PI.toFloat()

internal fun toRadians(degrees: Float) = degrees / 180f * PI

internal fun toDegrees(radians: Float) = radians * 180f / PI

internal fun <T> ensureWithinRange(
    value: T,
    start: T,
    end: T
): T where T : Number, T : Comparable<T> = minOf(maxOf(value, start), end)

internal fun setAlpha(argb: Int, alpha: Int) =
    Color.argb(alpha, Color.red(argb), Color.green(argb), Color.blue(argb))

class HsvColor(hue: Float = 0f, saturation: Float = 0f, value: Float = 0f) {

    private val hsv = floatArrayOf(
        ensureHue(hue),
        ensureSaturation(saturation),
        ensureValue(value)
    )

    var hue
        get() = hsv[0]
        set(hue) { hsv[0] = ensureHue(hue) }

    var saturation
        get() = hsv[1]
        set(saturation) { hsv[1] = ensureSaturation(saturation) }

    var value
        get() = hsv[2]
        set(value) { hsv[2] = ensureValue(value) }

    var rgb
        get() = Color.HSVToColor(hsv)
        set(rgb) { Color.colorToHSV(rgb, hsv) }

    fun set(hue: Float = hsv[0], saturation: Float = hsv[1], value: Float = hsv[2]) {
        hsv[0] = ensureHue(hue)
        hsv[1] = ensureSaturation(saturation)
        hsv[2] = ensureValue(value)
    }

    private fun ensureHue(hue: Float) = ensureWithinRange(hue, 0f, 360f)

    private fun ensureValue(value: Float) = ensureWithinRange(value, 0f, 1f)

    private fun ensureSaturation(saturation: Float) = ensureValue(saturation)
}

internal interface OrientationStrategy {

    val gradientOrientation: GradientDrawable.Orientation

    fun measure(view: GradientSeekBar, widthSpec: Int, heightSpec: Int): Rect

    fun getGradientBounds(view: GradientSeekBar): Rect

    fun getThumbPosition(view: GradientSeekBar, gradient: Rect): PointF

    fun getOffset(view: GradientSeekBar, event: MotionEvent, gradient: Rect): Float
}

internal class ThumbDrawableState private constructor(
    val radius: Int,
    val thumbColor: Int,
    val strokeColor: Int,
    val colorCircleScale: Float
) : Parcelable {

    constructor(thumbDrawable: ThumbDrawable) : this(
        thumbDrawable.radius,
        thumbDrawable.thumbColor,
        thumbDrawable.strokeColor,
        thumbDrawable.colorCircleScale
    )

    constructor(parcel: Parcel) : this(
        parcel.readInt(),
        parcel.readInt(),
        parcel.readInt(),
        parcel.readFloat()
    )

    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeInt(radius)
        parcel.writeInt(thumbColor)
        parcel.writeInt(strokeColor)
        parcel.writeFloat(colorCircleScale)
    }

    override fun describeContents() = 0

    companion object {

        val EMPTY_STATE = ThumbDrawableState(0, 0, 0, 0f)

        @JvmField
        val CREATOR = object : Parcelable.Creator<ThumbDrawableState> {

            override fun createFromParcel(parcel: Parcel) = ThumbDrawableState(parcel)

            override fun newArray(size: Int) = arrayOfNulls<ThumbDrawableState>(size)
        }
    }
}

internal fun Parcel.writeThumbState(state: ThumbDrawableState, flags: Int) {
    this.writeParcelable(state, flags)
}

internal fun Parcel.readThumbState(): ThumbDrawableState {
    return this.readParcelable(ThumbDrawableState::class.java.classLoader)
        ?: ThumbDrawableState.EMPTY_STATE
}

internal class ThumbDrawable {

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { strokeWidth = 1f }
    private var x = 0f
    private var y = 0f

    var indicatorColor = 0
    var strokeColor = 0
    var thumbColor = 0
    var radius = 0

    var colorCircleScale = 0f
        set(value) { field = ensureWithinRange(value, 0f, 1f) }

    fun setCoordinates(x: Float, y: Float) {
        this.x = x
        this.y = y
    }

    fun draw(canvas: Canvas) {
        drawThumb(canvas)
        drawStroke(canvas)
        drawColorIndicator(canvas)
    }

    private fun drawThumb(canvas: Canvas) {
        paint.color = thumbColor
        paint.style = Paint.Style.FILL
        canvas.drawCircle(x, y, radius.toFloat(), paint)
    }

    private fun drawStroke(canvas: Canvas) {
        val strokeCircleRadius = radius - paint.strokeWidth / 2f

        paint.color = strokeColor
        paint.style = Paint.Style.STROKE
        canvas.drawCircle(x, y, strokeCircleRadius, paint)
    }

    private fun drawColorIndicator(canvas: Canvas) {
        val colorIndicatorCircleRadius = radius * colorCircleScale

        paint.color = indicatorColor
        paint.style = Paint.Style.FILL
        canvas.drawCircle(x, y, colorIndicatorCircleRadius, paint)
    }

    fun restoreState(state: ThumbDrawableState) {
        radius = state.radius
        thumbColor = state.thumbColor
        strokeColor = state.strokeColor
        colorCircleScale = state.colorCircleScale
    }

    fun saveState() = ThumbDrawableState(this)
}

internal class VerticalStrategy : OrientationStrategy {

    private val rect = Rect()
    private val point = PointF()

    override val gradientOrientation = GradientDrawable.Orientation.BOTTOM_TOP

    override fun measure(view: GradientSeekBar, widthSpec: Int, heightSpec: Int): Rect {
        val heightSize = View.MeasureSpec.getSize(heightSpec)
        val maxWidth = maxOf(view.barSize, view.thumbRadius * 2)
        val preferredWidth = maxWidth + view.paddingLeft + view.paddingRight
        val preferredHeight = heightSize + view.paddingTop + view.paddingBottom
        val finalWidth = View.resolveSize(preferredWidth, widthSpec)
        val finalHeight = View.resolveSize(preferredHeight, heightSpec)
        return rect.apply { set(0, 0, finalWidth, finalHeight) }
    }

    override fun getGradientBounds(view: GradientSeekBar): Rect {
        val availableWidth = view.width - view.paddingLeft - view.paddingRight
        val left = view.paddingLeft + (availableWidth - view.barSize) / 2
        val right = left + view.barSize
        val top = view.paddingTop + view.thumbRadius
        val bottom = view.height - view.paddingBottom - view.thumbRadius
        return rect.apply { set(left, top, right, bottom) }
    }

    override fun getThumbPosition(view: GradientSeekBar, gradient: Rect): PointF {
        val y = (gradient.top + (1f - view.offset) * gradient.height())
        val x = view.width / 2f
        return point.apply { set(x, y) }
    }

    override fun getOffset(view: GradientSeekBar, event: MotionEvent, gradient: Rect): Float {
        val checkedY = ensureWithinRange(event.y.roundToInt(), gradient.top, gradient.bottom)
        val relativeY = (checkedY - gradient.top).toFloat()
        return 1f - relativeY / gradient.height()
    }
}

// Main Dynamic Windows section
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
    const val HTML_MESSAGE = 20
}

val DWImageExts = arrayOf("", ".png", ".webp", ".jpg", ".jpeg", ".gif")

class DWTabViewPagerAdapter : RecyclerView.Adapter<DWTabViewPagerAdapter.DWEventViewHolder>() {
    val viewList = mutableListOf<View>()
    val pageList = mutableListOf<Long>()
    val titleList = mutableListOf<String?>()
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
    val HTMLAdds = mutableListOf<String>()

    //Implement shouldOverrideUrlLoading//
    @Deprecated("Deprecated in Java")
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
        // Inject the functions on the page on complete
        HTMLAdds.forEach { e -> view.loadUrl("javascript:function $e(body) { DWindows.postMessage('$e', body) }") }

        // Handle the DW_HTML_CHANGE_COMPLETE event
        eventHandlerHTMLChanged(view, DWEvent.HTML_CHANGED, url, 4)
    }

    external fun eventHandlerHTMLChanged(obj1: View, message: Int, URI: String, status: Int)
}

class DWWebViewInterface internal constructor(var view: View) {
    /* Show a toast from the web page  */
    @JavascriptInterface
    fun postMessage(name: String?, body: String?) {
        // Handle the DW_HTML_MESSAGE event
        eventHandlerHTMLMessage(view, DWEvent.HTML_MESSAGE, name, body)
    }

    external fun eventHandlerHTMLMessage(obj1: View, message: Int, hmltName: String?, htmlBody: String?)
}

class DWPrintDocumentAdapter : PrintDocumentAdapter()
{
    var context: Context? = null
    var pages: Int = 0
    var pdfDocument: PrintedPdfDocument? = null
    var print: Long = 0

    override fun onLayout(
        oldAttributes: PrintAttributes?,
        newAttributes: PrintAttributes,
        cancellationSignal: CancellationSignal?,
        callback: LayoutResultCallback,
        extras: Bundle?
    ) {
        // Create a new PdfDocument with the requested page attributes
        pdfDocument = context?.let { PrintedPdfDocument(it, newAttributes) }

        // Respond to cancellation request
        if (cancellationSignal?.isCanceled == true) {
            callback.onLayoutCancelled()
            return
        }

        if (pages > 0) {
            // Return print information to print framework
            PrintDocumentInfo.Builder("print_output.pdf")
                .setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT)
                .setPageCount(pages)
                .build()
                .also { info ->
                    // Content layout reflow is complete
                    callback.onLayoutFinished(info, true)
                }
        } else {
            // Otherwise report an error to the print framework
            callback.onLayoutFailed("No pages to print.")
        }
    }

    override fun onWrite(
        pageRanges: Array<out PageRange>,
        destination: ParcelFileDescriptor,
        cancellationSignal: CancellationSignal?,
        callback: WriteResultCallback
    ) {
        var writtenPagesArray: Array<PdfDocument.Page> = emptyArray()

        // Iterate over each page of the document,
        // check if it's in the output range.
        for (i in 0 until pages) {
            pdfDocument?.startPage(i)?.also { page ->

                // check for cancellation
                if (cancellationSignal?.isCanceled == true) {
                    callback.onWriteCancelled()
                    pdfDocument?.close()
                    pdfDocument = null
                    return
                }

                // Draw page content for printing
                var bitmap = Bitmap.createBitmap(page.canvas.width, page.canvas.height, Bitmap.Config.ARGB_8888)
                // Actual drawing is done in the JNI C code callback to the bitmap
                eventHandlerPrintDraw(print, bitmap, i, page.canvas.width, page.canvas.height)
                // Copy from the bitmap canvas our C code drew on to the PDF page canvas
                val rect = Rect(0, 0, page.canvas.width, page.canvas.height)
                page.canvas.drawBitmap(bitmap, rect, rect, null)

                // Rendering is complete, so page can be finalized.
                pdfDocument?.finishPage(page)

                // Add the new page to the array
                writtenPagesArray += page
            }
        }

        // Write PDF document to file
        try {
            pdfDocument?.writeTo(FileOutputStream(destination.fileDescriptor))
        } catch (e: IOException) {
            callback.onWriteFailed(e.toString())
            return
        } finally {
            pdfDocument?.close()
            pdfDocument = null
        }
        // Signal the print framework the document is complete
        callback.onWriteFinished(pageRanges)
    }

    override fun onFinish() {
        // Notify our C code so it can cleanup
        eventHandlerPrintFinish(print)
        super.onFinish()
    }

    external fun eventHandlerPrintDraw(print: Long, bitmap: Bitmap, page: Int, width: Int, height: Int)
    external fun eventHandlerPrintFinish(print: Long)
}

class DWSlider
@JvmOverloads constructor(context: Context): FrameLayout(context) {
    val slider: SeekBar = SeekBar(context)

    init {
        slider.layoutParams = LayoutParams(LayoutParams.MATCH_PARENT,
                LayoutParams.WRAP_CONTENT, Gravity.CENTER)
        addView(slider)
    }

    @Synchronized
    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        if(slider.rotation == 90F || slider.rotation == 270F) {
            val layoutHeight = MeasureSpec.getSize(heightMeasureSpec)
            // set slider width to layout heigth
            slider.layoutParams.width = layoutHeight
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        if(slider.rotation == 90F || slider.rotation == 270F) {
            // update layout width to the rotated height of the slider
            // otherwise the layout remains quadratic
            setMeasuredDimension(slider.measuredHeight, measuredHeight)
        }
    }
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
    var multiple = mutableListOf<Int>()
    var selected: Int = -1
    var colorFore: Int? = null
    var colorBack: Int? = null
    var colorSelected: Int = Color.DKGRAY

    init {
        adapter = object : ArrayAdapter<String>(
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
        colorSelected = getPlatformSelectionColor(context)
        onItemClickListener = this
    }

    override fun onItemClick(parent: AdapterView<*>?, view: View, position: Int, id: Long) {
        selected = position
        if(this.choiceMode == ListView.CHOICE_MODE_MULTIPLE) {
            if(multiple.contains(position)) {
                multiple.remove(position)
                view.setBackgroundColor(Color.TRANSPARENT)
            } else {
                multiple.add(position)
                view.setBackgroundColor(colorSelected)
            }
        }
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
    var backingBitmap: Bitmap? = null
    var typeface: Typeface? = null
    var fontsize: Float? = null
    var evx: Float = 0f
    var evy: Float = 0f
    var button: Int = 1

    fun createBitmap() {
        // If we don't have a backing bitmap, or its size in invalid
        if(backingBitmap == null ||
            backingBitmap!!.width != this.width || backingBitmap!!.height != this.height) {
            // Create the backing bitmap
            backingBitmap = Bitmap.createBitmap(this.width, this.height, Bitmap.Config.ARGB_8888)
        }
        cachedCanvas = Canvas(backingBitmap!!)
    }

    fun finishDraw() {
        cachedCanvas = null
        this.invalidate()
    }

    override fun onSizeChanged(width: Int, height: Int, oldWidth: Int, oldHeight: Int) {
        super.onSizeChanged(width, height, oldWidth, oldHeight)
        if(backingBitmap != null &&
            (backingBitmap!!.width != width || backingBitmap!!.height != height)) {
            backingBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        }
        // Send DW_SIGNAL_CONFIGURE
        eventHandlerInt(DWEvent.CONFIGURE, width, height, 0, 0)
        if(backingBitmap != null) {
            // Send DW_SIGNAL_EXPOSE
            eventHandlerInt(DWEvent.EXPOSE, 0, 0, width, height)
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if(backingBitmap != null && cachedCanvas == null) {
            canvas.drawBitmap(backingBitmap!!, 0f, 0f, null)
        } else {
            val savedCanvas = cachedCanvas
            cachedCanvas = canvas
            // Send DW_SIGNAL_EXPOSE
            eventHandlerInt(DWEvent.EXPOSE, 0, 0, this.width, this.height)
            cachedCanvas = savedCanvas
        }
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
            MenuCompat.setGroupDividerEnabled(menu!!, true)

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
                            // Toggle the check automatically
                            if(menuitem.menuitem!!.isCheckable) {
                                menuitem.menuitem!!.isChecked = !menuitem.menuitem!!.isChecked
                                menuitem.checked = menuitem.menuitem!!.isChecked
                            }
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
    var selected = mutableListOf<Boolean>()
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
                (((types[column] and (1 shl 2)) != 0) && (obj is Int || obj is Long))) {
                data[index] = obj
            }
            // If it isn't one of those special types, image or numeric...use string
            else if(((types[column] and 1) == 0) && ((types[column] and (1 shl 2)) == 0)
                        && obj is String) {
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

    fun changeRowSelected(row: Int, rselected: Boolean)
    {
        if(row > -1 && row < selected.size) {
            selected[row] = rselected
        }
    }

    fun getRowSelected(row: Int): Boolean
    {
        if(row > -1 && row < selected.size) {
            return selected[row]
        }
        return false
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
                selected.removeAt(0)
            }
        } else {
            data.clear()
            rowdata.clear()
            rowtitle.clear()
            selected.clear()
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
                selected.removeAt(i)
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
                selected.removeAt(i)
            }
        }
    }

    fun positionByTitle(title: String?): Int
    {
        for(i in 0 until rowtitle.size) {
            if(rowtitle[i] != null && rowtitle[i] == title) {
                return i
            }
        }
        return -1
    }

    fun positionByData(rdata: Long): Int
    {
        for(i in 0 until rowdata.size) {
            if (rowdata[i] == rdata) {
                return i
            }
        }
        return -1
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
            selected.add(false)
        }
        return startRow
    }

    fun clear()
    {
        data.clear()
        rowdata.clear()
        rowtitle.clear()
        selected.clear()
    }
}

class DWContainerRow : RelativeLayout, Checkable {
    private var mChecked = false
    private var colorSelection = Color.DKGRAY
    private var colorBackground: Int? = null
    var position: Int = -1
    var imageview: ImageView = ImageView(context)
    var text: TextView = TextView(context)
    var stack: LinearLayout = LinearLayout(context)
    var parent: ListView? = null

    override fun setBackgroundColor(color: Int) {
        colorBackground = color
        super.setBackgroundColor(color)
    }

    fun setup(context: Context?) {
        val wrap = RelativeLayout.LayoutParams.WRAP_CONTENT
        val match = RelativeLayout.LayoutParams.MATCH_PARENT
        var lp = RelativeLayout.LayoutParams(wrap, wrap)
        imageview.id = View.generateViewId()
        lp.setMargins(4)
        this.addView(imageview, lp)
        lp = RelativeLayout.LayoutParams(match, wrap)
        text.id = View.generateViewId()
        lp.addRule(RelativeLayout.RIGHT_OF, imageview.id);
        this.addView(text, lp)
        lp = RelativeLayout.LayoutParams(match, wrap)
        stack.id = View.generateViewId()
        stack.orientation = LinearLayout.HORIZONTAL
        stack.descendantFocusability = LinearLayout.FOCUS_BLOCK_DESCENDANTS
        lp.addRule(RelativeLayout.BELOW, imageview.id);
        lp.addRule(RelativeLayout.BELOW, text.id);
        this.addView(stack, lp)
    }

    constructor(context: Context?) : super(context) {
        setup(context)
        colorSelection = context?.let { getPlatformSelectionColor(it) }!!
    }
    constructor(context: Context?, attrs: AttributeSet?) : super(context, attrs) {
        setup(context)
        colorSelection = context?.let { getPlatformSelectionColor(it) }!!
    }
    constructor(context: Context?, attrs: AttributeSet?, defStyle: Int) : super(context, attrs, defStyle) {
        setup(context)
        colorSelection = context?.let { getPlatformSelectionColor(it) }!!
    }

    fun updateBackground() {
        if(mChecked) {
            super.setBackgroundColor(colorSelection)
        } else {
            // Preserve the stripe color when toggling selection
            if(colorBackground != null) {
                super.setBackgroundColor(colorBackground!!)
            } else {
                super.setBackgroundColor(Color.TRANSPARENT)
            }
        }
        if(parent is ListView && position != -1) {
            val cont = this.parent as ListView
            val adapter = cont.adapter as DWContainerAdapter

            adapter.model.changeRowSelected(position, mChecked)
        }
    }

    override fun setChecked(b: Boolean) {
        if(b != mChecked) {
            mChecked = b
            updateBackground()
        }
    }

    override fun isChecked(): Boolean {
        return mChecked
    }

    override fun toggle() {
        mChecked = !mChecked
        updateBackground()
    }
}

class DWContainerAdapter(c: Context) : BaseAdapter()
{
    private var context = c
    var model = DWContainerModel()
    var selectedItem: Int = -1
    var contMode: Int = 0
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
        var rowView: DWContainerRow? = view as DWContainerRow?
        var displayColumns = model.numberOfColumns()
        var isFilesystem: Boolean = false
        var extraColumns: Int = 1

        // If column 1 is bitmap and column 2 is text...
        if(displayColumns > 1 && (model.getColumnType(0) and 1) != 0 &&
            (model.getColumnType(1) and (1 shl 1)) != 0) {
            // We are a filesystem style container...
            isFilesystem = true
            extraColumns = 2
        }

        // In default mode (0), limit the columns to 1 or 2
        if(contMode == 0) {
            // depending on if we are filesystem style or not
            displayColumns = extraColumns
        }

        // If the view passed in is null we need to create the layout
        if(rowView == null) {
            rowView = DWContainerRow(context)

            // Save variables for later use
            rowView.position = position
            rowView.parent = parent as ListView

            // Handle DW_CONTAINER_MODE_MULTI by setting the orientation vertical
            if(contMode == 2) {
                rowView.stack.orientation = LinearLayout.VERTICAL
                rowView.stack.gravity = Gravity.LEFT
            }

            // If there are extra columns and we are not in default mode...
            // Add the columns to the stack (LinearLayout)
            for(i in extraColumns until displayColumns) {
                var content = model.getRowAndColumn(position, i)

                // Image
                if((model.getColumnType(i) and 1) != 0) {
                    val imageview = ImageView(context)
                    val params = LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                                                           LinearLayout.LayoutParams.WRAP_CONTENT)
                    if(contMode == 2)
                        params.gravity = Gravity.LEFT
                    else
                        params.gravity = Gravity.CENTER
                    imageview.layoutParams = params
                    imageview.id = View.generateViewId()
                    if (content is Drawable) {
                        imageview.setImageDrawable(content)
                    }
                    rowView.stack.addView(imageview)
                } else  {
                    // Everything else is displayed as text
                    var textview: TextView? = null

                    // Special case for DW_CONTAINER_MODE_MULTI
                    if(contMode == 2) {
                        // textview will be a text button instead
                        textview = Button(context)
                    } else {
                        textview = TextView(context)
                    }
                    var hsize = LinearLayout.LayoutParams.WRAP_CONTENT
                    if(contMode == 0)
                        hsize = LinearLayout.LayoutParams.MATCH_PARENT
                    val params = LinearLayout.LayoutParams(hsize, LinearLayout.LayoutParams.WRAP_CONTENT)
                    if(contMode == 2)
                        params.gravity = Gravity.LEFT
                    else
                        params.gravity = Gravity.CENTER
                    // Multi-line (vertical) mode does not require horizontal margins
                    if(contMode != 2)
                        params.setMargins(5, 0, 5, 0)
                    textview.layoutParams = params
                    textview.id = View.generateViewId()
                    if (content is String) {
                        textview.text = content
                    } else if(content is Long || content is Int) {
                        textview.text = content.toString()
                    }
                    textview.setOnClickListener {
                        var columnClicked: Int = i
                        if(isFilesystem) {
                            columnClicked = i - 1
                        }
                        eventHandlerInt(parent, DWEvent.COLUMN_CLICK, columnClicked, 0, 0, 0)
                    }
                    rowView.stack.addView(textview)
                }
            }
        } else {
            // Update the position and parent
            rowView.position = position
            rowView.parent = parent as ListView

            // Refresh the selected state from the model
            rowView.isChecked = model.getRowSelected(position)

            // Otherwise we just need to update the existing layout
            for(i in extraColumns until displayColumns) {
                var content = model.getRowAndColumn(position, i)

                // Image
                if((model.getColumnType(i) and 1) != 0) {
                    val imageview = rowView.stack.getChildAt(i - extraColumns)

                    if (imageview is ImageView && content is Drawable) {
                        imageview.setImageDrawable(content)
                    }
                } else {
                    // Text
                    val textview = rowView.stack.getChildAt(i - extraColumns)

                    if (textview is TextView) {
                        if (content is String) {
                            textview.text = content
                        } else if (content is Long || content is Int) {
                            textview.text = content.toString()
                        }
                        if(foreColor != null) {
                            textview.setTextColor(foreColor!!)
                        }
                    }
                }
            }
        }

        // Check the main column content, image or text
        var content = model.getRowAndColumn(position, 0)

        // Setup the built-in Image and Text based on if we are fileystem mode or not
        if(isFilesystem) {
            if(content is Drawable) {
                rowView.imageview.setImageDrawable(content)
            }
            content = model.getRowAndColumn(position, 1)
            if (content is String) {
                rowView.text.text = content
            } else if(content is Long || content is Int) {
                rowView.text.text = content.toString()
            }
        } else {
            if(content is Drawable) {
                rowView.imageview.setImageDrawable(content)
                rowView.text.text = ""
            } else if (content is String) {
                rowView.text.text = content
                rowView.imageview.setImageDrawable(null)
            } else if(content is Long || content is Int) {
                rowView.text.text = content.toString()
                rowView.imageview.setImageDrawable(null)
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
    external fun eventHandlerInt(
        obj1: View,
        message: Int,
        inta: Int,
        intb: Int,
        intc: Int,
        intd: Int
    )
}

private class DWMLE(c: Context): AppCompatEditText(c) {
    protected override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        if(!isHorizontalScrollBarEnabled) {
            this.maxWidth = w
        } else {
            this.maxWidth = -1
        }
        super.onSizeChanged(w, h, oldw, oldh)
    }
}

fun getPlatformSelectionColor(context: Context): Int {
    val bitmap = BitmapFactory.decodeResource(context.resources, R.drawable.list_selector_background)
    var redBucket: Float = 0f
    var greenBucket: Float = 0f
    var blueBucket: Float = 0f
    var pixelCount: Float = 0f

    if(bitmap != null) {
        for (y in 0 until bitmap.height) {
            for (x in 0 until bitmap.width) {
                val c = bitmap.getPixel(x, y)
                pixelCount++
                redBucket += Color.red(c)
                greenBucket += Color.green(c)
                blueBucket += Color.blue(c)
            }
        }

        return Color.rgb(
            (redBucket / pixelCount).toInt(),
            (greenBucket / pixelCount).toInt(),
            (blueBucket / pixelCount).toInt()
        )
    }
    return Color.GRAY
}

class DWindows : AppCompatActivity() {
    var windowLayout: ViewPager2? = null
    var threadLock = ReentrantLock()
    var threadCond = threadLock.newCondition()
    var notificationID: Int = 0
    var darkMode: Int = -1
    var contMode: Int = 0
    var lastClickView: View? = null
    var colorSelection: Int = Color.DKGRAY
    private var appID: String? = null
    private var paint = Paint()
    private var bgcolor: Int? = null
    private var fileURI: Uri? = null
    private var fileLock = ReentrantLock()
    private var fileCond = fileLock.newCondition()
    private var colorLock = ReentrantLock()
    private var colorCond = colorLock.newCondition()
    private var colorChosen: Int = 0
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

        colorSelection = getPlatformSelectionColor(this)

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

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        if(windowLayout != null) {
            val index = windowLayout!!.currentItem
            val count = windowMenuBars.count()

            if (count > 0 && index < count) {
                var menuBar = windowMenuBars[index]

                if(menuBar == null) {
                    menuBar = DWMenu()
                    windowMenuBars[index] = menuBar
                }
                menuBar.menu = menu
                return super.onCreateOptionsMenu(menu)
            }
        }
        return false
    }

    override fun onPrepareOptionsMenu(menu: Menu): Boolean {
        if(windowLayout != null) {
            val index = windowLayout!!.currentItem
            val count = windowMenuBars.count()

            if (count > 0 && index < count) {
                var menuBar = windowMenuBars[index]

                if(menuBar != null) {
                    menuBar.createMenu(menu, true)
                } else {
                    menuBar = DWMenu()
                    menuBar.createMenu(menu, true)
                    windowMenuBars[index] = menuBar
                }
                return super.onPrepareOptionsMenu(menu)
            }
        }
        return false
    }

    @Deprecated("Deprecated in Java")
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

    fun setContainerMode(mode: Int)
    {
        contMode = mode
    }

    fun browseURL(url: String): Int {
        var retval: Int = -1 // DW_ERROR_UNKNOWN

        waitOnUiThread {
            val browserIntent = Intent(Intent.ACTION_VIEW, Uri.parse(url))
            try {
                retval = 0 // DW_ERROR_NONE
                startActivity(browserIntent)
            } catch (e: ActivityNotFoundException) {
                retval = -1 // DW_ERROR_UNKNOWN
            }
        }
       return retval
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
                var changed: Boolean = false
                // Handle DW_MIS_ENABLED/DISABLED
                if((state and (1 or (1 shl 1))) != 0) {
                    var enabled = false

                    // Handle DW_MIS_ENABLED
                    if ((state and 1) != 0) {
                        enabled = true
                    }
                    menuitem.enabled = enabled
                    if (menuitem.menuitem != null && menuitem.menuitem!!.isEnabled != enabled) {
                        menuitem.menuitem!!.isEnabled = enabled
                        changed = true
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
                    if (menuitem.menuitem != null && menuitem.menuitem!!.isChecked != checked) {
                        menuitem.menuitem!!.isChecked = checked
                        changed = true
                    }
                    if(changed == true) {
                        runOnUiThread {
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
                adapter.notifyDataSetChanged()
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

    fun windowSetStyle(window: Any, style: Int, mask: Int)
    {
        // TODO: Need to handle menu items and others
        waitOnUiThread {
            if (window is TextView && window !is EditText) {
                val ourmask = (Gravity.HORIZONTAL_GRAVITY_MASK or Gravity.VERTICAL_GRAVITY_MASK) and mask

                if (ourmask != 0) {
                    // Gravity with the masked parts zeroed out
                    val newgravity = style and ourmask

                    window.gravity = newgravity
                }
            } else if(window is ImageButton) {
                // DW_BS_NOBORDER = 1
                if((mask and 1) == 1) {
                    val button = window as ImageButton
                    if((style and 1) == 1) {
                        button.background = null
                    } // TODO: Handle turning border back on if possible
                }
            } else if(window is DWMenuItem) {
                var menuitem = window as DWMenuItem
                var changed: Boolean = false

                // Handle DW_MIS_ENABLED/DISABLED
                if((mask and (1 or (1 shl 1))) != 0) {
                    var enabled = false

                    // Handle DW_MIS_ENABLED = 1 or DW_MIS_DISABLED = 0
                    if (((mask and 1) != 0 && (style and 1) != 0) ||
                        ((mask and (1 shl 1) != 0 && (style and (1 shl 1) == 0)))) {
                        enabled = true
                    }
                    menuitem.enabled = enabled
                    if(menuitem.menuitem != null && menuitem.menuitem!!.isEnabled != enabled) {
                        menuitem.menuitem!!.isEnabled = enabled
                        changed = true
                    }
                }

                // Handle DW_MIS_CHECKED/UNCHECKED
                if((mask and ((1 shl 2) or (1 shl 3))) != 0) {
                    var checked = false

                    // Handle DW_MIS_CHECKED = 1 or DW_MIS_UNCHECKED = 0
                    if (((mask and (1 shl 2)) != 0 && (style and (1 shl 2)) != 0) ||
                        ((mask and (1 shl 3) != 0 && (style and (1 shl 3) == 0)))) {
                        checked = true
                    }
                    menuitem.checked = checked
                    if (menuitem.menuitem != null && menuitem.menuitem!!.isChecked != checked) {
                        menuitem.menuitem!!.isChecked = checked
                        changed = true
                    }
                }
                if(changed == true) {
                    runOnUiThread {
                        invalidateOptionsMenu()
                    }
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
            window.isEnabled = state
            if(window is ImageButton) {
                val ib = window as ImageButton

                if(ib.drawable != null) {
                    if(state) {
                        ib.drawable.colorFilter = null
                    } else {
                        ib.drawable.colorFilter = PorterDuffColorFilter(Color.LTGRAY, PorterDuff.Mode.SRC_IN)
                    }
                    if(ib.background != null) {
                        if(state) {
                            ib.background.colorFilter = null
                        } else {
                            ib.background.colorFilter = PorterDuffColorFilter(Color.LTGRAY, PorterDuff.Mode.SRC_IN)
                        }
                    }
                }
            }
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
                adapter.notifyDataSetChanged()
                windowSwitchWindow(index)
            }
        }
    }

    fun windowDestroy(window: Any): Int {
        var retval: Int = 1 // DW_ERROR_GENERAL

        if(window is DWMenu) {
            menuDestroy(window as DWMenu)
            retval = 0 // DW_ERROR_NONE
        } else if(windowLayout != null && window is View) {
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

                    adapter.notifyDataSetChanged()
                    
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

    fun groupBoxNew(type: Int, pad: Int, title: String?): LinearLayout? {
        var box: LinearLayout? = null
        waitOnUiThread {
            box = RadioGroup(this)
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

    fun scrollBoxNew(type: Int, pad: Int): ScrollView? {
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

    fun scrollBoxGetPos(scrollBox: ScrollView, orient: Int): Int {
        var retval: Int = -1

        waitOnUiThread {
            // DW_VERT 1
            if(orient == 1) {
                retval = scrollBox.scrollY
            } else {
                retval = scrollBox.scrollX
            }
        }
        return retval
    }

    fun scrollBoxGetRange(scrollBox: ScrollView, orient: Int): Int {
        var retval: Int = -1

        waitOnUiThread {
            // DW_VERT 1
            if(orient == 1) {
                retval = scrollBox.getChildAt(0).height
            } else {
                retval = scrollBox.getChildAt(0).width
            }
        }
        return retval
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
                // If we are out of bounds, pass -1 to add to the end
                if(index >= box.childCount) {
                    box.addView(item, -1)
                } else {
                    box.addView(item, index)
                }
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
            button!!.setOnClickListener {
                lastClickView = button!!
                eventHandlerSimple(button!!, DWEvent.CLICKED)
            }

            if(resid > 0 && resid < 65536) {
                filename = resid.toString()
            } else {
                button!!.setImageResource(resid)
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
            entryfield!!.inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
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
            val inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_MULTI_LINE)

            mle = DWMLE(this)
            mle!!.tag = dataArrayMap
            mle!!.id = cid
            mle!!.isSingleLine = false
            mle!!.maxLines = Integer.MAX_VALUE
            mle!!.imeOptions = EditorInfo.IME_FLAG_NO_ENTER_ACTION
            mle!!.inputType = (inputType or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
            mle!!.isVerticalScrollBarEnabled = true
            mle!!.scrollBarStyle = View.SCROLLBARS_INSIDE_INSET
            mle!!.setHorizontallyScrolling(false)
            mle!!.isHorizontalScrollBarEnabled = false
            mle!!.gravity = Gravity.TOP or Gravity.LEFT
        }
        return mle
    }

    fun mleSetVisible(mle: EditText, line: Int)
    {
        waitOnUiThread {
            val layout = mle.layout

            if(layout != null) {
                val y: Int = layout.getLineTop(line)

                mle.scrollTo(0, y)
            }
        }
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

    fun mleSetAutoComplete(mle: EditText, state: Int)
    {
        waitOnUiThread {
            val inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_MULTI_LINE)

            // DW_MLE_COMPLETE_TEXT 1
            if((state and 1) == 1) {
                mle.inputType = (inputType or InputType.TYPE_TEXT_FLAG_AUTO_CORRECT)
            } else {
                mle.inputType = (inputType or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
            }
        }
    }

    fun mleSearch(mle: EditText, text: String, point: Int, flags: Int): Int
    {
        var retval: Int = -1
        var ignorecase: Boolean = true

        // DW_MLE_CASESENSITIVE 1
        if(flags == 1) {
            ignorecase = false
        }

        waitOnUiThread {
            retval = mle.text.indexOf(text, point, ignorecase)

            if(retval > -1) {
                mle.setSelection(retval, retval + text.length)
            }
        }
        return retval
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
                val adapter = pager.adapter as DWTabViewPagerAdapter
                tab.text = adapter.titleList[position]
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
                    adapter.titleList.add(0, null)
                    tabs.addTab(tab, 0)
                } else {
                    adapter.viewList.add(placeholder)
                    adapter.pageList.add(pageID)
                    adapter.titleList.add(null)
                    tabs.addTab(tab)
                }
                adapter.notifyDataSetChanged()
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
                    adapter.titleList.removeAt(index)
                    tabs.removeTab(tab)
                    adapter.notifyDataSetChanged()
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
                    adapter.titleList[index] = text
                }

                notebookCapsOff(tabs)
            }
        }
    }

    fun notebookPagePack(notebook: RelativeLayout, pageID: Long, box: View)
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
                adapter.notifyDataSetChanged()
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

                if (index > -1 && index < adapter.pageList.count()) {
                    tabs.setScrollPosition(index, 0F, true)
                    pager.setCurrentItem(index, true)
                }
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
            val percent: Float = position * 1000000.0F

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

    fun scrollBarNew(vertical: Int, cid: Int): DWSlider?
    {
        var scrollbar: DWSlider? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()

            scrollbar = DWSlider(this)
            scrollbar!!.tag = dataArrayMap
            scrollbar!!.id = cid
            scrollbar!!.slider.max = 1
            scrollbar!!.slider.progressTintList = null
            scrollbar!!.slider.progressBackgroundTintList = null
            if (vertical != 0) {
                scrollbar!!.slider.rotation = 90F
            }
            scrollbar!!.slider.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
                override fun onStopTrackingTouch(seekBar: SeekBar) {
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {
                }

                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    eventHandlerInt(scrollbar as View, DWEvent.VALUE_CHANGED, scrollbar!!.slider.progress, 0, 0, 0)
                }
            })
        }
        return scrollbar
    }

    fun sliderNew(vertical: Int, increments: Int, cid: Int): DWSlider?
    {
        var slider: DWSlider? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()

            slider = DWSlider(this)
            slider!!.tag = dataArrayMap
            slider!!.id = cid
            slider!!.slider.max = increments
            if (vertical != 0) {
                slider!!.slider.rotation = 90F
            }
            slider!!.slider.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
                override fun onStopTrackingTouch(seekBar: SeekBar) {
                }

                override fun onStartTrackingTouch(seekBar: SeekBar) {
                }

                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    eventHandlerInt(slider as View, DWEvent.VALUE_CHANGED, slider!!.slider.progress, 0, 0, 0)
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

    fun percentGetPos(percent: View): Int
    {
        var retval = 0

        waitOnUiThread {
            var progress: ProgressBar

            if(percent is ProgressBar) {
                progress = percent as ProgressBar
            } else {
                val slider = percent as DWSlider
                progress = slider.slider
            }
            retval = progress.progress
        }
        return retval
    }

    fun percentSetPos(percent: View, position: Int)
    {
        waitOnUiThread {
            var progress: ProgressBar

            if(percent is ProgressBar) {
                progress = percent as ProgressBar
            } else {
                val slider = percent as DWSlider
                progress = slider.slider
            }
            progress.progress = position
        }
    }

    fun percentSetRange(percent: View, range: Int)
    {
        waitOnUiThread {
            var progress: ProgressBar

            if(percent is ProgressBar) {
                progress = percent as ProgressBar
            } else {
                val slider = percent as DWSlider
                progress = slider.slider
            }
            progress.max = range
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
            html!!.addJavascriptInterface(DWWebViewInterface(html!!), "DWindows");
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

    fun htmlJavascriptAdd(html: WebView, name: String)
    {
        waitOnUiThread {
            val client = html.webViewClient as DWWebViewClient
            client.HTMLAdds += name
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
            spinbutton!!.inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
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
            combobox!!.inputType = (InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
            combobox!!.setText(text)
        }
        return combobox
    }

    fun treeNew(cid: Int): DWTree?
    {
        var tree: DWTree? = null

        waitOnUiThread {
            tree = DWTree(this)
            if(tree != null) {
                val dataArrayMap = SimpleArrayMap<String, Long>()
                val factory = object : DWTreeViewHolderFactory {
                    override fun getTreeViewHolder(view: View?, layout: Int): DWTreeViewHolder {
                        return DWTreeCustomViewHolder(view!!)
                    }
                }
                val treeViewAdapter = DWTreeViewAdapter(factory)
                tree!!.tag = dataArrayMap
                tree!!.id = cid
                tree!!.adapter = treeViewAdapter
                tree!!.layoutManager = LinearLayoutManager(this)
                treeViewAdapter.setTreeItemLongClickListener { treeitem: DWTreeItem?, view: View? ->
                    if(treeitem != null) {
                        eventHandlerTree(tree!!, DWEvent.ITEM_CONTEXT, treeitem,
                            treeitem.getTitle(), treeitem.getData())
                    }
                    true
                }
                treeViewAdapter.setTreeItemClickListener { treeitem: DWTreeItem?, view: View? ->
                    if(treeitem != null) {
                        eventHandlerTree(tree!!, DWEvent.ITEM_SELECT, treeitem,
                            treeitem.getTitle(), treeitem.getData())
                    }
                    true
                }
                treeViewAdapter.setTreeItemExpandListener { treeitem: DWTreeItem?, view: View? ->
                    if(treeitem != null) {
                        eventHandlerTreeItem(tree!!, DWEvent.TREE_EXPAND, treeitem)
                    }
                    true
                }
            }
        }
        return tree
    }

    fun treeInsertAfter(tree: DWTree, item: DWTreeItem?, title: String, icon: Drawable, parent: DWTreeItem?, itemdata: Long): DWTreeItem?
    {
        var treeitem: DWTreeItem? = null

        waitOnUiThread {
            var treeViewAdapter = tree.adapter as DWTreeViewAdapter

            treeitem = DWTreeItem(title, icon, itemdata, parent)
            if(parent == null) {
                tree.roots.add(treeitem!!)
            } else {
                parent.addChild(treeitem!!)
            }
            tree.updateTree()
        }
        return treeitem
    }

    fun treeGetTitle(tree: DWTree, item: DWTreeItem): String?
    {
        var retval: String? = null

        waitOnUiThread {
            retval = item.getTitle()
        }
        return retval
    }

    fun treeGetParent(tree: DWTree, item: DWTreeItem): DWTreeItem?
    {
        var retval: DWTreeItem? = null

        waitOnUiThread {
            retval = item.getParent()
        }
        return retval
    }

    fun treeItemChange(tree: DWTree, item: DWTreeItem, title: String?, icon: Drawable?)
    {
        waitOnUiThread {
            var changed = false

            if(title != null) {
                item.setTitle(title)
                changed = true
            }
            if(icon != null) {
                item.setIcon(icon)
                changed = true
            }
            if(changed == true) {
                var treeViewAdapter = tree.adapter as DWTreeViewAdapter
                treeViewAdapter.notifyDataSetChanged()
            }
        }
    }

    fun treeItemSetData(tree: DWTree, item: DWTreeItem, data: Long)
    {
        waitOnUiThread {
            item.setData(data)
        }
    }

    fun treeItemGetData(tree: DWTree, item: DWTreeItem): Long
    {
        var retval: Long = 0

        waitOnUiThread {
            retval = item.getData()
        }
        return retval
    }

    fun treeItemSelect(tree: DWTree, item: DWTreeItem)
    {
        waitOnUiThread {
            item.setSelected(true)
        }
    }

    fun treeItemExpand(tree: DWTree, item: DWTreeItem, state: Int)
    {
        waitOnUiThread {
            val treeViewAdapter = tree.adapter as DWTreeViewAdapter

            if(state != 0) {
                treeViewAdapter.expandNode(item)
            } else {
                treeViewAdapter.collapseNode(item)
            }
        }
    }

    fun treeItemDelete(tree: DWTree, item: DWTreeItem)
    {
        // TODO: Implement this
    }

    fun treeClear(tree: DWTree)
    {
        waitOnUiThread {
            val treeViewAdapter = tree.adapter as DWTreeViewAdapter
            treeViewAdapter.clear()
            tree.roots.clear()
        }
    }

    fun containerNew(cid: Int, multi: Int): ListView?
    {
        var cont: ListView? = null

        waitOnUiThread {
            val dataArrayMap = SimpleArrayMap<String, Long>()
            val adapter = DWContainerAdapter(this)

            // Save the global container mode into the adapter
            adapter.contMode = contMode
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
                val rowView: DWContainerRow = view as DWContainerRow

                view.isSelected = !view.isSelected
                rowView.toggle()
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

    // Create a new SparseBooleanArray with only the true or false contents
    private fun onlyBooleanArray(array: SparseBooleanArray, bool: Boolean): SparseBooleanArray
    {
        val newArray = SparseBooleanArray()

        for (i in 0 until array.size()) {
            if (array.valueAt(i) == bool) {
                newArray.put(array.keyAt(i), bool)
            }
        }
        return newArray
    }

    fun containerGetTitleStart(cont: ListView, flags: Int): String?
    {
        var retval: String? = null

        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            // Handle DW_CRA_SELECTED
            if((flags and 1) != 0) {
                val checked: SparseBooleanArray = onlyBooleanArray(cont.checkedItemPositions, true)

                if(checked.size() > 0) {
                    val position = checked.keyAt(0)

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
                    val checked: SparseBooleanArray = onlyBooleanArray(cont.checkedItemPositions, true)

                    // Otherwise loop until we find our current place
                    for (i in 0 until checked.size()) {
                        // Item position in adapter
                        val position: Int = checked.keyAt(i)

                        if (adapter.model.querypos == position && (i + 1) < checked.size()) {
                            val newpos = checked.keyAt(i + 1)

                            adapter.model.querypos = newpos
                            retval = adapter.model.getRowTitle(newpos)
                            break
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
                val checked: SparseBooleanArray = onlyBooleanArray(cont.checkedItemPositions, true)

                if(checked.size() > 0) {
                    val position = checked.keyAt(0)

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
                    val checked: SparseBooleanArray = onlyBooleanArray(cont.checkedItemPositions, true)

                    // Otherwise loop until we find our current place
                    for (i in 0 until checked.size()) {
                        // Item position in adapter
                        val position: Int = checked.keyAt(i)

                        if (adapter.model.querypos == position && (i + 1) < checked.size()) {
                            val newpos = checked.keyAt(i + 1)

                            adapter.model.querypos = newpos
                            retval = adapter.model.getRowData(newpos)
                            break
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

    fun containerChangeItemInt(cont: ListView, column: Int, row: Int, num: Long)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter

            adapter.model.setRowAndColumn(row, column, num)
        }
    }

    fun timeString(num: Int, zeroAllowed: Boolean): String
    {
        if(zeroAllowed && num == 0)
            return "00"
        if(num > 0 && num < 60) {
            if(num > 9)
                return num.toString()
            else
                return "0" + num.toString()
        }
        return "01"
    }

    fun yearString(year: Int): String
    {
        if(year < 100)
        {
            if(year > 69)
                return "19" + timeString(year, false)
            else
                return "20" + timeString(year, true)
        }
        if(year in 1901..2199)
            return year.toString()
        val calendar = Calendar.getInstance()
        val thisyear = calendar[Calendar.YEAR]
        return thisyear.toString()
    }

    fun containerChangeItemDate(cont: ListView, column: Int, row: Int, year: Int, month: Int, day: Int)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter
            val dateString = timeString(day, false) + "/" + timeString(month, false) + "/" + yearString(year)
            val sdf = SimpleDateFormat("dd/MM/yyyy")
            var date: Date? = null
            var s = dateString
            try {
                date = sdf.parse(dateString)
                val dateFormat = DateFormat.getDateFormat(this)
                s = dateFormat.format(date)
            } catch (e: ParseException) {
                // handle exception here !
            }
            adapter.model.setRowAndColumn(row, column, s)
        }
    }

    fun containerChangeItemTime(cont: ListView, column: Int, row: Int, hour: Int, minute: Int, second: Int)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter
            val timeStr = timeString(hour, true) + ":" + timeString(minute, true) + ":" + timeString(second, true)
            val sdf = SimpleDateFormat("hh:mm:ss")
            var date: Date? = null
            var s = timeStr
            try {
                date = sdf.parse(timeStr)
                val timeFormat = DateFormat.getTimeFormat(this)
                s = timeFormat.format(date)
            } catch (e: ParseException) {
                // handle exception here !
            }
            adapter.model.setRowAndColumn(row, column, s)
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

            adapter.lastClick = 0L
            adapter.lastClickRow = -1
            adapter.selectedItem = -1
            adapter.model.clear()

            windowSetData(cont, "_dw_rowstart", 0L)
        }
    }

    fun containerScroll(cont: ListView, direction: Int, rows: Int)
    {
        waitOnUiThread {
            // DW_SCROLL_UP 0
            if(direction == 0) {
                cont.smoothScrollByOffset(-rows)
                // DW_SCROLL_DOWN 1
            } else if(direction == 1) {
                cont.smoothScrollByOffset(rows)
                // DW_SCROLL_TOP 2
            } else if(direction == 2) {
                cont.setSelection(0)
                cont.smoothScrollToPosition(0)
                // DW_SCROLL_BOTTOM 3
            } else if(direction == 3) {
                val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter
                val pos = adapter.model.rowdata.size

                if(pos > 0) {
                    cont.setSelection(pos - 1)
                    cont.smoothScrollToPosition(pos - 1)
                }
            }
        }
    }

    fun containerCursor(cont: ListView, title: String?)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter
            val pos = adapter.model.positionByTitle(title)

            if(pos > -1) {
                cont.setSelection(pos)
                cont.smoothScrollToPosition(pos)
            }

        }
    }

    fun containerCursorByData(cont: ListView, rdata: Long)
    {
        waitOnUiThread {
            val adapter: DWContainerAdapter = cont.adapter as DWContainerAdapter
            val pos = adapter.model.positionByData(rdata)

            if(pos > -1) {
                cont.setSelection(pos)
                cont.smoothScrollToPosition(pos)
            }

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
                listbox.multiple.clear()
                val adapter = listbox.adapter as ArrayAdapter<String>
                adapter.notifyDataSetChanged()
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
                listbox.multiple.clear()
                val adapter = listbox.adapter as ArrayAdapter<String>
                adapter.notifyDataSetChanged()
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
                listbox.multiple.clear()
                val adapter = listbox.adapter as ArrayAdapter<String>
                adapter.notifyDataSetChanged()
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

                if(index > -1 && index < listbox.list.count()) {
                    listbox.list[index] = text
                    val adapter = listbox.adapter as ArrayAdapter<String>
                    adapter.notifyDataSetChanged()
                }
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
                    listbox.multiple.clear()
                    val adapter = listbox.adapter as ArrayAdapter<String>
                    adapter.notifyDataSetChanged()
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

                // If we are starting over....
                if(where == -1 && listbox.multiple.count() > 0) {
                    retval = listbox.multiple[0]
                } else {
                    // Otherwise loop until we find our current place
                    for (i in 0 until listbox.multiple.count()) {
                        // Item position in adapter
                        val position: Int = listbox.multiple[i]
                        // If we are at our current point... check to see
                        // if there is another one, and return it...
                        // otherwise we will return -1 to indicated we are done.
                        if (where == position && (i+1) < listbox.multiple.count()) {
                            retval = listbox.multiple[i+1]
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

    fun windowSetBitmap(window: View, resID: Int, file: String?): Int
    {
        var retval: Int = -1

        waitOnUiThread {
            var filename: String? = file

            if(resID > 0 && resID < 65536) {
                filename = resID.toString()
            } else if(resID != 0) {
                if (window is ImageButton) {
                    val button = window

                    button.setImageResource(resID)
                    retval = 0
                } else if (window is ImageView) {
                    val imageview = window

                    imageview.setImageResource(resID)
                    retval = 0
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
                                retval = 0
                            } else if (window is ImageView) {
                                val imageview = window

                                imageview.setImageBitmap(b)
                                retval = 0
                            }
                            break
                        } else {
                            retval = 1
                        }
                    } catch (e: IOException) {
                    }
                }
            }
        }
        return retval
    }

    fun windowSetBitmapFromData(window: View, resID: Int, data: ByteArray?, length: Int): Int
    {
        var retval: Int = -1

        waitOnUiThread {
            if(resID != 0) {
                if (window is ImageButton) {
                    val button = window

                    button.setImageResource(resID)
                    retval = 0
                } else if (window is ImageView) {
                    val imageview = window

                    imageview.setImageResource(resID)
                    retval = 0
                }
            }
            if(data != null) {
                val b = BitmapFactory.decodeByteArray(data, 0, length)

                if(b != null) {
                    if (window is ImageButton) {
                        val button = window

                        button.setImageBitmap(b)
                        retval = 0
                    } else if (window is ImageView) {
                        val imageview = window

                        imageview.setImageBitmap(b)
                        retval = 0
                    }
                } else {
                    retval = 1
                }
            }
        }
        return retval
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

    fun printRun(print: Long, flags: Int, jobname: String, pages: Int, runflags: Int): PrintJob?
    {
        var retval: PrintJob? = null

        waitOnUiThread {
            // Get a PrintManager instance
            val printManager = this.getSystemService(Context.PRINT_SERVICE) as PrintManager
            // Setup our print adapter
            val printAdapter = DWPrintDocumentAdapter()
            printAdapter.context = this
            printAdapter.pages = pages
            printAdapter.print = print
            // Start a print job, passing in a PrintDocumentAdapter implementation
            // to handle the generation of a print document
            retval = printManager.print(jobname, printAdapter, null)
        }
        return retval
    }

    fun printCancel(printjob: PrintJob)
    {
        waitOnUiThread {
            // Get a PrintManager instance
            val printManager = this.getSystemService(Context.PRINT_SERVICE) as PrintManager
            // Remove the job we earlier added from the queue
            printManager.printJobs.remove(printjob)
        }
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

    fun renderRedraw(render: DWRender, safe: Int)
    {
        runOnUiThread {
            if(safe != 0) {
                render.eventHandlerInt(DWEvent.EXPOSE, 0, 0, render.width, render.height)
            } else {
                render.invalidate()
            }
        }
    }

    fun renderCreateBitmap(render: DWRender)
    {
        runOnUiThread {
            render.createBitmap()
        }
    }

    fun renderFinishDraw(render: DWRender)
    {
        runOnUiThread {
            render.finishDraw()
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
        Log.d(appID, text)
    }

    @Deprecated("Deprecated in Java")
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

    fun getDataColumn(context: Context, uri: Uri?, selection: String?, selectionArgs: Array<String?>?): String? {
        var cursor: Cursor? = null
        val column = "_data"
        val projection = arrayOf(column)

        try {
            cursor = context.contentResolver.query(
                uri!!, projection, selection, selectionArgs,
                null
            )
            if (cursor != null && cursor.moveToFirst()) {
                val index = cursor.getColumnIndexOrThrow(column)
                return cursor.getString(index)
            }
        } finally {
            cursor?.close()
        }
        return null
    }

    fun fileOpen(filename: String, mode: Int): Int
    {
        var retval: Int = -1
        var uri = Uri.parse(filename)
        var smode: String = "r"
        var fd: ParcelFileDescriptor? = null

        if((mode and OsConstants.O_WRONLY) == OsConstants.O_WRONLY) {
            smode = "w"
        } else if((mode and OsConstants.O_RDWR) == OsConstants.O_RDWR) {
            smode = "rw"
        }
        try {
            fd = contentResolver.openFileDescriptor(uri, smode)
        } catch (e: FileNotFoundException) {
            fd = null
        }
        if (fd != null) {
            retval = fd.fd
        }
        return retval
    }

    // Defpath does not seem to be supported on Android using the ACTION_GET_CONTENT Intent
    fun fileBrowseNew(title: String, defpath: String?, ext: String?, flags: Int): String?
    {
        var retval: String? = null
        var permission = Manifest.permission.WRITE_EXTERNAL_STORAGE
        var permissions: Int = -1

        // Handle requesting permissions if necessary
        permissions = ContextCompat.checkSelfPermission(this, permission)
        if(permissions == -1) //PERMISSION_DENIED
        {
            // You can directly ask for the permission.
            requestPermissions(arrayOf(permission), 100)
        }

        // This can't be called from the main thread
        if(Looper.getMainLooper() != Looper.myLooper()) {
            var success = true

            fileLock.lock()
            waitOnUiThread {
                val fileintent = Intent(Intent.ACTION_GET_CONTENT)
                // TODO: Filtering requires MIME types, not extensions
                fileintent.type = "*/*"
                fileintent.addCategory(Intent.CATEGORY_OPENABLE)
                try {
                    startActivityForResult(fileintent, 100)
                } catch (e: ActivityNotFoundException) {
                    success = false
                }
            }

            if(success) {
                // Wait until the intent finishes.
                fileCond.await()
                fileLock.unlock()

                // Save the URI string for later use
                retval = fileURI.toString()

                // If DW_DIRECTORY_OPEN or DW_FILE_PATH ... use the path not URI
                if((flags and 65535) == 2 || ((flags shr 16) and 1) == 1) {
                    if (DocumentsContract.isDocumentUri(this, fileURI)) {
                        // ExternalStorageProvider
                        if (fileURI?.authority == "com.android.externalstorage.documents") {
                            val docId = DocumentsContract.getDocumentId(fileURI)
                            val split = docId.split(":").toTypedArray()
                            retval = Environment.getExternalStorageDirectory()
                                .toString() + "/" + split[1]
                        } else if (fileURI?.authority == "com.android.providers.downloads.documents") {
                            val id = DocumentsContract.getDocumentId(fileURI)
                            val contentUri = ContentUris.withAppendedId(
                                Uri.parse("content://downloads/public_downloads"),
                                java.lang.Long.valueOf(id)
                            )
                            retval = getDataColumn(this, contentUri, null, null)
                        } else if (fileURI?.authority == "com.android.providers.media.documents") {
                            val docId = DocumentsContract.getDocumentId(fileURI)
                            val split = docId.split(":").toTypedArray()
                            val type = split[0]
                            var contentUri: Uri? = null
                            if ("image" == type) {
                                contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                            } else if ("video" == type) {
                                contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                            } else if ("audio" == type) {
                                contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
                            }
                            val selection = "_id=?"
                            val selectionArgs = arrayOf<String?>(
                                split[1]
                            )
                            retval = getDataColumn(this, contentUri, selection, selectionArgs)
                        }
                    } else if (fileURI?.scheme == "content") {
                        retval = getDataColumn(this, fileURI, null, null)
                    }
                    // File
                    else if (fileURI?.scheme == "file") {
                        retval = fileURI?.path
                    }

                    // If we are opening a directory DW_DIRECTORY_OPEN
                    if (retval != null && (flags and 65535) == 2) {
                        val split = retval.split("/")
                        val filename = split.last()

                        if (filename != null) {
                            val pathlen = retval.length
                            val filelen = filename.length

                            retval = retval.substring(0, pathlen - filelen - 1)
                        }
                    }
                }
            } else {
                // If we failed to start the intent... use old dialog
                fileLock.unlock()
                retval = fileBrowse(title, defpath, ext, flags)
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

    // No reverse evaluate function to get the offset from a color in a range...
    // So we do a hacky while loop to test offsets in the range to see if we can
    // find the color and return the offset... return -1F on failure
    fun colorSliderOffset(chosenColor: Int, startColor: Int, endColor: Int): Float
    {
        val argbEvaluator = android.animation.ArgbEvaluator()
        var testOffset = 0F

        while(testOffset <= 1F) {
            val testColor = argbEvaluator.evaluate(testOffset, startColor, endColor) as Int

            if(testColor == chosenColor) {
                return testOffset
            }
            testOffset += 0.001F
        }
        return -1F
    }

    fun colorChoose(color: Int, alpha: Int, red: Int, green: Int, blue: Int): Int
    {
        var retval: Int = color

        // This can't be called from the main thread
        if(Looper.getMainLooper() != Looper.myLooper()) {
            colorLock.lock()
            waitOnUiThread {
                val dialog = Dialog(this)
                val colorWheel = ColorWheel(this, null, 0)
                val gradientBar = GradientSeekBar(this, null, 0)
                val display = View(this)
                val layout = RelativeLayout(this)
                val w = RelativeLayout.LayoutParams.MATCH_PARENT
                val h = RelativeLayout.LayoutParams.WRAP_CONTENT
                val margin = 10

                colorWheel.id = View.generateViewId()
                gradientBar.id = View.generateViewId()
                display.id = View.generateViewId()
                gradientBar.orientation = GradientSeekBar.Orientation.HORIZONTAL

                var params: RelativeLayout.LayoutParams = RelativeLayout.LayoutParams(w, 100)
                params.addRule(RelativeLayout.ALIGN_PARENT_TOP)
                params.setMargins(margin,margin,margin,margin)
                layout.addView(display, params)
                params = RelativeLayout.LayoutParams(w, w)
                params.setMargins(margin,margin,margin,margin)
                params.addRule(RelativeLayout.BELOW, display.id)
                params.addRule(RelativeLayout.ABOVE, gradientBar.id)
                layout.addView(colorWheel, params)
                params = RelativeLayout.LayoutParams(w, h)
                params.setMargins(margin,margin,margin,margin)
                params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM)
                layout.addView(gradientBar, params)

                dialog.setContentView(layout)
                colorChosen = Color.rgb(red, green, blue)
                colorWheel.rgb = colorChosen
                gradientBar.setBlackToColor(colorWheel.rgb)
                val testOffset = colorSliderOffset(colorChosen, gradientBar.startColor, gradientBar.endColor)
                if(testOffset < 0F) {
                    // If our test method didn't work... convert to HSV
                    // and use the brightness value as the slider offset
                    var hsv = FloatArray(3)
                    Color.colorToHSV(colorChosen, hsv)
                    gradientBar.offset = hsv[2]
                } else {
                    gradientBar.offset = testOffset
                }
                display.setBackgroundColor(colorChosen)
                colorWheel.colorChangeListener = { rgb: Int ->
                    gradientBar.setBlackToColor(rgb)
                    display.setBackgroundColor(gradientBar.argb)
                    colorChosen = gradientBar.argb
                }
                gradientBar.colorChangeListener = { offset: Float, argb: Int ->
                    display.setBackgroundColor(argb)
                    colorChosen = argb
                }
                dialog.window?.setLayout(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                )
                dialog.setOnDismissListener {
                    colorLock.lock()
                    colorCond.signal()
                    colorLock.unlock()
                }
                dialog.show()
            }
            colorCond.await()
            retval = colorChosen
            colorLock.unlock()
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
        appID = appid

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
                .setSmallIcon(R.mipmap.sym_def_app_icon)
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
                if (ActivityCompat.checkSelfPermission(
                        this@DWindows,
                        Manifest.permission.POST_NOTIFICATIONS
                    ) == PackageManager.PERMISSION_GRANTED
                ) {
                    notify(notificationID, builder.build())
                }
            }
        }
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
    external fun eventHandlerTree(obj1: View, message: Int, item: DWTreeItem?, title: String?, data: Long)
    external fun eventHandlerTreeItem(obj1: View, message: Int, item: DWTreeItem?)
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