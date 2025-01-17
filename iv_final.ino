#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>

// Pin configuration
const int sensorPin = 22;  // IR sensor output pin for ESP32 (change as needed)
volatile int dropCount = 0;  // Store total number of drops detected
unsigned long lastDropTime = 0;  // Track time of last detected drop
// Constants for flow rate calculation
float dripRate = 0.0;  // Drip rate in drops per minute
const float dropFactor = 20.0;  // Example: IV set with 20 drops per mL
const int calculationInterval = 3000;  // Calculate flow rate every 6 seconds (6000 ms)
// Variables for flow rate calculation
unsigned long lastCalculationTime = 0;
float flowRate = 0.0;  // Flow rate in mL/hr
int charge=98;
const float R1 = 36500.0; // Resistor R1 in ohms
const float R2 = 10000.0; // Resistor R2 in ohms
const float Vref = 0.95;   // ESP32 reference voltage (3.3V)
const float totalvotage = 3.3;
// Use an appropriate analog pin for ESP32
int chargePin = 34; // Example: GPIO34 for analog input


U8G2_ST7567_ENH_DG128064_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* cs=*/ 5, /* dc=*/ 4, /* reset=*/ 2); 


const unsigned char BELL_BITMAP[] PROGMEM = {
  0xF0, 0x01,  
  0xF8, 0x03, 
  0xFC, 0x07,    
  0xFC, 0x07, 
  0xFC, 0x07,    
  0xFC, 0x07,   
  0xFC, 0x07,   
  0xFF, 0x1F,
  0xFF, 0x1F,  
  0xFF, 0x1F,   
  0x00, 0x00,  
  0xF0, 0x01,   
  0xF8, 0x03,   
  0xF8, 0x03,  
  0xF0, 0x01,   
};

void IRAM_ATTR onDrop() {
  unsigned long currentTime = millis();  // Get current time
  // Ensure at least 100 ms between drops to prevent double-counting
  if (currentTime - lastDropTime >= 100) {
    dropCount++;  // Increment drop count
    Serial.print("Drop count: ");
    Serial.println(dropCount);
    lastDropTime = currentTime;  // Update last drop time
  }
}

void setup(void) {
  Serial.begin(115200);  // Start Serial Monitor
  delay(1000);  // Allow time for Serial Monitor to initialize
  pinMode(sensorPin, INPUT);  // Set sensor pin as input
  pinMode(chargePin, INPUT);
  attachInterrupt(sensorPin, onDrop, FALLING);  // Attach interrupt for ESP32
  Serial.println("IV Flow Rate Monitoring Initialized...");
  u8g2.begin();
}

void loop(void) {
  unsigned long currentTime = millis();
  int rawValue = readStableAnalog(); // Get stable ADC reading
  float voltageOut = (rawValue * Vref) / 4095.0; // Calculate voltage at the analog pin (ESP32 ADC range is 0-4095)
  float batteryVoltage =( voltageOut * ((R1 + R2) / R2)); // Calculate battery voltage
  if(batteryVoltage<=4.2&&batteryVoltage>4.0){charge=100;}
  else if(batteryVoltage<=4.0&&batteryVoltage>3.9){charge=75;}
  else if(batteryVoltage<=3.8&&batteryVoltage>3.6){charge=50;}
  else if(batteryVoltage<=3.6&&batteryVoltage>3.4){charge=25;}
  else{charge=0;}
  
  Serial.print("Battery Voltage: ");
  Serial.print(batteryVoltage);
  Serial.println(" V");

  // Calculate flow rate every calculationInterval (6 seconds)
  if (currentTime - lastCalculationTime >= calculationInterval) {
    noInterrupts();  // Disable interrupts for safe access to dropCount
    int drops = dropCount;  // Get current drop count
    Serial.print("Drops: ");
    Serial.println(drops);
    dropCount = 0;  // Reset drop count for next interval
    interrupts();  // Re-enable interrupts

    // Calculate drip rate in drops per minute
    dripRate = (drops / 6.0) * 60.0;  // Scale up to drops per minute
    Serial.print("Drip Rate: ");
    Serial.println(dripRate);

    // Calculate flow rate in mL/hr
    flowRate = (dripRate / dropFactor) * 60.0;
    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.println(" mL/hr");

    // Update last calculation time
    lastCalculationTime = currentTime;
  }
  u8g2.firstPage();
  do {
     u8g2.drawFrame(5, 0, 122, 64);
     u8g2.drawLine(0, 20, 128,20); 
     u8g2.drawLine(45, 20, 45,64);
     drawBatteryIcon(charge,u8g2);
     drawBluetoothSymbol(u8g2);
     drawWiFiSemiCircles(u8g2);
     drawDropIcon(u8g2);
     drawText(flowRate,charge,u8g2);
     drawNumberBox(0,23,u8g2);
     u8g2.drawXBMP(70, 3, 16, 15, BELL_BITMAP);
  } while (u8g2.nextPage());
  delay(1000);
}

void drawBluetoothSymbol(U8G2 &u8g2) {

    // Center of the display (128x64), position the symbol around the center
    int centerX = 114; // X-coordinate of the center
    int centerY = 10; // Y-coordinate of the center
    int size = 7;     // Half of the 15x15 size for scaling


    u8g2.drawBox(centerX - 1, centerY - size, 1, 2 * size);
    // Top arrow (rightward diagonal)
    u8g2.drawLine(centerX, centerY - size, centerX + size, centerY - size / 2); // Diagonal line
 

    // Bottom arrow (rightward diagonal)
    u8g2.drawLine(centerX, centerY + size, centerX + size, centerY + size / 2); // Diagonal line

    // Cross lines (X shape)
    u8g2.drawLine(centerX + size, centerY - size / 2, centerX - size, centerY + size / 2); // Top-left diagonal
    u8g2.drawLine(centerX + size, centerY + size / 2, centerX - size, centerY - size / 2); // Bottom-left diagonal

}

