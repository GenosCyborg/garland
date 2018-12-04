#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <avr/eeprom.h>

  
#define RED 0xFF0000
#define ORANGE 0xFF7F00
#define YELLOW 0xFFFF00
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define INDIGO 0x4B0082
#define VIOLET 0x9400D3


uint32_t mul(uint32_t c, float k){
    uint16_t low = ((uint16_t)((c >> 8 & 0xFF) * k)) << 8 | ((uint16_t)((c & 0xFF) * k));
    uint16_t high = ((uint16_t)((c >> 24 & 0xFF) * k)) << 8 | ((uint16_t)((c >> 16 & 0xFF) * k));
  return ( (uint32_t)low | (uint32_t)high << 16 );
}

uint32_t add(uint32_t c, uint8_t k){
    uint16_t lowlow = ((c & 0xFF) + k);
    uint16_t lowhigh = (((c >> 8 & 0xFF) + k));
    if (lowlow > 65279) lowlow = 0;
    else if (lowlow > 255) lowlow = 255;
    if (lowhigh > 65279) lowhigh = 0;
    else if (lowhigh > 255) lowhigh = 255;
    uint16_t low = lowhigh << 8 | lowlow;
    
    
    lowlow = ((c >> 16 & 0xFF) + k);
    lowhigh = ((c >> 24 & 0xFF) + k);
    if (lowlow > 65279) lowlow = 0;
    else if (lowlow > 255) lowlow = 255;
    if (lowhigh > 65279) lowhigh = 0;
    else if (lowhigh > 255) lowhigh = 255;
    uint16_t high = lowhigh << 8 | lowlow;
    
    
    return ( (uint32_t)low | (uint32_t)high << 16 );
}

uint32_t sub(uint32_t c, uint8_t k){
    uint16_t lowlow = ((c & 0xFF) - k);
    uint16_t lowhigh = (((c >> 8 & 0xFF) - k));
    if (lowlow > 65279) lowlow = 0;
    else if (lowlow > 255) lowlow = 255;
    if (lowhigh > 65279) lowhigh = 0;
    else if (lowhigh > 255) lowhigh = 255;
    uint16_t low = lowhigh << 8 | lowlow;
    
    
    lowlow = ((c >> 16 & 0xFF) - k);
    lowhigh = ((c >> 24 & 0xFF) - k);
    if (lowlow > 65279) lowlow = 0;
    else if (lowlow > 255) lowlow = 255;
    if (lowhigh > 65279) lowhigh = 0;
    else if (lowhigh > 255) lowhigh = 255;
    uint16_t high = lowhigh << 8 | lowlow;
    
    
    return ( (uint32_t)low | (uint32_t)high << 16 );
}

union Decompozition
{
  float f;
  unsigned char b[4];
};

union Decompozition speed;
#define countL 30
uint32_t time;
uint8_t brightness;
bool useMicro = false;
uint8_t mode;



void writeBrightness(){
  eeprom_write_byte((uint8_t*)0,brightness);
}

void readBrightness(){
  brightness = eeprom_read_byte((uint8_t*)0);
}

void writeMode(){
  eeprom_write_byte((uint8_t*)5,mode);
}

void readMode(){
  mode = eeprom_read_byte((uint8_t*)5);
}

void writeSpeed(){
  eeprom_write_byte((uint8_t*)1,speed.b[0]);
  eeprom_write_byte((uint8_t*)2,speed.b[1]);
  eeprom_write_byte((uint8_t*)3,speed.b[2]);
  eeprom_write_byte((uint8_t*)4,speed.b[3]);
}

void readSpeed(){
  speed.b[0] = eeprom_read_byte((uint8_t*)1);
  speed.b[1] = eeprom_read_byte((uint8_t*)2);
  speed.b[2] = eeprom_read_byte((uint8_t*)3);
  speed.b[3] = eeprom_read_byte((uint8_t*)4);

}




class Button{
  public:
  Button(){
    state = 0;
    mode = 0;
    showState = 0;
  }

  uint8_t state;
  uint32_t timer;
  uint8_t mode;
  uint8_t showState;
  //0 - отпущена без нажатия, 1  нажата,
  //2 удерживатеся, 3 отпущена

