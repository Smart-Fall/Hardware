#include "WiFi_Manager.h"
#include "Log_Manager.h"

WiFi_Manager::WiFi_Manager()
{
    lastReconnectAttempt = 0;
    reconnectInterval = WIFI_RECONNECT_INTERVAL_MS;
    autoReconnect = false;
    initialized = false;
    reconnectFailCount = 0;
}

void WiFi_Manager::addDeviceAuthHeader(HTTPClient &http)
{
    if (strlen(DEVICE_API_KEY) > 0)
    {
        http.addHeader("X-Device-Api-Key", DEVICE_API_KEY);
    }
}

bool WiFi_Manager::isHTTPSuccess(int httpStatusCode)
{
    return httpStatusCode >= 200 && httpStatusCode < 300;
}

bool WiFi_Manager::isHTTPS(const String &url)
{
    return url.startsWith("https://") || url.startsWith("HTTPS://");
}

bool WiFi_Manager::beginHTTP(HTTPClient &http, const String &url)
{
    if (isHTTPS(url))
    {
        secureClient.setInsecure(); // Skip cert validation for self-signed/dev server
        return http.begin(secureClient, url);
    }
    else
    {
        return http.begin(plainClient, url);
    }
}

bool WiFi_Manager::begin(const char *wifi_ssid, const char *wifi_password)
{
    ssid = String(wifi_ssid);
    password = String(wifi_password);

    WiFi.mode(WIFI_STA);

    // Keep startup responsive: before first successful connection,
    // try a single quick attempt and rely on loop-based auto-reconnect later.
    const uint8_t maxAttempts = initialized ? MAX_RETRIES : 1;

    for (uint8_t attempt = 0; attempt < maxAttempts; attempt++)
    {
        Serial.print("Connecting to WiFi: ");
        Serial.print(ssid);
        if (attempt > 0)
        {
            Serial.printf(" (attempt %d/%d)", attempt + 1, maxAttempts);
        }
        Serial.println();

        WiFi.begin(wifi_ssid, wifi_password);

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED)
        {
            // Enable WiFi modem sleep to save ~60-80mA between beacon intervals
            WiFi.setSleep(WIFI_POWER_SAVE_MODE);
            Serial.println("WiFi connected successfully!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("Signal Strength (RSSI): ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            Serial.print("WiFi power save: ");
            Serial.println(WIFI_POWER_SAVE_MODE == WIFI_PS_MAX_MODEM ? "MAX_MODEM" : "MIN_MODEM");
            initialized = true;
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_WIFI, "WiFi connected");
            }
            return true;
        }

        if (attempt < maxAttempts - 1)
        {
            Serial.printf("[WiFi] Connection failed, retry %d/%d...\n", attempt + 1, maxAttempts);
            WiFi.disconnect();
            delay(RETRY_DELAY_MS);
        }
    }

    Serial.println("[WiFi] Failed to connect - all retries failed");
    initialized = false;
    if (logManager.isReady())
    {
        logManager.log(LOG_LEVEL_ERROR, LOG_CAT_WIFI, "WiFi connection failed");
    }
    return false;
}

void WiFi_Manager::setServerURL(const char *url)
{
    serverURL = String(url);
    Serial.print("Server URL set to: ");
    Serial.println(serverURL);
}

void WiFi_Manager::enableAutoReconnect(bool enable)
{
    autoReconnect = enable;
    Serial.print("Auto-reconnect: ");
    Serial.println(enable ? "Enabled" : "Disabled");
}

bool WiFi_Manager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

void WiFi_Manager::checkConnection()
{
    if (!autoReconnect)
        return;

    if (!isConnected())
    {
        // After WIFI_MAX_RECONNECT_ATTEMPTS consecutive failures, back off to long interval
        unsigned long interval = (reconnectFailCount >= WIFI_MAX_RECONNECT_ATTEMPTS)
                                     ? WIFI_RECONNECT_LONG_INTERVAL_MS
                                     : WIFI_RECONNECT_INTERVAL_MS;

        unsigned long currentTime = millis();
        if (currentTime - lastReconnectAttempt >= interval)
        {
            lastReconnectAttempt = currentTime;
            Serial.printf("\n[WiFi] Connection lost. Reconnect attempt (fail streak: %d)...\n", reconnectFailCount);
            if (reconnect())
            {
                reconnectFailCount = 0;
            }
            else
            {
                reconnectFailCount++;
                if (reconnectFailCount >= WIFI_MAX_RECONNECT_ATTEMPTS)
                {
                    Serial.printf("[WiFi] %d consecutive failures — backing off to %lu s interval\n",
                                  reconnectFailCount, WIFI_RECONNECT_LONG_INTERVAL_MS / 1000UL);
                }
            }
        }
    }
    else
    {
        // Connected — reset failure streak
        if (reconnectFailCount > 0)
        {
            reconnectFailCount = 0;
        }
    }
}

bool WiFi_Manager::reconnect()
{
    WiFi.disconnect();
    delay(100);
    return begin(ssid.c_str(), password.c_str());
}

