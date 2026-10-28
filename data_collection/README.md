# Serial data-collection tools

These scripts support the earlier serial-to-PC workflow. The active
[STM32Cube/HAL firmware](../firmware/README.md) publishes directly over Wi-Fi;
the PC gateway is optional and should be stopped when the direct publisher is
running.

| Script | Purpose |
| --- | --- |
| [collect_data.py](collect_data.py) | Record six raw integer sensor axes to a timestamped walking/running CSV |
| [preview_payload.py](preview_payload.py) | Convert raw readings to the six-field JSON contract and print one per second |
| [mqtt_sensor_publisher.py](mqtt_sensor_publisher.py) | Forward converted readings to an existing local MQTT listener/tunnel using QoS 1 |

Install these scripts' dependencies from the repository root:

```powershell
python -m pip install -r data_collection/requirements.txt
```

Configure the serial port in the selected script; the checked-in default is
`COM9` at 115200 baud. The input must be six comma-separated raw integers in
`ax, ay, az, gx, gy, gz` order. These parsers are intended for that earlier raw
CSV firmware output, rather than the active firmware's diagnostic/JSON stream.

For a local capture:

```powershell
python data_collection/collect_data.py
```

`collect_data.py` uses a manually selected label and stores its CSV beside the
script. Generated capture CSVs are excluded from Git. Preview/publisher scaling
assumes the original sensor ranges: acceleration raw/16384 in **g**, and
gyro raw/131 converted from degrees/s to **rad/s**.

The gateway currently targets `127.0.0.1:18883`; configure its constants and
establish the required local broker or SSH tunnel before running it. It does
not set up that listener or replace the direct firmware's MQTT/TLS connection.