  void update(uint8_t s){
    showState = 0;
    if ( s && (state == 3 || state == 0) ){
      state = 1;
    }
    else if (s && state == 1){
      state = 2;
    }
    else if (!s && (state == 1 || state == 2)){
      state = 3;
    }
    else if (!s && (state == 3)){
      state = 0;
    }

    if (state == 1){
      timer = time + 300;
      if (mode == 0) mode = 1;
      if (mode == 2) mode = 3;
    }

    if (state == 3){
      if (mode == 1 && timer >= time)mode = 2;
    }

    if (mode != 0 && timer < time){
      showState = mode;
      if (mode != 1) mode = 0;
      if (state == 3) mode = 0;
    }
  }
};







Adafruit_NeoPixel pixels = Adafruit_NeoPixel(countL, 11, NEO_GRB + NEO_KHZ800);
Button b1;
Button b2;


void setup() {
  readBrightness();
  readSpeed();
  mode = 0;

  
  pinMode(A3, INPUT);
  randomSeed(analogRead(A0));

  DDRB &= ~(1 << 1);
  DDRB &= ~(1 << 2);
  DDRB |= (1 << 3);
  PORTB &= ~(1 << 3);
  
  PORTB |= (1 << 1) | (1 << 2);

  b1 = Button();
  b2 = Button();

  Serial.begin(2000000);
  
  pixels.begin();
  pixels.setBrightness(brightness/2.55f);
}



void checkButtons(){
  b1.update(!((PINB >> 1) & 0x01));
  b2.update(!((PINB >> 2) & 0x01));

}

uint16_t step = 0;
uint32_t STRIP[countL+20];
uint32_t BSTRIP[countL+20];



void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, c);
    pixels.show();
    delay(wait);
//    S
  }
}

void rainbow() {
  uint16_t i;

    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i+step) & 255));
    }
    pixels.show();
    step++;
    if (step > 255) step = 0;
}






void rainbowCycle() {
  uint16_t i;

  
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + step) & 255));
    }
    pixels.show();
    step++;
    if (step > 1275) step = 0;
    pixels.show();
}







uint8_t dirrection;
uint32_t meteor_color;
void meteor(){
  uint8_t dt = step/3;
  if (step == 0){
    dirrection = random(2);
    meteor_color = random(7);
    if (meteor_color == 0) meteor_color = RED;
    else if (meteor_color == 1) meteor_color = ORANGE;
    else if (meteor_color == 2) meteor_color = YELLOW;
    else if (meteor_color == 3) meteor_color = GREEN;
    else if (meteor_color == 4) meteor_color = BLUE;
    else if (meteor_color == 5) meteor_color = INDIGO;
    else if (meteor_color == 6) meteor_color = VIOLET;
  }
  for(uint8_t i(0); i<countL+20; i++) {
    if (dirrection == 1){
      if (i >= dt && i < dt + 10){
        STRIP[i] = mul(meteor_color, (i-dt)/10.0f);
      }
      else{
        STRIP[i] = 0;
      }
    }
    else{
      if (i >= dt && i < dt + 10){
        STRIP[countL - i +19] = mul(meteor_color, (i-dt)/10.0f);
      }
      else{
        STRIP[countL - i + 19] = 0;
      }
    }
    pixels.setPixelColor(i, STRIP[i+10]);
  }
  pixels.show();
  step++;
  if (step == 150) step = 0;
}






void rain(){
  uint8_t pixel = random(countL);
  if (step == 5){
    uint8_t is = random(2);
    if (is == 0) STRIP[pixel] = 0xFFFFFFFF;
    step = 0;
  }
  else step++;
  for(uint8_t i (0); i< pixels.numPixels(); i++) {
    if(STRIP[i] > 0) STRIP[i] -= 0x1010101;
    pixels.setPixelColor(i, STRIP[i]);
  }
    pixels.show();
}











float Position=0;
void RunningLights(byte red, byte green, byte blue) {
  
    Position+=0.1f; // = 0; //Position + Rate;
    for(int i=0; i<countL; i++) {
      pixels.setPixelColor(i,((sin(i+Position) * 127 + 128)/255)*red,
                 ((sin(i+Position) * 127 + 128)/255)*green,
                 ((sin(i+Position) * 127 + 128)/255)*blue);
    }
    
    pixels.show();
    step++;
    if (step > 510) step = 0;
    pixels.show();
}