bool WiFi_Manager::sendTestMessage(const String &message)
{
    if (!isConnected())
    {
        Serial.println("Error: WiFi not connected!");
        return false;
    }

    if (serverURL.length() == 0)
    {
        Serial.println("Error: Server URL not set!");
        return false;
    }

    Serial.print("Sending test message to: ");
    Serial.println(serverURL);

    for (uint8_t attempt = 0; attempt < HTTP_MAX_RETRIES; attempt++)
    {
        HTTPClient http;
        beginHTTP(http, serverURL);
        http.addHeader("Content-Type", "text/plain");
        addDeviceAuthHeader(http);

        int httpResponseCode = http.POST(message);

        if (isHTTPSuccess(httpResponseCode))
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.print("Server response: ");
            Serial.println(response);
            http.end();
            return true;
        }

        Serial.printf("[WiFi] sendTestMessage HTTP status %d on attempt %d/%d\n", httpResponseCode, attempt + 1, HTTP_MAX_RETRIES);
        Serial.printf("[WiFi] sendTestMessage error %d on attempt %d/%d\n", httpResponseCode, attempt + 1, HTTP_MAX_RETRIES);
        http.end();

        if (attempt < HTTP_MAX_RETRIES - 1)
        {
            delay(HTTP_RETRY_DELAY_MS);
        }
    }

    Serial.println("[WiFi] sendTestMessage failed - all retries failed");
    return false;
}

bool WiFi_Manager::sendJSON(const String &jsonPayload)
{
    if (!isConnected())
    {
        Serial.println("Error: WiFi not connected!");
        return false;
    }

    if (serverURL.length() == 0)
    {
        Serial.println("Error: Server URL not set!");
        return false;
    }

    Serial.println("Sending JSON payload:");
    Serial.println(jsonPayload);

    for (uint8_t attempt = 0; attempt < HTTP_MAX_RETRIES; attempt++)
    {
        HTTPClient http;
        beginHTTP(http, serverURL);
        http.addHeader("Content-Type", "application/json");
        addDeviceAuthHeader(http);

        int httpResponseCode = http.POST(jsonPayload);

        if (isHTTPSuccess(httpResponseCode))
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.print("Server response: ");
            Serial.println(response);
            http.end();
            return true;
        }

        Serial.printf("[WiFi] sendJSON HTTP status %d on attempt %d/%d\n", httpResponseCode, attempt + 1, HTTP_MAX_RETRIES);
        Serial.printf("[WiFi] sendJSON error %d on attempt %d/%d\n", httpResponseCode, attempt + 1, HTTP_MAX_RETRIES);
        http.end();

        if (attempt < HTTP_MAX_RETRIES - 1)
        {
            delay(HTTP_RETRY_DELAY_MS);
        }
    }

    Serial.println("[WiFi] sendJSON failed - all retries failed");
    return false;
}

void WiFi_Manager::printConnectionInfo()
{
    Serial.println("\n=== WiFi Connection Info ===");
    Serial.print("Status: ");
    Serial.println(isConnected() ? "Connected" : "Disconnected");

    if (isConnected())
    {
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());
        Serial.print("Subnet Mask: ");
        Serial.println(WiFi.subnetMask());
        Serial.print("DNS: ");
        Serial.println(WiFi.dnsIP());
        Serial.print("Signal Strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("MAC Address: ");
        Serial.println(WiFi.macAddress());
    }
    Serial.println("============================\n");
}

bool WiFi_Manager::sendJSONToEndpoint(const String &path, const String &jsonPayload)
{
    if (!isConnected())
    {
        Serial.println("Error: WiFi not connected!");
        return false;
    }

    if (serverURL.length() == 0)
    {
        Serial.println("Error: Server URL not set!");
        return false;
    }

    String fullURL = serverURL + path;
    Serial.print("Sending JSON to: ");
    Serial.println(fullURL);

    for (uint8_t attempt = 0; attempt < HTTP_MAX_RETRIES; attempt++)
    {
        HTTPClient http;
        beginHTTP(http, fullURL);
        http.addHeader("Content-Type", "application/json");
        addDeviceAuthHeader(http);

        int httpResponseCode = http.POST(jsonPayload);

        if (isHTTPSuccess(httpResponseCode))
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.print("Server response: ");
            Serial.println(response);
            http.end();
            return true;
        }

        Serial.printf("[WiFi] sendJSONToEndpoint HTTP status %d on attempt %d/%d\n", httpResponseCode, attempt + 1, HTTP_MAX_RETRIES);
        if (logManager.isReady())
        {
            logManager.log(LOG_LEVEL_WARN, LOG_CAT_WIFI, "HTTP send error",
                           (float)httpResponseCode, (float)(attempt + 1));
        }
        http.end();

        if (attempt < HTTP_MAX_RETRIES - 1)
        {
            delay(HTTP_RETRY_DELAY_MS);
        }
    }

    Serial.println("[WiFi] sendJSONToEndpoint failed - all retries failed");
    return false;
}

String WiFi_Manager::getFromEndpoint(const String &path)
{
    if (!isConnected() || serverURL.length() == 0)
        return "";

    String fullURL = serverURL + path;

    for (uint8_t attempt = 0; attempt < HTTP_MAX_RETRIES; attempt++)
    {
        HTTPClient http;
        beginHTTP(http, fullURL);
        addDeviceAuthHeader(http);

        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            String response = http.getString();
            http.end();
            return response;
        }

        http.end();
        if (attempt < HTTP_MAX_RETRIES - 1)
            delay(HTTP_RETRY_DELAY_MS);
    }

    return "";
}

String WiFi_Manager::getLocalIP()
{
    return WiFi.localIP().toString();
}

int WiFi_Manager::getRSSI()
{
    return WiFi.RSSI();
}
