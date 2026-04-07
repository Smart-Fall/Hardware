#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#include <Preferences.h>
#include "Config.h"  // for fallback WIFI_SSID / WIFI_PASSWORD defines

#define NVS_NAMESPACE "wifi_creds"
#define NVS_KEY_SSID  "ssid"
#define NVS_KEY_PASS  "pass"

class WiFi_Credentials {
public:
    // Load SSID and password from NVS. If nothing is stored, use compile-time fallbacks.
    // Returns true if NVS had stored values; false = compile-time fallbacks used
    static bool load(char* ssidBuf, size_t ssidLen, char* passBuf, size_t passLen) {
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, true);  // read-only
        String storedSSID = prefs.getString(NVS_KEY_SSID, "");
        String storedPass = prefs.getString(NVS_KEY_PASS, "");
        prefs.end();

        if (storedSSID.length() == 0) {
            // Nothing stored — use compile-time fallbacks
            strncpy(ssidBuf, WIFI_SSID,    ssidLen - 1);
            strncpy(passBuf, WIFI_PASSWORD, passLen - 1);
            ssidBuf[ssidLen - 1] = '\0';
            passBuf[passLen - 1] = '\0';
            return false;
        }

        strncpy(ssidBuf, storedSSID.c_str(), ssidLen - 1);
        strncpy(passBuf, storedPass.c_str(), passLen - 1);
        ssidBuf[ssidLen - 1] = '\0';
        passBuf[passLen - 1] = '\0';
        return true;
    }

    // Save credentials to NVS. Returns true on success.
    static bool save(const char* ssid, const char* password) {
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, false);  // read-write
        bool ok = prefs.putString(NVS_KEY_SSID, ssid) &&
                  prefs.putString(NVS_KEY_PASS, password);
        prefs.end();
        return ok;
    }

    // Read back SSID only (for identify response — do NOT expose password).
    static String getSavedSSID() {
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, true);
        String s = prefs.getString(NVS_KEY_SSID, String(WIFI_SSID));
        prefs.end();
        return s;
    }

    // Erase stored credentials (factory reset helper).
    static void clear() {
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, false);
        prefs.clear();
        prefs.end();
    }
};

#endif // WIFI_CREDENTIALS_H