void newYear(){
  uint8_t dt = step/50;
  bool direction = dt%2==0;
  for(uint8_t i(0); i<countL; i++) {
    if ((i%2==0 && direction) || (i%2!=0 && !direction)) pixels.setPixelColor(i, RED);
    else pixels.setPixelColor(i, GREEN);
  }
  pixels.show();
  step++;
  if (step > 2000) step = 0;
}









uint8_t fadeRain_direction[countL];
void fadeRain(){
  uint8_t pixel = random(countL);
  if (step == 5){
    uint8_t is = random(2);
    if (is == 0) fadeRain_direction[pixel] = 0;
    step = 0;
  }
  else step++;
  for(uint8_t i (0); i< countL; i++) {
    if (fadeRain_direction[i] == 0){
      if ((STRIP[i] >> 24) > 0) STRIP[i] = sub(STRIP[i], 1);
      else if ((STRIP[i] >> 24) == 0) fadeRain_direction[i] = 1;
    }
    else{
      if ((STRIP[i] >> 24) < 255)  STRIP[i] = add(STRIP[i], 1);
    }
    pixels.setPixelColor(i, STRIP[i]);
  }
  pixels.show();
}












void fadeSinRain(){
  uint8_t pixel = random(countL)+10;
    if (step == 5){
      uint8_t is = random(10);
      if (is == 0) {
        STRIP[pixel] = 0xFFFFFFFF;
        STRIP[pixel-1] = 0x99999999;
        STRIP[pixel-2] = 0x44444444;
        STRIP[pixel+1] = 0x99999999;
        STRIP[pixel-2] = 0x44444444;
      }
      step = 0;
    }
    else step++;
    for(uint8_t i (10); i< pixels.numPixels()+10; i++) {
      if(STRIP[i] > 0) {
        STRIP[i] = mul(STRIP[i], 0.99f);
      }
      pixels.setPixelColor(i-10, STRIP[i]);
    }
    pixels.show();

}


uint32_t swarm_color;
void swarm(){
  uint8_t dt = step/50;

  for(uint8_t i(0); i<countL; i++) {
    if (i == dt){
      STRIP[i] = 0xFFFFFFFF;
    }
    if (i > dt && i - 10 < dt) {
      STRIP[i] -= 0x06060606;
    }
    else{
      STRIP[i]=0;
    }
    pixels.setPixelColor(i, STRIP[i]);
  }

  
  pixels.show();
  step++;
  if (step > 2000) step = 0;
}




void girlyand(){

  if (step == 0){
    for(uint8_t i (0); i< pixels.numPixels(); i++){
      uint8_t c = random(4);
      if ( c == 0 ) pixels.setPixelColor(i, RED);
      else if ( c == 1 ) pixels.setPixelColor(i, GREEN);
      else if ( c == 2 ) pixels.setPixelColor(i, BLUE);
      else if ( c == 3 ) pixels.setPixelColor(i, YELLOW);
    
    }
    pixels.show();
  }

  step++;
  if (step == 50) step = 0;
}









void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, c);   
      }
      pixels.show();

      delay(wait);

      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);   
      }
    }
  }
}


void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, Wheel( (i+j) % 255));  
      }
      pixels.show();

      delay(wait);

      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);     
      }
    }
  }
}



uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


uint32_t mask(uint32_t c, uint32_t mask){
  uint16_t low = ((uint16_t)((c >> 8 & 0xFF) * (mask >> 8 & 0xFF)/0xFF)) << 8 | ((uint16_t)((c & 0xFF) * (mask & 0xFF)/0xFF));
  uint16_t high = ((uint16_t)((c >> 24 & 0xFF) * (mask >> 24 & 0xFF)/0xFF)) << 8 | ((uint16_t)((c >> 16 & 0xFF) * (mask >> 16 & 0xFF)/0xFF));
  return ( (uint32_t)low | (uint32_t)high << 16 );
}


