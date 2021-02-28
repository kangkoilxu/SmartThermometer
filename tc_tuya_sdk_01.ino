#include "mcu_api.h"
#include "protocol.h"
#include "tuya_wifi.h"
#include "HardwareSerial.h"
#include "DHT.h"

#define DEBUG //调试用 宏定义
#define CLOUD //tuya调试宏定义

#define TEMP_UPDATE_INTERVAL  180//数据上报间隔，单位s

#define DHTPIN 2        // DHT11 引脚
DHT dht(DHTPIN, DHT11);

#define PRPIN 32        //光敏电阻引脚
#define PIRPIN 4

float lastTemperature = 0;    //温度
float lastHumidity    = 0;    //湿度
uint32_t lastPr       = 0;    //光敏
uint8_t FirstUpdate   = TRUE;
uint32_t last_millis  = 0;
uint8_t pir_value = 0;

void setup() {

  Serial.begin(115200);
  Serial2.begin(115200);
  dht.begin();
  #ifdef CLOUD
    wifi_protocol_init();
  #endif 
  pinMode(PIRPIN,INPUT);
  Serial.println("Tuya: Initialze Done!");
  FirstUpdate = TRUE; //第一次循环时，立即发送一次数据
}

void loop() {

  uint8_t ret = FALSE;

  #ifdef CLOUD
    wifi_uart_service();

  if (Serial2.available()) {
    unsigned char ch = (unsigned char)Serial2.read();
    uart_receive_input(ch);
  }

  if(FirstUpdate || (millis() - last_millis > 1000 * TEMP_UPDATE_INTERVAL )){
    FirstUpdate = false;
    last_millis = millis();
    ret = reportTemp();
    //Serial.println("Tuya: Update Temperature and Humidity!");
    //Serial.printf("Tuya: Update Result is %s!\n", ret ? "Success" : "Faield");
  }
  #else 
    float thisTemperature = dht.readTemperature();      
    float thisHumidity    = dht.readHumidity();
    if ( isnan(thisTemperature) || isnan(thisHumidity) ) {
      return ;
    }
    Serial.printf("( %.1f℃ , %.1f\%% )\n",thisTemperature, thisHumidity);
    delay(1000);
  #endif 
  // pir_value = digitalRead(PIRPIN);
  // Serial.printf("PIR: %d\n",pir_value);
  // delay(1000);

}

//温湿度上报
uint8_t reportTemp(void)
{

  unsigned char ret;
  
  float thisTemperature = dht.readTemperature();      
  float thisHumidity    = dht.readHumidity();

  if ( isnan(thisTemperature) || isnan(thisHumidity) ) {
    return ERROR;
  }

  #ifdef DEBUG
    Serial.printf("( %.1f℃ , %.1f\%% )\n",thisTemperature, thisHumidity);
  #endif            

 if (abs(thisTemperature - lastTemperature) > 0.1){  
    lastTemperature = thisTemperature; 
    mcu_dp_value_update(DPID_TEMP_CURRENT,(lastTemperature));   //temp上报，原temp_current数据点定义的有问题
    ret = mcu_dp_value_update(DPID_CH4_VALUE,(lastTemperature*10));   //0 - 1000, ratio 1 
  }
  
  if (thisHumidity != lastHumidity){            
    lastHumidity = thisHumidity;    
    ret = mcu_dp_value_update(DPID_HUMIDITY_VALUE,long(lastHumidity)); 

  }

  uint8_t prvalue = analogRead(PRPIN);
  float pr_value_t = prvalue * 1.0975 ; 
  Serial.printf("Debug: Pr Value t is %f\n",pr_value_t);
  ret = mcu_dp_value_update(DPID_BRIGHT_VALUE,long(pr_value_t)); 

  return SUCCESS;

}
