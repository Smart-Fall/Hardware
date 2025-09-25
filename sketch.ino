// I2C: SDA=21, SCL=18
// Fonctions :
// 1) Lecture pression/temperature + altitude
// 2) Moyenne glissante par fenêtres d'1s (logging propre)
// 3) Détection simple de "chute" par variation rapide d'altitude (delta négatif)

// Libraries
#include <Wire.h>
#include "SparkFunBMP384.h"  

// Capteur
BMP384 bmp;
uint8_t i2cAddress = BMP384_I2C_ADDRESS_DEFAULT; // 0x77 
const float SEA_LEVEL_HPA = 1013.25;             // à ajuster 

// Seuils (à tester et changer si pas convenable au labo)
const float LIGHT_FALL_THRESHOLD = 0.5f;   // m  (delta altitude <= -0.5 m)
const float SEVERE_FALL_THRESHOLD = 1.0f;  // m  (delta altitude <= -1.0 m)
const unsigned long FALL_CHECK_PERIOD_MS = 200; // vérif toutes les 200ms

//  Accumulateurs pour moyenne 1s 
unsigned long winStartMs = 0; //moment où la fenêtre courante a commencé
const unsigned long WIN_MS = 1000; // durée d'une fenêtre = 1000 ms = 1 s
double sumP_Pa = 0.0, sumT_C = 0.0, sumAlt_m = 0.0;
uint32_t nSamples = 0;

// Mémoire pour détection "delta altitude" 
float lastAltitude_m = NAN; //garde l’altitude précédente pour calculer la différence deltaAlt
unsigned long lastCheckMs = 0; //quand on a fait la dernière vérification (200 ms)

void setup() {
  Serial.begin(115200); //Active la communication série à 115200 bauds.
  delay(200);
  Wire.begin(21, 18); // SDA=21, SCL=18

  // Init BMP384
  while (bmp.beginI2C(i2cAddress) != BMP3_OK) {
    Serial.println("BMP384 non detecte (0x77). Essaie 0x76 si jumper ADR.");
    delay(1000);
  }
  // Filtre et OSR pour lisser sans perdre trop de réactivité
  (void)bmp.setFilterCoefficient(BMP3_IIR_FILTER_COEFF_127);
  bmp3_odr_filter_settings osr = { .press_os = BMP3_OVERSAMPLING_32X,
                                   .temp_os  = BMP3_OVERSAMPLING_2X };
  (void)bmp.setOSRMultipliers(osr);

  winStartMs = millis();
  lastCheckMs = millis();

  // En-tête CSV lisible par Serial Plotter pour les graphes
  Serial.println("time_ms,pressure_hPa,temperature_C,altitude_m,avg1s_pressure_hPa,avg1s_temperature_C,avg1s_altitude_m,verdict");
}

void loop() {
  unsigned long now = millis();

  // Lecture capteur 
  bmp3_data data = {0};
  bool ok = (bmp.getSensorData(&data) == BMP3_OK);

  float pressure_hPa = NAN, temp_C = NAN, altitude_m = NAN;
  if (ok) {
    pressure_hPa = data.pressure / 100.0f; // Pa -> hPa
    temp_C       = data.temperature;

    // Altitude baro (relative) : h = 44330 * (1 - (P/Po)^(1/5.255))
    altitude_m   = 44330.0f * (1.0f - powf(pressure_hPa / SEA_LEVEL_HPA, 0.19029495f));

    // Accumuler pour la moyenne/1s 
    sumP_Pa  += data.pressure;
    sumT_C   += temp_C;
    sumAlt_m += altitude_m;
    nSamples++;
  }

  // Détection de chute simple par delta d'altitude (toutes 200 ms) 
  String verdict = "";
  if (!isnan(altitude_m) && (now - lastCheckMs) >= FALL_CHECK_PERIOD_MS) {
    if (isnan(lastAltitude_m)) {
      lastAltitude_m = altitude_m; // 1ère référence
    }
    float deltaAlt = altitude_m - lastAltitude_m; // négatif si on "descend"
    if (deltaAlt <= -SEVERE_FALL_THRESHOLD) {
      verdict = "SEVERE_FALL";
    } else if (deltaAlt <= -LIGHT_FALL_THRESHOLD) {
      verdict = "LIGHT_FALL";
    } else {
      verdict = "NO_FALL";
    }
    lastAltitude_m = altitude_m;
    lastCheckMs = now;
  }

  // Ligne instantanée (temps réel)
  Serial.print(now); Serial.print(",");
  if (ok) {
    Serial.print(pressure_hPa, 2); Serial.print(",");
    Serial.print(temp_C, 2);       Serial.print(",");
    Serial.print(altitude_m, 2);   Serial.print(",");
  } else {
    Serial.print(",,," ); Serial.print(","); // trous si lecture KO
  }

  // Sortie des moyennes toutes 1s 
  if (now - winStartMs >= WIN_MS) {
    if (nSamples > 0) {
      float avgP_hPa = (float)(sumP_Pa / nSamples) / 100.0f;
      float avgT_C   = (float)(sumT_C  / nSamples);
      float avgAlt_m = (float)(sumAlt_m / nSamples);
      Serial.print(avgP_hPa, 2); Serial.print(",");
      Serial.print(avgT_C, 2);   Serial.print(",");
      Serial.print(avgAlt_m, 2); Serial.print(",");
    } else {
      Serial.print(",,," );
    }
    // Afficher le dernier verdict connu (si vide, on met NO_DATA)
    if (verdict.length() == 0) verdict = "NO_DATA";
    Serial.println(verdict);

    // Reset fenêtre
    sumP_Pa = sumT_C = sumAlt_m = 0.0;
    nSamples = 0;
    winStartMs = now;
  } else {
    // Complète la ligne instantanée pour rester en CSV (placeholders)
    Serial.println(",,," + (verdict.length() ? verdict : String("")));
  }

  delay(10); // ~100 Hz
}