uint32_t breath_color;
void breath(){
  if (step == 0){
    dirrection = 1;
    breath_color = random(7);
    if (breath_color == 0) breath_color = RED;
    else if (breath_color == 1) breath_color = ORANGE;
    else if (breath_color == 2) breath_color = YELLOW;
    else if (breath_color == 3) breath_color = GREEN;
    else if (breath_color == 4) breath_color = BLUE;
    else if (breath_color == 5) breath_color = INDIGO;
    else if (breath_color == 6) breath_color = VIOLET;
  }
  
  for(uint8_t i(0); i<countL; i++) {
    if (dirrection){
      BSTRIP[i] = add(BSTRIP[i], 1);
    }
    else{
      BSTRIP[i] = sub(BSTRIP[i], 1);
    }
    STRIP[i] = mask(BSTRIP[i], breath_color);
    pixels.setPixelColor(i, STRIP[i]);
  }
  if (step == 255) dirrection = 0;
  
  pixels.show();
  step++;
  if (step == 511) step = 0;
}


  

  void addMode(){
    mode++;
    if (mode == 12) mode = 1;
    step = 0;
    fill(0x0);
    Serial.println(String("Mode: ") + String(mode));
  }
  void removeMode(){
    mode--;
    if (mode == 0 || mode == 255) mode = 11;
    step = 0;
    fill(0x0);
    Serial.println(String("Mode: ") + String(mode));
  }
  void zeroMode(){
    mode = 0;
    step = 0;
    fill(0x0);
    Serial.println(String("Mode: ") + String(mode));
  }
  
uint8_t allModesMode = 1;
  void addRMode(){
    allModesMode++;
    if (allModesMode == 12) allModesMode = 1;
    step = 0;
    fill(0x0);
    Serial.println(String("AutoMode: ") + String(allModesMode));
  }
  void removeRMode(){
    allModesMode--;
    if (allModesMode == 0) allModesMode = 11;
    step = 0;
    fill(0x0);
    Serial.println(String("AutoMode: ") + String(allModesMode));
  }

  void fill(uint32_t t){
    for(uint8_t i (0); i< countL+20; i++){
      STRIP[i] = t;
      BSTRIP[i] = t;
    }
  }

uint32_t lastChange = 0;
void allModesCircle(){
  if (millis() > lastChange){
    lastChange = millis() + 30000;
    addRMode();
  }
  drawMode(allModesMode);
}


void drawMode(uint8_t mode){
  switch(mode){
    case 0:
      allModesCircle();
    break;
    case 1:
      rainbow();
    break;
    case 2:
      rainbowCycle();
    break;
    case 3:
      meteor();
    break;
    case 4:
      rain();
    break;
    case 5:
      RunningLights(0xFF, 0xFF, 0xFF);
    break;
    case 6:
      newYear();
    break;
    case 7:
      fadeRain();
    break;
    case 8:
      fadeSinRain();
    break;
    case 9:
      girlyand();
    break;
    case 10:
      swarm();
    break;
    case 11:
      breath();
    break;
  }

}







uint8_t onMic = 0;
void loop() {
  time = millis();
  checkButtons();
  if(b1.showState == 1 && b2.showState == 3){
    if (brightness >= 0){
      brightness-=15;
      pixels.setBrightness(brightness/2.55f);
      writeBrightness();
      Serial.println(String("Brightness: ") + String(brightness));
    }
  }
  else if(b1.showState == 1 && b2.showState == 2){
    if (brightness <= 255){
      brightness+=15;
      pixels.setBrightness(brightness/2.55f);
      writeBrightness();
      Serial.println(String("Brightness: ") + String(brightness));
    }
  }
  else if(b2.showState == 1 && b1.showState == 3){
    speed.f *= 0.95f;
    if (speed.f < 10.0f) speed.f = 10;
      writeSpeed();
      Serial.println(String("Speed: ") + String(speed.f));
  }
  else if(b2.showState == 1 && b1.showState == 2){
    speed.f *= 1.05263f;
      writeSpeed();
      Serial.println(String("Speed: ") + String(speed.f));
  }
  else if(b1.showState == 1 && b2.showState == 1){
    onMic++;
    if (onMic == 100){
      uint8_t on = true;
      for (uint8_t j(0); j < 10; j++){
        for(uint8_t i(0); i<countL; i++){
            if (on) pixels.setPixelColor(i, 0xFFFFFFFF);
            else pixels.setPixelColor(i, 0x0);
        }
        on = !on;
        delay(100);
        pixels.show();
      }
      onMic = 0;
      if (mode != 0 )zeroMode();
    }
  }
  else if(b1.showState == 2){
    addMode();
  }
  else if(b2.showState == 2){
    removeMode();
  }
  drawMode(mode);
  delay(1000.0f/speed.f);

}
