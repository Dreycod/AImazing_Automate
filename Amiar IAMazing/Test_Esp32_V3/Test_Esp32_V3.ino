#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"

#define RXD2 13  // RX ESP32 ← TX Mega
#define TXD2 14  // TX ESP32 → RX Mega


//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE  // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_CAMS3_UNIT  // Has PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

// ===========================
//WiFi credentials
// ===========================
const char *ssid_Cam = "SFR_5888"; //TP-Link_8E03
const char *password_Cam = "7znp37zxfx6jxu25sbvi"; //23697140

void startCameraServer();
void setupLedFlash(int pin);


// === Configuration API Supabase ===
const char* supabase_url = "https://aozgpljtorqbhvlfhbnn.supabase.co";
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImFvemdwbGp0b3JxYmh2bGZoYm5uIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDk2MjY0NjgsImV4cCI6MjA2NTIwMjQ2OH0.jxq8p3cbzOFxn1fRjvQ5biliRCRU1CBQRFpeaGhB4OY";  // ta clé "anon" publique

// === Configuration API ===
const char* apiBaseURL = "http://este.alwaysdata.net";

const char* loginEndpoint = "/login";

const char* GetAllTestEndPoints = "/getAllTest";
const char* AddTestEndPoints = "/addTest";

const char* AddMouvementsEndPoints = "/addMouvement";

const char* GetModelIdValideEndPoint = "/getAllModelValide";
const char* GetModelByIdEndPoint = "/getModelById/";

const char* AddTestEndPoint = "/addTest";

int test_id_api = -1;
int test_id_supabase = -1;
int modelIdValide = -1;  // 💡 Important pour les mouvements

int InputRobot[7] = {0};
int inputSize = sizeof(InputRobot) / sizeof(InputRobot[0]);  // calcule 7 ici

bool readyForPrediction = false;


// === Données d'identification ===
const char* username = "darklion84";
const char* pass = "N4rT7kA2vL9pQwX3";

String authToken = "";



void connectToWiFi() {
  Serial.print("Connexion à WiFi");
  WiFi.begin(ssid_Cam, password_Cam);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connecté au WiFi !");
  Serial.println(WiFi.localIP());
} 

