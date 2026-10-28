package com.yena.rtsparkactivity

import org.json.JSONArray
import org.json.JSONObject
import java.time.Instant
import java.time.LocalDateTime
import java.time.ZoneId
import java.time.ZoneOffset
import java.time.format.DateTimeFormatter
import java.time.format.ResolverStyle
import java.util.Locale

/** The API's SQLite timestamps are UTC. Convert them only when displaying. */
data class ActivityRecord(
    val id: Long,
    val activityId: Int,
    val activity: String,
    val confidence: Double,
    val recordedAt: Instant,
) {
    val title: String get() = activity.lowercase(Locale.ROOT).replaceFirstChar { it.uppercase() }
    val percentage: String get() = String.format(Locale.getDefault(), "%.1f%%", confidence * 100)

    fun localTime(zone: ZoneId = ZoneId.systemDefault()): String =
        DISPLAY_TIME.withZone(zone).format(recordedAt)

    fun toJson(): JSONObject = JSONObject()
        .put("id", id).put("activity_id", activityId).put("activity", activity)
        .put("confidence", confidence).put("created_at", DATABASE_TIME.format(recordedAt.atOffset(ZoneOffset.UTC)))

    companion object {
        private val DATABASE_TIME = DateTimeFormatter.ofPattern("uuuu-MM-dd HH:mm:ss", Locale.ROOT)
            .withResolverStyle(ResolverStyle.STRICT)
        private val DISPLAY_TIME = DateTimeFormatter.ofPattern("MMM d, yyyy · h:mm:ss a", Locale.getDefault())

        fun fromJson(json: JSONObject): ActivityRecord {
            val id = json.getLong("id")
            val activityId = json.getInt("activity_id")
            val activity = json.getString("activity")
            val confidence = json.getDouble("confidence")
            require(id > 0) { "Invalid prediction ID" }
            require((activityId == 0 && activity == "WALKING") ||
                (activityId == 1 && activity == "RUNNING")) { "Unsupported activity" }
            require(confidence.isFinite() && confidence in 0.0..1.0) { "Invalid confidence" }
            val date = LocalDateTime.parse(json.getString("created_at"), DATABASE_TIME)
                .toInstant(ZoneOffset.UTC)
            return ActivityRecord(id, activityId, activity, confidence, date)
        }

        fun history(json: JSONObject): List<ActivityRecord> {
            val records = json.getJSONArray("records")
            require(json.getInt("count") == records.length() && records.length() <= 50) {
                "Unexpected history size"
            }
            val parsed = (0 until records.length()).map { fromJson(records.getJSONObject(it)) }
            require(parsed.map { it.id }.distinct().size == parsed.size) { "Duplicate prediction IDs" }
            return parsed.sortedByDescending { it.id }
        }

        fun historyJson(records: List<ActivityRecord>): JSONObject = JSONObject()
            .put("count", records.size).put("records", JSONArray(records.map { it.toJson() }))
    }
}

enum class FeedFreshness { LIVE, WAITING, CLOCK_MISMATCH }

fun freshness(recordedAt: Instant, now: Instant = Instant.now()): FeedFreshness {
    val age = now.toEpochMilli() - recordedAt.toEpochMilli()
    return when {
        age < -30_000 -> FeedFreshness.CLOCK_MISMATCH
        age > 20_000 -> FeedFreshness.WAITING
        else -> FeedFreshness.LIVE
    }
}
