# RT-Spark Android app

The Kotlin client displays the latest Walking/Running prediction and recent
history from the cloud API. It uses Volley for HTTPS requests and refreshes
while the app is open, with stale-data and connection-error states.

## Open and run

1. Open this `android_app/` directory in Android Studio.
2. Sync the Gradle project using the checked-in wrapper and version catalog.
   The project requests Android SDK 37, supports Android 8.0/API 26 and newer,
   and specifies Gradle daemon JVM 25.
3. Select an emulator or connected Android phone, then run the `app` target.

From this directory, the debug build commands are:

```powershell
.\gradlew.bat assembleDebug
```

On Linux/macOS:

```sh
./gradlew assembleDebug
```

Android Studio, the SDK and the requested Java/Gradle dependencies are required.
Generated builds, local SDK paths and signing material are excluded from Git.

## Server configuration

The checked-in hostname and public CA target the original laboratory server.
When using another server, configure these files together:

| File | Setting |
| --- | --- |
| [RtsparkApi.kt](app/src/main/java/com/yena/rtsparkactivity/RtsparkApi.kt) | `BASE_URL` for the HTTPS API |
| [rtspark_network_security.xml](app/src/main/res/xml/rtspark_network_security.xml) | Exact trusted hostname and public CA resource |
| [rtspark_ca.crt](app/src/main/res/raw/rtspark_ca.crt) | Public CA certificate that signs that server's certificate |

The app requires HTTPS and uses Android's hostname and certificate checks.
The server must implement `/api/activity/latest` and
`/api/activity/history?limit=50`; see the [cloud API guide](../cloud/README.md).

## Recorded demonstration

The [evidence index](../docs/testing/README.md) links physical-phone observations,
API comparisons and the later Android confirmation. The saved
[Live/History screenshots](../docs/testing/2026-10-26_pytorch_android_public.md)
show the app after the PyTorch deployment. Displayed confidence is separate
from the independently measured live movement score.
