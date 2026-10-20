package com.yena.rtsparkactivity

import android.app.Activity
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.BaseAdapter
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ListView
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONObject
import java.time.Instant

/** Native Kotlin views fit the project's current Android module; no Compose plugin is needed. */
class RtsparkActivity : Activity() {
    private val navy = Color.rgb(22, 42, 62)
    private val muted = Color.rgb(91, 108, 125)
    private val teal = Color.rgb(0, 105, 96)
    private val coral = Color.rgb(170, 62, 38)
    private val paper = Color.rgb(245, 248, 251)
    private val paleTeal = Color.rgb(224, 243, 237)
    private val paleCoral = Color.rgb(255, 237, 230)
    private val amber = Color.rgb(121, 78, 0)
    private val line = Color.rgb(222, 229, 236)
    private val handler = Handler(Looper.getMainLooper())
    private lateinit var api: RtsparkApi
    private lateinit var refreshButton: Button
    private lateinit var liveButton: Button
    private lateinit var historyButton: Button
    private lateinit var livePage: ScrollView
    private lateinit var historyPage: LinearLayout
    private lateinit var status: TextView
    private lateinit var statusDetail: TextView
    private lateinit var activityName: TextView
    private lateinit var confidence: TextView
    private lateinit var confidenceBar: ProgressBar
    private lateinit var recorded: TextView
    private lateinit var recordNumber: TextView
    private lateinit var historySummary: TextView
    private lateinit var historyError: TextView
    private lateinit var emptyHistory: TextView
    private val historyAdapter = HistoryAdapter()
    private var latest: ActivityRecord? = null
    private var history: List<ActivityRecord> = emptyList()
    private var latestError: String? = null
    private var historyProblem: String? = null
    private var hasResponse = false
    private var selectedTab = 0
    private var started = false
    private var refreshing = false
    private var generation = 0
    private val poll = Runnable { refresh() }
    private val ticker = object : Runnable {
        override fun run() {
            if (!started) return
            renderStatus()
            handler.postDelayed(this, 1_000)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        setTheme(android.R.style.Theme_Material_Light_NoActionBar)
        super.onCreate(savedInstanceState)
        api = RtsparkApi(applicationContext)
        selectedTab = savedInstanceState?.getInt("tab") ?: 0
        try {
            savedInstanceState?.getString("latest")?.let { latest = ActivityRecord.fromJson(JSONObject(it)) }
            savedInstanceState?.getString("history")?.let { history = ActivityRecord.history(JSONObject(it)) }
        } catch (_: Exception) { latest = null; history = emptyList() }
        buildInterface()
        render()
        selectTab(selectedTab)
    }

    override fun onStart() {
        super.onStart()
        started = true
        refresh()
        handler.post(ticker)
    }

    override fun onStop() {
        started = false
        generation += 1
        handler.removeCallbacksAndMessages(null)
        api.cancel()
        refreshing = false
        super.onStop()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putInt("tab", selectedTab)
        latest?.let { outState.putString("latest", it.toJson().toString()) }
        outState.putString("history", ActivityRecord.historyJson(history).toString())
        super.onSaveInstanceState(outState)
    }

    private fun refresh() {
        if (!started || refreshing) return
        handler.removeCallbacks(poll)
        refreshing = true
        val attempt = ++generation
        refreshButton.isEnabled = false
        refreshButton.text = "Refreshing…"
        api.fetch { result ->
            if (!started || attempt != generation) return@fetch
            refreshing = false
            hasResponse = true
            latestError = result.latestError
            historyProblem = result.historyError
            if (result.latestError == null) latest = result.latest
            if (result.history != null) history = result.history
            render()
            handler.postDelayed(poll, 5_000)
        }
    }

    private fun buildInterface() {
        val root = column().apply { setBackgroundColor(paper) }
        root.setOnApplyWindowInsetsListener { view, insets ->
            if (Build.VERSION.SDK_INT >= 30) {
                val bars = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
                view.setPadding(dp(20) + bars.left, dp(10) + bars.top, dp(8) + bars.right, dp(8) + bars.bottom)
            } else {
                @Suppress("DEPRECATION")
                view.setPadding(dp(20) + insets.systemWindowInsetLeft, dp(10) + insets.systemWindowInsetTop,
                    dp(8) + insets.systemWindowInsetRight, dp(8) + insets.systemWindowInsetBottom)
            }
            insets
        }
        val header = row()
        val brand = column()
        brand.addView(label("RTSpark", 28, bold = true))
        brand.addView(label("ACTIVITY MONITOR", 11, muted).apply { letterSpacing = .13f })
        header.addView(brand, LinearLayout.LayoutParams(0, -2, 1f))
        refreshButton = button("Refresh").apply {
            setTextColor(teal); background = rounded(Color.WHITE, 16, line)
            setOnClickListener { refresh() }
        }
        header.addView(refreshButton, LinearLayout.LayoutParams(-2, dp(48)))
        root.addView(header, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(22) })
        val body = FrameLayout(this)
        root.addView(body, LinearLayout.LayoutParams(-1, 0, 1f))
        livePage = ScrollView(this).apply { isFillViewport = true; clipToPadding = false; setPadding(0, 0, dp(12), dp(8)) }
        livePage.addView(buildLive())
        body.addView(livePage, FrameLayout.LayoutParams(-1, -1))
        historyPage = buildHistory()
        body.addView(historyPage, FrameLayout.LayoutParams(-1, -1))
        val navigation = row().apply { setPadding(0, dp(12), dp(12), 0) }
        liveButton = button("Live activity").apply { setOnClickListener { selectTab(0) } }
        historyButton = button("History").apply { setOnClickListener { selectTab(1) } }
        navigation.addView(liveButton, LinearLayout.LayoutParams(0, dp(52), 1f).apply { marginEnd = dp(8) })
        navigation.addView(historyButton, LinearLayout.LayoutParams(0, dp(52), 1f))
        root.addView(navigation)
        setContentView(root)
        root.requestApplyInsets()
    }