bool loginToAPI() {
  HTTPClient http;
  String url = String(apiBaseURL) + loginEndpoint;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"username\":\"" + String(username) + "\",\"password\":\"" + String(pass) + "\"}";
  int httpResponseCode = http.POST(body);

  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.println("🔐 Login réussi : " + payload);

    int start = payload.indexOf(":\"") + 2;
    int end = payload.indexOf("\"", start);
    authToken = payload.substring(start, end);

    http.end();
    return true;
  } else {
    Serial.print("❌ Echec de login : ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

void getAllTestData() {
  if (authToken == "") {
    Serial.println("❗ Pas de token valide");
    return;
  }

  HTTPClient http;
  String url = String(apiBaseURL) + GetAllTestEndPoints ;
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + authToken);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    String data = http.getString();
    Serial.println("📦 Données reçues : " + data);
  } else {
    Serial.print("❌ Erreur GET : ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void getModelIdValide() {
  if (authToken == "") {
    Serial.println("❗ Pas de token valide");
    return;
  }

  HTTPClient http;
  String url = String(apiBaseURL) + GetModelIdValideEndPoint;
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + authToken);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    String data = http.getString();
    Serial.println("✅ Test créé avec succès : " + data);

    // Extraire l'ID depuis le JSON
    int valeurIndex = data.indexOf("\"valeur\":") + 9;
    int fin = data.indexOf("}", valeurIndex);
    String idStr = data.substring(valeurIndex, fin); 

    idStr.trim();

    int idModel = idStr.toInt();
    modelIdValide = idModel;


    Serial.print("✅ ID modèle = ");
    Serial.println(idModel);


    Serial.print("🆔 test_id confirmé après création = ");
    Serial.println(test_id_api);

    // Ensuite on récupère les poids
    getModelDataById(idModel);

  } else {
    Serial.print("❌ Erreur GET id modèle : ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void createNewTest(int model_id_valide) {
  if (authToken == "") {
    Serial.println("❗ Token manquant, impossible de créer un test.");
    return;
  }

  HTTPClient http;
  String url = String(apiBaseURL) + AddTestEndPoint;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + authToken);

  String body = "{\"nom\":\"TestAuto_" + String(random(1000)) + "\","
                "\"n_mouvements\":0,"
                "\"distance_total\":0,"
                "\"t_deplacement\":0,"
                "\"model_id\":" + String(model_id_valide) + "}";

  Serial.println("📤 Envoi test AlwaysData avec body : " + body);

  int httpResponseCode = http.POST(body);
  String response = http.getString();

  if (httpResponseCode == 200 || httpResponseCode == 201) {
    Serial.println("🧪 Test ajouté avec succès !");
    Serial.println("🧾 Réponse (sans id) : " + response);

    delay(1000); // ⏱️ Donne un petit délai au serveur
    getLastTestId(); // ✅ Récupère l’ID le plus élevé
  } else {
    Serial.print("❌ Erreur POST création test : ");
    Serial.println(httpResponseCode);
    Serial.println("🧾 Réponse serveur : " + response);
  }

  http.end();
}

void getLastTestId() {
  if (authToken == "") {
    Serial.println("❗ Token manquant pour getLastTestId");
    return;
  }

  HTTPClient http;
  String url = String(apiBaseURL) + "/getIdTest";
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + authToken);

  int code = http.GET();
  if (code == 200) {
    String response = http.getString();
    Serial.println("📦 IDs reçus : " + response);

    int lastId = -1;
    int pos = 0;

    while (true) {
      int index = response.indexOf("\"id\":", pos);
      if (index == -1) break;

      int start = index + 5;
      int end = response.indexOf("}", start);
      String idStr = response.substring(start, end);
      int id = idStr.toInt();

      if (id > lastId) lastId = id;

      pos = end + 1;
    }

    if (lastId > 0) {
      test_id_api = lastId;
      Serial.print("✅ ID du dernier test enregistré : ");
      Serial.println(test_id_api);
    } else {
      Serial.println("❌ Aucun ID trouvé dans la réponse");
    }

  } else {
    Serial.print("❌ Erreur GET /getIdTest : ");
    Serial.println(code);
  }

  http.end();
}



