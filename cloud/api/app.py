from flask import Flask, jsonify, request

import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT_DIR))

from cloud.database.database import (
    initialize_database,
    get_latest_prediction,
    get_activity_history,
)


app = Flask(__name__)

initialize_database()


@app.route("/", methods=["GET"])
def home():
    return jsonify({
        "service": "RT-Spark Activity Recognition API",
        "status": "running"
    })


@app.route("/api/activity/latest", methods=["GET"])
def latest_activity():
    record = get_latest_prediction()

    if record is None:
        return jsonify({
            "message": "No activity predictions available."
        }), 404

    return jsonify(record)


@app.route("/api/activity/history", methods=["GET"])
def activity_history():
    try:
        limit = int(request.args.get("limit", 50))
    except ValueError:
        return jsonify({
            "error": "limit must be an integer"
        }), 400

    limit = max(1, min(limit, 500))

    records = get_activity_history(limit)

    return jsonify({
        "count": len(records),
        "records": records
    })


if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=False
    )