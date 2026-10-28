package com.yena.rtsparkactivity

import android.content.Context
import com.android.volley.DefaultRetryPolicy
import com.android.volley.Request
import com.android.volley.RequestQueue
import com.android.volley.TimeoutError
import com.android.volley.VolleyError
import com.android.volley.toolbox.JsonObjectRequest
import com.android.volley.toolbox.Volley
import org.json.JSONObject
import javax.net.ssl.SSLException

data class FeedResult(
    val latest: ActivityRecord?,
    val history: List<ActivityRecord>?,
    val latestError: String?,
    val historyError: String?,
)

/** Uses Android's HTTPS and hostname checks, with the domain CA declared in XML. */
class RtsparkApi(context: Context) {
    private val queue: RequestQueue = queueFor(context)
    private val tag = Any()

    fun cancel() = queue.cancelAll(tag)

    fun fetch(deliver: (FeedResult) -> Unit) {
        var remaining = 2
        var latest: ActivityRecord? = null
        var history: List<ActivityRecord>? = null
        var latestError: String? = null
        var historyError: String? = null
        // Volley delivers both callbacks on the main thread.
        fun finish() {
            remaining -= 1
            if (remaining == 0) deliver(FeedResult(latest, history, latestError, historyError))
        }
        fun request(path: String, success: (JSONObject) -> Unit, failure: (VolleyError) -> Unit) {
            val request = JsonObjectRequest(Request.Method.GET, BASE_URL + path, null,
                { json -> success(json); finish() },
                { error -> failure(error); finish() })
            request.setShouldCache(false)
            request.retryPolicy = DefaultRetryPolicy(8_000, 0, 1f)
            request.tag = tag
            queue.add(request)
        }
        request("/api/activity/latest", { json ->
            try { latest = ActivityRecord.fromJson(json) }
            catch (_: Exception) { latestError = "The server returned an unexpected prediction." }
        }, { error ->
            // This endpoint returns 404 when the database has no predictions yet.
            if (error.networkResponse?.statusCode != 404) latestError = explain(error)
        })
        request("/api/activity/history?limit=50", { json ->
            try { history = ActivityRecord.history(json) }
            catch (_: Exception) { historyError = "The server returned an unexpected history response." }
        }, { error -> historyError = explain(error) })
    }

    companion object {
        const val BASE_URL = "https://ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com"
        @Volatile private var sharedQueue: RequestQueue? = null
        private fun queueFor(context: Context): RequestQueue = sharedQueue ?: synchronized(this) {
            sharedQueue ?: Volley.newRequestQueue(context.applicationContext).also { sharedQueue = it }
        }

        private fun explain(error: VolleyError): String {
            if (generateSequence<Throwable>(error) { it.cause }.any { it is SSLException }) {
                return "Secure connection failed. Check the device date and the app certificate."
            }
            return when {
                error is TimeoutError -> "The server did not respond. Check Wi-Fi and API access."
                error.networkResponse != null -> "The server returned HTTP ${error.networkResponse.statusCode}. Try Refresh."
                else -> "Cannot reach the server. Check Wi-Fi and API access."
            }
        }
    }
}
