#include <ModbusMaster.h>
#include <Zigbee.h>

// กำหนดขา UART สำหรับเชื่อมต่อโมดูล TTL485-V2.0 บน ESP32-C5
#define RXD2 12  
#define TXD2 11  

ModbusMaster node;

// ใช้ ZigbeeTempSensor ตัวหลัก (Endpoint ID = 1)
ZigbeeTempSensor zbTempSensor(1);

// กำหนดเวลาหน่วงระหว่างการส่งข้อมูลเป็น 3 นาที (3 นาที * 60 วินาที * 1000 มิลลิวินาที)
#define REPORT_INTERVAL_MS (3UL * 60UL * 1000UL)

void setup() {
  Serial.begin(115200);
  
  // เริ่มต้น Serial1 สำหรับอ่านเซนเซอร์ RS485 (Baud Rate 4800)
  Serial1.begin(4800, SERIAL_8N1, RXD2, TXD2);
  node.begin(1, Serial1);

  // ตั้งค่าช่วงอุณหภูมิและความชื้น
  zbTempSensor.setMinMaxValue(-40, 80); 
  zbTempSensor.addHumiditySensor(0, 100, 1); 

  // เพิ่ม Endpoint เข้าไปในระบบ Zigbee
  Zigbee.addEndpoint(&zbTempSensor);

  // เริ่มต้นการทำงานของ Zigbee
  if (!Zigbee.begin()) {
    Serial.println("Zigbee initialization failed! Restarting...");
    ESP.restart();
  }

  Serial.println("Waiting to join Zigbee network...");
  
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to Zigbee network successfully!");
}

void loop() {
  uint8_t result;
  
  // อ่านค่า Register จากเซนเซอร์
  result = node.readHoldingRegisters(0x0000, 2);

  if (result == node.ku8MBSuccess) {
    float humidity = node.getResponseBuffer(0) / 10.0;    
    float temperature = node.getResponseBuffer(1) / 10.0; 

    Serial.print("Soil Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    
    Serial.print("Soil Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.println("-----------------------------------");

    // อัปเดตค่าเข้าสู่ Zigbee Endpoint ตัวเดียวกัน
    zbTempSensor.setTemperature(temperature);
    zbTempSensor.setHumidity(humidity);
    zbTempSensor.report(); // ส่งข้อมูลไปยังเกตเวย์

  } else {
    Serial.print("Failed to read from RS485 sensor! Error code: ");
    Serial.println(result, HEX);
  }

  // หน่วงเวลา 3 นาทีก่อนจะวนกลับมาวัดค่ารอบถัดไป
  delay(REPORT_INTERVAL_MS); 
}