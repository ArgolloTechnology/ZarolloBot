// Sayan Electronics - Código convertido para FreeRTOS (ESP32)
// -----------------------------

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#include <SPI.h>
#include <TFT_eSPI.h>  

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Posições
int leftEyeX = 35;
int spritesUpX = 70;
int eyeY = 18;
int eyeYoffR = 0;
int eyeYoffL = 5;
int eyeWidthR = 25;
int eyeWidthL = 29;
int eyeHeightR = 30;
int eyeHeightL = 20;
static int offsetX = 0;
static int offsetY = 0;

// Movimentação
int targetOffsetX = 0;
int targetOffsetY = 0;
int moveSpeed = 5;

// Piscar
int blinkState = 0;  
int blinkDelay = 4000;
unsigned long lastBlinkTime = 0;
unsigned long moveTime = 0;

unsigned int happiness = 0;

SemaphoreHandle_t xDisplayMutex;

// Protótipo da task
void TaskEyeDraw(void *pvParameters);
void TaskEyeBlink(void *pvParameters);
void TaskEyeMoviment(void *pvParameters);
void TaskMoodManager(void *pvParameters);
void TaskEyeDrawTFT(void *pvParameters);
// ----------------------------------------------------------------

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.begin(115200);
  display.clearDisplay();
  display.display();
  delay(500);

  xDisplayMutex = xSemaphoreCreateMutex();

  // Criar task de animação
  xTaskCreate(TaskEyeMoviment, "EyeMoviment", 4096, NULL, 1, NULL);
  xTaskCreate(TaskEyeBlink, "EyeBlink", 4096, NULL, 1, NULL);
  xTaskCreate(TaskEyeDraw, "EyeDraw", 4096, NULL, 3, NULL);
  xTaskCreate(TaskMoodManager, "Mood", 4096, NULL, 2, NULL);
  xTaskCreate(TaskEyeDrawTFT, "TFT", 4096, NULL, 3, NULL);
}

void loop() {
  // Nada — FreeRTOS controla tudo agora
}