void drawBatteryIcon(int charge,U8G2 &u8g2) {
  // Battery dimensions and position
  int x =10;           // X-coordinate of the top-left corner
  int y = 4;           // Y-coordinate of the top-left corner
  int width = 23;       // Width of the battery body
  int height = 13;      // Height of the battery body
  int capWidth = 3;     // Width of the battery cap

  // Calculate the width of the charge level
  int chargeWidth = (charge * (width - 4)) / 100; // Subtract 4 for padding


  // Draw the battery body (outer rectangle)
  u8g2.drawFrame(x, y, width, height);

  // Draw the battery cap
  u8g2.drawBox(x + width, y + height / 4, capWidth, height / 2);

  // Draw the charge level
  if (charge > 0) {
    u8g2.drawBox(x + 2, y + 2, chargeWidth, height - 4);
  }
}

void drawWiFiSemiCircles(U8G2 &u8g2) {
    // Scaled-down semi-circles for the WiFi symbol
    drawSemiCircle(96, 14, 9,u8g2); // Outer semi-circle
    drawSemiCircle(96, 14, 6,u8g2); // Middle semi-circle
    drawSemiCircle(96, 14, 3,u8g2); // Inner semi-circle
    // Scaled-down small circle at the center
    u8g2.drawDisc(96, 14, 1); // Center disc
}

void drawSemiCircle(int cx, int cy, int r,U8G2 &u8g2) {
  // Draw semi-circle using line approximation
  for (int angle = 30; angle <= 150; angle += 10) { // Only 0 to 180 degrees
    float radians = angle * 3.14159 / 180.0;       // Convert degrees to radians
    int x1 = cx + r * cos(radians);
    int y1 = cy - r * sin(radians);               // Subtract to flip Y-axis for display
    int x2 = cx + r * cos(radians + 10 * 3.14159 / 180.0);
    int y2 = cy - r * sin(radians + 10 * 3.14159 / 180.0);
    u8g2.drawLine(x1, y1, x2, y2);                 // Draw the segment
    u8g2.setColorIndex(1);
  }
}

void drawDropIcon(U8G2 &u8g2) {
    // Drop dimensions and position
    int centerX = 60; // X-coordinate of the center of the drop
    int centerY = 42; // Y-coordinate of the center of the drop
    int width = 20;   // Width of the drop
    int height = 30;  // Height of the drop

    // Draw the pointed tip of the drop (triangle-like)
    u8g2.drawTriangle(centerX, centerY - height / 2, centerX - width / 2, centerY, centerX + width / 2, centerY);

    // Draw the rounded bottom part of the drop (circle or arc)
    u8g2.drawDisc(centerX, centerY + height / 6, width / 2, U8G2_DRAW_ALL);
}

void drawText(int flowrate,int charge, U8G2 &u8g2) {
    // Set font for flowrate
    u8g2.setFont(u8g2_font_ncenB14_tr);
    
    // Convert flowrate to string to measure its width
    char flowrateStr[8];
    sprintf(flowrateStr, "%d", flowrate);
    
    // Get the width of the flowrate text
    int textWidth = u8g2.getStrWidth(flowrateStr);
    
    // Center at x=80 by subtracting half the text width
    int xPos = 98 - (textWidth / 2);
    
    // Draw centered flowrate
    u8g2.setCursor(xPos, 41);
    u8g2.print(flowrate);
    
    // Draw "ml/hr" label
    u8g2.setFont(u8g2_font_ncenB10_tr);
    textWidth = u8g2.getStrWidth("ml/hr");
    xPos = 98 - (textWidth / 2);
    u8g2.setCursor(xPos, 57);
    u8g2.print("ml/hr");

    u8g2.setCursor(40,16);
    u8g2.print(charge);
}

void drawNumberBox(int topLeftX,int topLeftY,U8G2 &u8g2) {
    // Set font for numbers
    u8g2.setFont(u8g_font_6x10);
    
    // Calculate center position based on top-left coordinates
    int boxCenterX = topLeftX + 20;  // Center is 20 pixels right of top-left X
    
    // Draw numbers with vertical spacing of 12 pixels
    // Draw "10"
    int textWidth = u8g2.getStrWidth("10");
    u8g2.drawStr(boxCenterX - (textWidth/2), topLeftY + 10, "10");
    if (dropFactor == 10.0) {
      u8g2.drawFrame(boxCenterX - (textWidth/2) - 2, (topLeftY + 10) - 9, textWidth + 4, 11);  
    }
    // Draw "15"
    textWidth = u8g2.getStrWidth("15");
    u8g2.drawStr(boxCenterX - (textWidth/2), topLeftY + 22, "15");
    if (dropFactor == 15.0) {
      u8g2.drawFrame(boxCenterX - (textWidth/2) - 2, (topLeftY + 22) - 9, textWidth + 4, 11);
    } 
    // Draw "20"
    textWidth = u8g2.getStrWidth("20");
    u8g2.drawStr(boxCenterX - (textWidth/2), topLeftY + 34, "20");
    if (dropFactor == 20.0) {
      u8g2.drawFrame(boxCenterX - (textWidth/2) - 2, (topLeftY + 34) - 9, textWidth + 4, 11);
    }
}

int readStableAnalog() {
  const int numSamples = 20; // Number of samples for averaging
  long total = 0;            // Accumulator for ADC values
  for (int i = 0; i < numSamples; i++) {
    total += analogRead(chargePin); // Read raw ADC value
    delay(5);                     // Short delay between samples
  }
  return total / numSamples; // Return the average value
}