    private fun buildLive(): LinearLayout = column().apply {
        status = label("CONNECTING", 12, amber, true).apply {
            letterSpacing = .07f; setPadding(dp(12), dp(8), dp(12), dp(8))
        }
        addView(status, LinearLayout.LayoutParams(-2, -2))
        gap(this, 18)
        addView(label("Live activity", 30, bold = true))
        addView(label("The latest prediction from your sensor.", 15, muted), spaceBelow(22))
        val card = column().apply { background = rounded(Color.WHITE, 24, line); setPadding(dp(24), dp(24), dp(24), dp(24)) }
        card.addView(label("LATEST PREDICTION", 11, muted, true).apply { letterSpacing = .12f })
        activityName = label("Waiting…", 38, teal, true)
        card.addView(activityName, LinearLayout.LayoutParams(-1, -2).apply { topMargin = dp(22); bottomMargin = dp(24) })
        confidence = label("Confidence —", 16, bold = true)
        card.addView(confidence)
        confidenceBar = ProgressBar(this@RtsparkActivity, null, android.R.attr.progressBarStyleHorizontal).apply {
            max = 1_000; progress = 0
            progressTintList = ColorStateList.valueOf(teal)
            progressBackgroundTintList = ColorStateList.valueOf(line)
            importantForAccessibility = View.IMPORTANT_FOR_ACCESSIBILITY_NO
        }
        card.addView(confidenceBar, LinearLayout.LayoutParams(-1, dp(10)).apply { topMargin = dp(10); bottomMargin = dp(26) })
        card.addView(label("RECORDED · LOCAL TIME", 10, muted, true))
        recorded = label("—", 15)
        card.addView(recorded, spaceBelow(16))
        recordNumber = label("No prediction yet", 13, muted)
        card.addView(recordNumber)
        addView(card, spaceBelow(16))
        statusDetail = label("Connecting to your activity feed…", 14, muted)
        addView(statusDetail, spaceBelow(24))
        val info = column().apply { background = rounded(paleTeal, 18); setPadding(dp(18), dp(16), dp(18), dp(16)) }
        info.addView(label("About this model", 14, teal, true))
        info.addView(label("Recognizes walking and running. A resting sensor can still receive either label.", 14, navy))
        addView(info)
        gap(this, 16)
        addView(label("Refreshes automatically while this app is open.", 12, muted))
    }