void TaskEyeMoviment(void *pvParameters) {
  for (;;) {

    unsigned long currentTime = millis();

    if (currentTime - moveTime > random(1500, 3000) && blinkState == 0) {

      //if (xSemaphoreTake(xDisplayMutex, 5)) {

        int movementType = random(0, 8);

        switch (movementType) {
          case 0: targetOffsetX = -16; targetOffsetY = 0; break;
          case 1: targetOffsetX = 16;  targetOffsetY = 0; break;
          case 2: targetOffsetX = -16; targetOffsetY = -8; break;
          case 3: targetOffsetX = 16;  targetOffsetY = -8; break;
          case 4: targetOffsetX = -16; targetOffsetY = 8; break;
          case 5: targetOffsetX = 16;  targetOffsetY = 8; break;
          default: targetOffsetX = 0; targetOffsetY = 0; break;
        }

        //xSemaphoreGive(xDisplayMutex);
      //}

      moveTime = currentTime;
    }

    offsetX += (targetOffsetX - offsetX) * 0.2;
    offsetY += (targetOffsetY - offsetY) * 0.2;

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskEyeBlink(void *pvParameters) {
  for (;;) {
    unsigned long currentTime = millis();
    // ----- PISCAR -----
    if (currentTime - lastBlinkTime > blinkDelay && blinkState == 0) {
      blinkState = 1;
      lastBlinkTime = currentTime;
    } 
    else if (currentTime - lastBlinkTime > 150 && blinkState == 1) {
      blinkState = 0;
      lastBlinkTime = currentTime;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskMoodManager(void *pvParameters) {
  for (;;) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    animateHappiness(100,1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    animateHappiness(0,10);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    animateHappiness(50,1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    animateHappiness(0,5);
  }
}

void TaskEyeDraw(void *pvParameters) {
  for (;;) {

    if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY)) {
      display.clearDisplay();
      if (blinkState == 0) {
        drawEye(leftEyeX + offsetX, eyeYoffL + eyeY + offsetY, eyeWidthL, eyeHeightL);
        drawEye(spritesUpX + offsetX, eyeYoffR + eyeY + offsetY, eyeWidthR, eyeHeightR);
      } else {
        display.fillRect(leftEyeX + offsetX,  eyeYoffL + eyeY + offsetY + eyeHeightL/2 - 2, eyeWidthL, 4, WHITE);
        display.fillRect(spritesUpX + offsetX, eyeYoffR + eyeY + offsetY + eyeHeightR/2 - 2, eyeWidthR, 4, WHITE);
      }
      display.display();
      xSemaphoreGive(xDisplayMutex);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void drawEye(int eyeX, int eyeY, int eyeWidth, int eyeHeight) {
  display.fillRoundRect(eyeX, eyeY, eyeWidth, eyeHeight, 5, WHITE);
  // --- Happy curve ---
  float h = happiness / 100.0;         // 0.0 a 1.0
  int radius = h * (eyeHeight / 2);    // 0 → meio do olho | 100 → max

  // A curva deve começar do fundo do olho
  int ellipseY = eyeY + eyeHeight;

  // desenhar a curva (pálpebra inferior sorridente)
  if (radius > 0) {
    display.fillEllipse(
      eyeX + eyeWidth / 2,  // centro X
      ellipseY,             // centro Y da elipse
      eyeWidth / 2,         // raio horizontal
      radius,               // raio vertical controlado pelo happiness
      BLACK
    );
  }
}

void animateHappiness(int target, int speed) {
    // target entre 0 e 100
    if (target < 0) target = 0;
    if (target > 100) target = 100;

    int start = happiness;

    // Quantos passos vamos usar (velocidade define isso)
    // speed = 1  → lento
    // speed = 10 → rápido
    float step = 0.01f * speed;   // 0.01 → 1% por ciclo, multiplicado pela velocidade

    float t = 0.0f;

    while (t < 1.0f) {
        t += step;
        if (t > 1.0f) t = 1.0f;

        // LERP → valor interpolado
        int value = start + (target - start) * t;

        happiness = value;

        // Delay pequeno para suavidade (frame time)
        vTaskDelay(pdMS_TO_TICKS(16));  // ~60 FPS
    }
}

void drawEyeTFT(int eyeX, int eyeY, int eyeWidth, int eyeHeight, TFT_eSprite &spr){
  spr.fillRoundRect(eyeX, eyeY, eyeWidth, eyeHeight, 10, TFT_BLUE);
  // --- Happy curve ---
  float h = happiness / 100.0;         // 0.0 a 1.0
  int radius = h * (eyeHeight / 2);    // 0 → meio do olho | 100 → max

  // A curva deve começar do fundo do olho
  int ellipseY = eyeY + eyeHeight;

  // desenhar a curva (pálpebra inferior sorridente)
  if (radius > 0) {
    spr.fillEllipse(
      eyeX + eyeWidth / 2,  // centro X
      ellipseY,             // centro Y da elipse
      eyeWidth / 2,         // raio horizontal
      radius,               // raio vertical controlado pelo happiness
      TFT_TRANSPARENT
    );
  }
}

/*TFT*/
void TaskEyeDrawTFT(void *pvParameters){
  tft.init();
  //sprite.createSprite(240, 240);
  TFT_eSprite eyes = TFT_eSprite(&tft);  
  TFT_eSprite spritesUp = TFT_eSprite(&tft);
  TFT_eSprite spritesDown = TFT_eSprite(&tft);
  float scale = 2;
  uint8_t widthL = eyeWidthL*scale, widthR = eyeWidthR*scale;
  uint8_t heightL = eyeHeightL*scale, heightR = eyeHeightR*scale;
  eyes.createSprite(widthL+80+widthR, heightL+80);
  spritesUp.createSprite(240, 120);
  spritesDown.createSprite(240, 120);
  //spritesUp.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  //eyes.setSwapBytes(true);
  // Set "cursor" at top left corner of display (0,0) and select font 4
  //tft.setCursor(0, 4, 4);

  //tft.invertDisplay( false ); // Where i is true or false
  for(;;){
    if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY)){
      uint8_t x = 50-((scale-1)*widthL/4)-(scale - 1)*2*scale;
      uint8_t y = 50;
      spritesUp.fillSprite(TFT_CYAN);
      spritesDown.fillSprite(TFT_CYAN);
      eyes.fillSprite(TFT_TRANSPARENT);
      if(blinkState == 0){
        drawEyeTFT(offsetX + leftEyeX, offsetY + eyeYoffL + 20, widthL, heightL, eyes);
        drawEyeTFT(offsetX + spritesUpX + (((scale - 1)*widthR)/scale + (scale - 1)*scale), offsetY + eyeYoffR + 20, widthR, heightR, eyes);
      }
      else{
        eyes.fillRect(offsetX + leftEyeX, eyeYoffL + eyeY + offsetY + heightL/2 - 2, widthL, 4, TFT_BLUE);
        eyes.fillRect(offsetX + spritesUpX + (((scale - 1)*widthR)/scale + (scale - 1)*scale), eyeYoffR + eyeY + offsetY + heightR/2 - 2, widthR, 4, TFT_BLUE);
      }
    
      eyes.pushToSprite(&spritesUp,x,y,TFT_TRANSPARENT);
      eyes.pushToSprite(&spritesDown,x,y-120,TFT_TRANSPARENT);
      spritesUp.pushSprite(0,0);
      spritesDown.pushSprite(0,120);
      xSemaphoreGive(xDisplayMutex);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}