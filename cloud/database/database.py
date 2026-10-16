from pathlib import Path
import sqlite3


DATABASE_PATH = Path(__file__).resolve().parent / "activity.db"


def get_connection():
    return sqlite3.connect(DATABASE_PATH)


def initialize_database():
    with get_connection() as connection:
        connection.execute(
            """
            CREATE TABLE IF NOT EXISTS activity_records (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                activity_id INTEGER NOT NULL,
                activity TEXT NOT NULL,
                confidence REAL NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
            """
        )

    print(f"Database ready: {DATABASE_PATH}")


def save_prediction(activity_id, activity, confidence):
    with get_connection() as connection:
        cursor = connection.execute(
            """
            INSERT INTO activity_records (
                activity_id,
                activity,
                confidence
            )
            VALUES (?, ?, ?)
            """,
            (
                activity_id,
                activity,
                confidence,
            ),
        )

        return cursor.lastrowid


def get_latest_prediction():
    with get_connection() as connection:
        connection.row_factory = sqlite3.Row

        row = connection.execute(
            """
            SELECT *
            FROM activity_records
            ORDER BY id DESC
            LIMIT 1
            """
        ).fetchone()

        return dict(row) if row else None


def get_activity_history(limit=50):
    with get_connection() as connection:
        connection.row_factory = sqlite3.Row

        rows = connection.execute(
            """
            SELECT *
            FROM activity_records
            ORDER BY id DESC
            LIMIT ?
            """,
            (limit,),
        ).fetchall()

        return [dict(row) for row in rows]


if __name__ == "__main__":
    initialize_database()