void getModelDataById(int modelId) {
  if (authToken == "") {
    Serial.println("❗ Pas de token valide");
    return;
  }

  HTTPClient http;
  String url = String(apiBaseURL) + GetModelByIdEndPoint + String(modelId);
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + authToken);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    String data = http.getString();
    Serial.println("📦 Modèle récupéré : " + data);
    parseModelAndExtractWeights(data);

    // Ici tu peux filtrer les poids IH et HO avec FilterMatrixString
    // (à faire selon ton format exact)

  } else {
    Serial.print("❌ Erreur GET modèle par ID : ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
double AIPrediction(int* InputRobot, int INPUT_SIZE, int HIDDEN_SIZE, double** weights_ih, double** weights_ho)
{
  double *hidden = (double *)malloc(HIDDEN_SIZE * sizeof(double));
  for (int j = 0; j < HIDDEN_SIZE; j++) {
      double sum = 0.0;
      for (int k = 0; k < INPUT_SIZE; k++) {
          sum += InputRobot[k] * weights_ih[j][k];
      }
      hidden[j] = sigmoid(sum);
  }

  double output = 0.0;
  for (int j = 0; j < HIDDEN_SIZE; j++) {
      output += hidden[j] * weights_ho[0][j];
  }
  output = sigmoid(output);
  output = round(output * 4) / 4.0; /* ATTENTION, si valeur chelou, retirer la ligne
                                    ou changer les valeur */
  return output;

}

double sigmoid(double sum)
{
  return 1.0 / (1.0 + exp(-sum));
}

// Variables globales nécessaires
double** weights_ih_matrix;
double** weights_ho_matrix;
int IH_ROWS, IH_COLS;
int HO_ROWS, HO_COLS;
int INPUT_SIZE = 0;
int HIDDEN_SIZE = 0;

void parseModelAndExtractWeights(String json) {
  // === Extraction brute des chaînes JSON ===
  int ihStart = json.indexOf("\"weights_ih\":\"") + 14;
  int ihEnd = json.indexOf("\"", ihStart);
  String weightsIH = json.substring(ihStart, ihEnd);
  weightsIH.replace("\\", "");  // Nettoyage des échappements JSON

  int hoStart = json.indexOf("\"weights_ho\":\"") + 14;
  int hoEnd = json.indexOf("\"", hoStart);
  String weightsHO = json.substring(hoStart, hoEnd);
  weightsHO.replace("\\", "");  // Nettoyage des slashs

  // === Affichage brut pour vérification ===
  Serial.println("📡 Weights IH : " + weightsIH);
  Serial.println("📡 Weights HO : " + weightsHO);

   int rowsIH, colsIH;
  weights_ih_matrix = FilterMatrixString(weightsIH, &rowsIH, &colsIH);

  int rowsHO, colsHO;
  weights_ho_matrix = FilterMatrixString(weightsHO, &rowsHO, &colsHO);

  // Mise à jour dynamique
  HIDDEN_SIZE = rowsIH;
  INPUT_SIZE = colsIH;

  Serial.print("📐 INPUT_SIZE = ");
  Serial.println(INPUT_SIZE);
  Serial.print("📐 HIDDEN_SIZE = ");
  Serial.println(HIDDEN_SIZE);

  // ✅ Activer le drapeau
  //readyForPrediction = true;
  Serial.println(readyForPrediction);

}

#include <Arduino.h>

int GLOBALweightSize = 0;

double** FilterMatrixString(String ReceivedArray, int* rows, int* cols) {
  String StringArray[20];  // suppose un max de 20 lignes
  int rowCount = 0;

  // Découper la chaîne d'entrée en sous-chaînes délimitées par "],"
  int start = 0;
  int end = ReceivedArray.indexOf("],");
  while (end != -1) {
    StringArray[rowCount++] = ReceivedArray.substring(start, end + 1);
    start = end + 2;
    end = ReceivedArray.indexOf("],", start);
  }
  StringArray[rowCount++] = ReceivedArray.substring(start);  // dernière ligne

  *rows = rowCount;

  // Supposer que toutes les lignes ont le même nombre de colonnes
  String firstLine = StringArray[0];
  firstLine.replace("[", "");
  firstLine.replace("]", "");
  int colCount = 1;
  for (int i = 0; i < firstLine.length(); i++) {
    if (firstLine.charAt(i) == ',') colCount++;
  }
  *cols = colCount;
  GLOBALweightSize = colCount;

  // Allocation dynamique
  double** array = (double**)malloc(rowCount * sizeof(double*));
  for (int i = 0; i < rowCount; i++) {
    array[i] = (double*)malloc(colCount * sizeof(double));

    StringArray[i].replace("[", "");
    StringArray[i].replace("]", "");

    int lastIndex = 0;
    int nextIndex = StringArray[i].indexOf(',');

    for (int j = 0; j < colCount; j++) {
      String value;
      if (nextIndex != -1) {
        value = StringArray[i].substring(lastIndex, nextIndex);
        lastIndex = nextIndex + 1;
        nextIndex = StringArray[i].indexOf(',', lastIndex);
      } else {
        value = StringArray[i].substring(lastIndex);
      }

      array[i][j] = value.toFloat();
    }
  }

  return array;
}

int GetWeightSize() {
  return GLOBALweightSize;
}


void parseSerialToInputRobot(String msg) {
  msg.trim();

if (!msg.startsWith("{") || !msg.endsWith("}")) {
  Serial.print("❌ Format incorrect ignoré : ");
  Serial.println(msg);
  return;
}

  Serial.println("🛰️ Message brut reçu : " + msg);  // Garde l'affichage brut

  // Prépare une copie nettoyée pour l'extraction des valeurs
  String cleanMsg = msg;
  cleanMsg.replace("{", "");
  cleanMsg.replace("}", "");

  int i = 0;
  int lastIndex = 0;
  while (i < 7) {
    int commaIndex = cleanMsg.indexOf(',', lastIndex);
    String value;

    if (commaIndex != -1) {
      value = cleanMsg.substring(lastIndex, commaIndex);
      lastIndex = commaIndex + 1;
    } else {
      value = cleanMsg.substring(lastIndex);
    }

    InputRobot[i] = value.toInt();
    i++;
  }

  printInputRobot();  // Pour affichage debug
  readyForPrediction = true;
}

void printInputRobot() {
  Serial.print("📦 InputRobot = {");
  for (int i = 0; i < 7; i++) {
    Serial.print(InputRobot[i]);
    if (i < 6) Serial.print(", ");
  }
  Serial.println("}");
}


void sendMouvementToAPI(int direction_id, int test_id, int test_model_id, int distance = 5, int temps = 5) {
  if (authToken == "") {
    Serial.println("❗ Token manquant");
    return;
  }

  HTTPClient http;
  String url = String(apiBaseURL) + AddMouvementsEndPoints;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + authToken);

  String body = "{";
  body += "\"distance\":" + String(distance) + ",";
  body += "\"temps\":" + String(temps) + ",";
  body += "\"direction_id\":" + String(direction_id) + ",";
  body += "\"test_id\":" + String(test_id) + ",";  // testId doit être une variable globale récupérée via addTest
  body += "\"test_model_id\":" + String(test_model_id);
  body += "}";

  int code = http.POST(body);
  if (code == 200) {
    Serial.println("📤 Mouvement enregistré avec succès");
  } else {
    Serial.print("❌ Erreur POST mouvement : ");
    Serial.println(code);
  }

  Serial.print("📌 Direction ID envoyée : ");
  Serial.println(direction_id);
  Serial.print("🧪 test_id = ");
  Serial.println(test_id);
  Serial.print("🔗 model_id = ");
  Serial.println(test_model_id);

  http.end();
}


void sendTestToSupabase() {
  HTTPClient http;
  String url = String(supabase_url) + "/rest/v1/tests";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=representation");
  http.addHeader("apikey", supabase_key);
  http.addHeader("Authorization", "Bearer " + String(supabase_key));

  String body = "{\"nom\":\"TestAuto ESP32\",\"description\":\"AutoStart\",\"model_id\":21,\"total_mouvements\":0,\"distance_totale\":\"0\",\"temps_total\":\"0\"}";

  int code = http.POST(body);
  if (code == 201) {
  Serial.println("✅ Test ajouté à Supabase !");
  String payload = http.getString();
  Serial.println("🧾 Réponse : " + payload);

  // ✅ Extraction correcte de l'ID (attention : retour Supabase = tableau JSON)
  int idStart = payload.indexOf("\"id\":") + 5;
  int idEnd = payload.indexOf(",", idStart);
  if (idStart > 4 && idEnd > idStart) {
    String idStr = payload.substring(idStart, idEnd);
    idStr.trim();
    test_id_supabase = idStr.toInt(); // ← nouvelle variable dédiée à Supabase
    Serial.print("🆔 test_id_supabase = ");
    Serial.println(test_id_supabase);
  } else {
    Serial.println("❌ Impossible d'extraire l'id depuis Supabase !");
  }



  } else {
    Serial.print("❌ Erreur POST Test : ");
    Serial.println(code);
  }

  http.end();
}

void sendMouvementToSupabase(String tag, int direction, int test_id) {
  HTTPClient http;
  String url = String(supabase_url) + "/rest/v1/test_details";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabase_key);
  http.addHeader("Authorization", "Bearer " + String(supabase_key));

  // 🔄 Conversion tableau -> chaîne
  String matrixStr = "";
  for (int i = 0; i < 7; i++) {
    matrixStr += String(InputRobot[i]);
    if (i < 6) matrixStr += ",";
  }

  // 📦 Construction du JSON
  String body = "{";
  body += "\"test_id\":" + String(test_id) + ",";
  body += "\"input_matrix\":\"" + matrixStr + "\",";
  body += "\"tag_prevision\":\"" + tag + "\",";
  body += "\"direction_id\":" + String(direction) + ",";
  body += "\"distance\":\"33\",";
  body += "\"temps\":\"4\"";
  body += "}";

  int code = http.POST(body);
  if (code == 201) {
    Serial.println("✅ Mouvement enregistré dans Supabase");
  } else {
    Serial.print("❌ Erreur POST mouvement : ");
    Serial.println(code);
    Serial.println("🧾 Réponse serveur : " + http.getString());
  }

  http.end();
}