    private fun buildHistory(): LinearLayout = column().apply {
        setPadding(0, 0, dp(12), 0)
        addView(label("Activity history", 30, bold = true))
        historySummary = label("Your 50 most recent predictions.", 14, muted)
        addView(historySummary, spaceBelow(16))
        historyError = label("", 14, coral)
        addView(historyError, spaceBelow(12))
        val container = FrameLayout(this@RtsparkActivity)
        val list = ListView(this@RtsparkActivity).apply {
            divider = android.graphics.drawable.ColorDrawable(Color.TRANSPARENT)
            dividerHeight = dp(10)
            adapter = historyAdapter
            clipToPadding = false
            setPadding(0, 0, 0, dp(12))
        }
        container.addView(list, FrameLayout.LayoutParams(-1, -1))
        emptyHistory = label("Waiting for the first prediction…", 16, muted).apply { gravity = Gravity.CENTER; setPadding(dp(20), dp(20), dp(20), dp(20)) }
        container.addView(emptyHistory, FrameLayout.LayoutParams(-1, -1))
        list.emptyView = emptyHistory
        addView(container, LinearLayout.LayoutParams(-1, 0, 1f))
    }

    private fun render() {
        refreshButton.isEnabled = !refreshing
        refreshButton.text = if (refreshing) "Refreshing…" else "Refresh"
        val item = latest
        val oldName = activityName.text.toString()
        activityName.text = item?.title ?: "Waiting…"
        val accent = if (item?.activityId == 1) coral else teal
        activityName.setTextColor(accent)
        if (oldName != activityName.text.toString()) {
            activityName.alpha = .4f
            activityName.animate().alpha(1f).setDuration(240).start()
        }
        confidence.text = item?.let { "Confidence ${it.percentage}" } ?: "Confidence —"
        confidenceBar.progressTintList = ColorStateList.valueOf(accent)
        confidenceBar.progress = ((item?.confidence ?: 0.0) * 1_000).toInt()
        recorded.text = item?.localTime() ?: "—"
        recordNumber.text = item?.let { "Prediction #${it.id}" } ?: "No prediction yet"
        val walks = history.count { it.activityId == 0 }
        val runs = history.count { it.activityId == 1 }
        historySummary.text = if (history.isEmpty()) "Your 50 most recent predictions."
            else "${history.size} recent · $walks walking · $runs running"
        historyError.text = historyProblem.orEmpty()
        historyError.visibility = if (historyProblem == null) View.GONE else View.VISIBLE
        emptyHistory.text = if (historyProblem != null) "History is unavailable. Tap Refresh to try again."
            else if (hasResponse) "No predictions yet. Your sensor feed will appear here."
            else "Loading activity history…"
        historyAdapter.submit(history)
        renderStatus()
    }

    private fun renderStatus() {
        val item = latest
        val state = item?.let { freshness(it.recordedAt) }
        val (text, color, background) = when {
            latestError != null -> Triple("CONNECTION ISSUE", coral, paleCoral)
            !hasResponse -> Triple("CONNECTING", amber, Color.rgb(255, 244, 213))
            item == null -> Triple("WAITING FOR DATA", amber, Color.rgb(255, 244, 213))
            state == FeedFreshness.CLOCK_MISMATCH -> Triple("CHECK DEVICE TIME", amber, Color.rgb(255, 244, 213))
            state == FeedFreshness.WAITING -> Triple("WAITING FOR SENSOR", amber, Color.rgb(255, 244, 213))
            else -> Triple("LIVE", teal, paleTeal)
        }
        status.text = text
        status.setTextColor(color)
        status.background = rounded(background, 12)
        statusDetail.text = when {
            latestError != null -> "${latestError} ${if (item != null) "Showing the last received prediction." else ""}"
            !hasResponse -> "Connecting to your activity feed…"
            item == null -> "No predictions yet. Keep the board powered and connected to Wi-Fi."
            state == FeedFreshness.CLOCK_MISMATCH -> "The prediction time is ahead of this device. Check its date and time."
            state == FeedFreshness.WAITING -> "The latest prediction is over 20 seconds old. Check the board's power and Wi-Fi."
            else -> "Updated ${maxOf(0L, Instant.now().epochSecond - item.recordedAt.epochSecond)}s ago · checking every 5 seconds"
        }
    }

