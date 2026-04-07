#include "Fall_Store.h"
#include <LittleFS.h>

bool Fall_Store::begin()
{
    if (!LittleFS.begin(true))  // true = format partition if mount fails (first boot)
    {
        Serial.println("[FallStore] ERROR: LittleFS mount failed");
        return false;
    }
    Serial.printf("[FallStore] Mounted — %d fall(s) pending\n", pendingCount());
    return true;
}

String Fall_Store::filename(uint32_t ts)
{
    return "/fall_" + String(ts) + ".json";
}

bool Fall_Store::save(const EmergencyData_t& data)
{
    if (pendingCount() >= FALL_STORE_MAX_PENDING)
    {
        Serial.println("[FallStore] Queue full — oldest fall will be overwritten");
        // Remove the oldest file to make room
        File root = LittleFS.open("/");
        File f = root.openNextFile();
        String oldest;
        while (f)
        {
            String name = f.name();
            if (name.startsWith("fall_") && (oldest.isEmpty() || name < oldest))
                oldest = name;
            f = root.openNextFile();
        }
        if (!oldest.isEmpty())
            LittleFS.remove("/" + oldest);
    }

    File f = LittleFS.open(filename(data.timestamp), "w");
    if (!f)
    {
        Serial.println("[FallStore] ERROR: Could not open file for write");
        return false;
    }

    // Store only the fields needed to reconstruct the API payload
    f.printf(
        "{\"ts\":%lu,\"cs\":%d,\"cl\":%d,\"bl\":%.1f,\"sos\":%s,\"did\":\"%s\"}",
        (unsigned long)data.timestamp,
        data.confidence_score,
        (int)data.confidence,
        data.battery_level,
        data.sos_triggered ? "true" : "false",
        data.device_id
    );
    f.close();

    Serial.printf("[FallStore] Fall saved: %s\n", filename(data.timestamp).c_str());
    return true;
}

uint8_t Fall_Store::loadPending(EmergencyData_t* buf, uint8_t maxCount)
{
    uint8_t count = 0;
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) return 0;

    File f = root.openNextFile();
    while (f && count < maxCount)
    {
        String name = f.name();
        if (name.startsWith("fall_"))
        {
            String json = f.readString();
            f.close();

            EmergencyData_t e = {};

            // Manual JSON field extraction (no library dependency)
            auto extractInt = [&](const char* key) -> long {
                String k = String("\"") + key + "\":";
                int idx = json.indexOf(k);
                if (idx < 0) return 0;
                return json.substring(idx + k.length()).toInt();
            };
            auto extractFloat = [&](const char* key) -> float {
                String k = String("\"") + key + "\":";
                int idx = json.indexOf(k);
                if (idx < 0) return 0.0f;
                return json.substring(idx + k.length()).toFloat();
            };

            e.timestamp       = (uint32_t)extractInt("ts");
            e.confidence_score = (uint8_t)extractInt("cs");
            e.confidence      = (FallConfidence_t)extractInt("cl");
            e.battery_level   = extractFloat("bl");

            int sosIdx = json.indexOf("\"sos\":");
            if (sosIdx >= 0)
                e.sos_triggered = json.substring(sosIdx + 6, sosIdx + 10) == "true";

            int didIdx = json.indexOf("\"did\":\"");
            if (didIdx >= 0)
            {
                int end = json.indexOf("\"", didIdx + 7);
                if (end > didIdx + 7)
                    strncpy(e.device_id, json.substring(didIdx + 7, end).c_str(),
                            sizeof(e.device_id) - 1);
            }

            buf[count++] = e;
            f = root.openNextFile();
            continue;
        }
        f.close();
        f = root.openNextFile();
    }

    return count;
}

bool Fall_Store::remove(uint32_t timestamp)
{
    String fn = filename(timestamp);
    if (!LittleFS.exists(fn)) return true;  // already gone
    bool ok = LittleFS.remove(fn);
    Serial.printf("[FallStore] %s: %s\n", ok ? "Deleted" : "Failed to delete", fn.c_str());
    return ok;
}

bool Fall_Store::hasPending()
{
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) return false;
    File f = root.openNextFile();
    while (f)
    {
        if (String(f.name()).startsWith("fall_")) return true;
        f = root.openNextFile();
    }
    return false;
}

uint8_t Fall_Store::pendingCount()
{
    uint8_t count = 0;
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) return 0;
    File f = root.openNextFile();
    while (f)
    {
        if (String(f.name()).startsWith("fall_")) count++;
        f = root.openNextFile();
    }
    return count;
}

void Fall_Store::clear()
{
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) return;
    File f = root.openNextFile();
    while (f)
    {
        String name = f.name();
        f.close();
        if (name.startsWith("fall_"))
            LittleFS.remove("/" + name);
        f = root.openNextFile();
    }
}