void setup() {

/***************** Demarage des ecoutes des serial *****************/
  Serial.begin(9600); // Pour monitor USB
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.setTimeout(2000);
  Serial.println("🛰️ ESP32 en écoute...");


Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid_Cam, password_Cam);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
/***************** Connection a la wifi *****************/
  connectToWiFi();
  // Faire le login une fois
  if (loginToAPI()) {
    Serial.println("🔓 Authentifié, prêt à faire des requêtes.");
  }

  /***************** Récupération modèle IA *****************/
  getModelIdValide();

  /***************** Création du test sur AlwaysData *****************/
  createNewTest(modelIdValide);

  /***************** Creation Test SupaBase *****************/
  sendTestToSupabase(); // ➕ test dans Supabase au démarrage
  delay(2000); 
  


}

void loop() 
{
  if (Serial2.available()) 
    {
      String msg = Serial2.readStringUntil('\n');
      msg.trim(); // très important

      if (msg.length() == 0) return; // Ignore les lignes vides

      Serial.println(msg);
      parseSerialToInputRobot(msg);


      if (readyForPrediction) 
        {
          Serial.println(readyForPrediction);
          double tag = AIPrediction(InputRobot, inputSize, HIDDEN_SIZE, weights_ih_matrix, weights_ho_matrix);
          Serial.print("📊 Prédiction IA tag : ");
          Serial.println(tag);

            // ✉️ Envoi du tag à la Mega
          String messageToMega = "TAG:" + String(tag, 2) + "\n";  // Ex : TAG:0.75
          Serial2.print(messageToMega);
          Serial.print("📤 Envoyé à Mega : ");
          Serial.println(messageToMega);  

              // ✅ Enregistrement mouvement dans la BDD
          int direction_id = 0;
          if (tag == 0.0) direction_id = 3;      // haut
          else if (tag == 0.25) direction_id = 1; // droite
          else if (tag == 0.5) direction_id = 4;  // bas
          else if (tag == 0.75) direction_id = 2; // gauche

          sendMouvementToAPI(direction_id, test_id_api, modelIdValide);

          // ➕ Envoi dans Supabase
          String inputMatrixStr = "";
          for (int i = 0; i < 7; i++) {
            inputMatrixStr += String(InputRobot[i]);
            if (i < 6) inputMatrixStr += ",";
          }
          String tagStr = String(tag, 2);

          Serial.print("🎯 test_id_supabase utilisé : ");
          Serial.println(test_id_supabase);
          sendMouvementToSupabase(tagStr, direction_id, test_id_supabase);

          readyForPrediction = false; // ❗ très important pour ne pas faire la prédiction en boucle infinie

        }
  }

}