    private fun selectTab(tab: Int) {
        selectedTab = if (tab == 1) 1 else 0
        livePage.visibility = if (selectedTab == 0) View.VISIBLE else View.GONE
        historyPage.visibility = if (selectedTab == 1) View.VISIBLE else View.GONE
        listOf(liveButton, historyButton).forEachIndexed { index, button ->
            button.isSelected = index == selectedTab
            button.setTextColor(if (button.isSelected) Color.WHITE else navy)
            button.background = rounded(if (button.isSelected) teal else Color.WHITE, 16, if (button.isSelected) teal else line)
        }
    }

    private inner class HistoryAdapter : BaseAdapter() {
        private var rows: List<ActivityRecord> = emptyList()
        fun submit(records: List<ActivityRecord>) { if (rows != records) { rows = records; notifyDataSetChanged() } }
        override fun getCount(): Int = rows.size
        override fun getItem(position: Int): ActivityRecord = rows[position]
        override fun getItemId(position: Int): Long = rows[position].id
        override fun hasStableIds(): Boolean = true
        override fun isEnabled(position: Int): Boolean = false
        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            val holder: HistoryHolder
            val root: LinearLayout
            if (convertView == null) {
                root = row().apply { background = rounded(Color.WHITE, 18, line); setPadding(dp(16), dp(18), dp(16), dp(18)) }
                val detail = column()
                val title = label("", 18, bold = true)
                val time = label("", 12, muted)
                val id = label("", 11, muted)
                detail.addView(title); detail.addView(time); detail.addView(id)
                root.addView(detail, LinearLayout.LayoutParams(0, -2, 1f))
                val score = label("", 15, teal, true).apply { setPadding(dp(10), dp(8), dp(10), dp(8)) }
                root.addView(score)
                holder = HistoryHolder(title, time, id, score)
                root.tag = holder
            } else { root = convertView as LinearLayout; holder = root.tag as HistoryHolder }
            val item = rows[position]
            holder.title.text = item.title
            holder.time.text = item.localTime()
            holder.id.text = "Prediction #${item.id}"
            holder.score.text = item.percentage
            holder.score.contentDescription = "Confidence ${item.percentage}"
            holder.score.setTextColor(if (item.activityId == 1) coral else teal)
            holder.score.background = rounded(if (item.activityId == 1) paleCoral else paleTeal, 12)
            return root
        }
    }

    private data class HistoryHolder(val title: TextView, val time: TextView, val id: TextView, val score: TextView)
    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + .5f).toInt()
    private fun column(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun row(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER_VERTICAL }
    private fun label(value: String, size: Int, color: Int = navy, bold: Boolean = false): TextView = TextView(this).apply {
        text = value; textSize = size.toFloat(); setTextColor(color)
        setTypeface(Typeface.create("sans-serif", if (bold) Typeface.BOLD else Typeface.NORMAL))
        setLineSpacing(dp(3).toFloat(), 1f)
    }
    private fun button(value: String): Button = Button(this).apply {
        text = value; textSize = 14f; isAllCaps = false; minWidth = dp(80); minimumHeight = dp(48)
        setPadding(dp(14), 0, dp(14), 0); stateListAnimator = null
    }
    private fun rounded(color: Int, radius: Int, border: Int? = null): GradientDrawable = GradientDrawable().apply {
        setColor(color); cornerRadius = dp(radius).toFloat(); if (border != null) setStroke(dp(1), border)
    }
    private fun spaceBelow(value: Int): LinearLayout.LayoutParams = LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(value) }
    private fun gap(parent: LinearLayout, height: Int) = parent.addView(View(this), LinearLayout.LayoutParams(1, dp(height)))
}
