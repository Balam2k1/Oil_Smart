#include "display_manager.h"
#include "data_model.h"
#include "max7219.h"
#include "alarm_manager.h"
#include "sensor_manager.h"
#include "app_config.h"

void DisplayManager_Init(void)
{
    // Bật tất cả các LED rời lên trước (logic 1 = sáng)
    HAL_GPIO_WritePin(LED_TEMP_C_GPIO_Port, LED_TEMP_C_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PA_C_GPIO_Port, LED_PA_C_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_OLDC_GPIO_Port, LED_OLDC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_TACHO_GPIO_Port, LED_TACHO_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_VIN_GPIO_Port, LED_VIN_Pin, GPIO_PIN_SET);
}

void DisplayManager_StartupBlink(void)
{
    bool blink_temp = (Sensor_GetOperatingHours(0) >= ((uint32_t)gConfigData.SensorMaintLimits[0] * 60));
    bool blink_rpm = (Sensor_GetOperatingHours(1) >= ((uint32_t)gConfigData.SensorMaintLimits[1] * 60));
    bool blink_pressure = (Sensor_GetOperatingHours(2) >= ((uint32_t)gConfigData.SensorMaintLimits[2] * 60));
    bool blink_oil1 = (Sensor_GetOperatingHours(3) >= ((uint32_t)gConfigData.SensorMaintLimits[3] * 60));
    bool blink_oil2 = (Sensor_GetOperatingHours(4) >= ((uint32_t)gConfigData.SensorMaintLimits[4] * 60));

    if (!blink_temp && !blink_rpm && !blink_pressure && !blink_oil1 && !blink_oil2) {
        return; // Không có cảm biến nào quá hạn bảo dưỡng
    }

    for (int i = 0; i < 3; i++) {
        // Turn OFF
        if (blink_temp) MAX7219_ClearSensor(0);
        if (blink_rpm) MAX7219_ClearSensor(1);
        if (blink_pressure) MAX7219_ClearSensor(2);
        if (blink_oil1) MAX7219_ClearSensor(3);
        if (blink_oil2) MAX7219_ClearSensor(4);
        HAL_Delay(300);

        // Turn ON (hiển thị gạch ngang)
        if (blink_temp) MAX7219_DashesSensor(0);
        if (blink_rpm) MAX7219_DashesSensor(1);
        if (blink_pressure) MAX7219_DashesSensor(2);
        if (blink_oil1) MAX7219_DashesSensor(3);
        if (blink_oil2) MAX7219_DashesSensor(4);
        HAL_Delay(300);
    }
}

void DisplayManager_Task(void)
{
      // 1. Cập nhật hiển thị MAX7219 (Kèm nháy 500ms khi quá ngưỡng)
      show_temp_warning(Limit_Temp, gSensorData.Temperature);
      
      // Net tiêu hao (L/h x10) = dầu đi − dầu hồi — trần theo cấu hình tàu 350HP
      uint16_t net_flow = (gSensorData.OilForward > gSensorData.OilReturn)
                          ? (gSensorData.OilForward - gSensorData.OilReturn) : 0;
      if (net_flow > OIL_NET_DISPLAY_MAX_XH) {
          net_flow = OIL_NET_DISPLAY_MAX_XH;
      }
      
      // Hiển thị Lưu lượng chênh lệch và cảnh báo nhấp nháy dựa trên thông số này
      if (net_flow < Limit_Oil) {
          show_oil(net_flow);
      } else {
          if (HAL_GetTick() % 1000 < 500) {
              show_oil(net_flow);
          } else {
              // Tắt 3 ô LED của cột Dầu để tạo hiệu ứng nhấp nháy
              MAX7219_SendCommandToChip(1, 0x04, 0x0f);
              MAX7219_SendCommandToChip(1, 0x05, 0x0f);
              MAX7219_SendCommandToChip(1, 0x06, 0x0f);
          }
      }
      
      show_rpm_warning(Limit_RPM, gSensorData.RPM);
    show_pressure_warning(Limit_Pressure, Limit_PressureMin, gSensorData.Pressure);
    show_volt_warning(Limit_VinMax,Limit_VinMin,gSensorData.Vin);

    // 2. Cập nhật nháy cho các LED đơn (Logic 1 là sáng, bình thường luôn sáng)
    bool blink_on = (HAL_GetTick() % 1000) < 500;
    
    // Nếu sai UID, nháy 4 LED: GPS, Bảo dưỡng, Cảnh báo, Wifi (ở đây chỉ có LED cảnh báo/bảo dưỡng, Wifi/GPS do mạch khác điều khiển? Để kiểm tra lại)
    // Tạm thời nếu sai UID, nháy LED MAINTAIN và WARNING liên tục.
    if (!gSystemData.UidMatched) {
        HAL_GPIO_WritePin(LED_TEMP_C_GPIO_Port, LED_TEMP_C_Pin, blink_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_PA_C_GPIO_Port, LED_PA_C_Pin, blink_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_OLDC_GPIO_Port, LED_OLDC_Pin, blink_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_TACHO_GPIO_Port, LED_TACHO_Pin, blink_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_VIN_GPIO_Port, LED_VIN_Pin, blink_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
        return; // Bỏ qua cập nhật bình thường
    }
    
    HAL_GPIO_WritePin(LED_TEMP_C_GPIO_Port, LED_TEMP_C_Pin, 
                      (gAlarmData.TempAlarm && !blink_on) ? GPIO_PIN_RESET : GPIO_PIN_SET);
                      
    HAL_GPIO_WritePin(LED_PA_C_GPIO_Port, LED_PA_C_Pin, 
                      (gAlarmData.PressureAlarm && !blink_on) ? GPIO_PIN_RESET : GPIO_PIN_SET);
                      
    HAL_GPIO_WritePin(LED_OLDC_GPIO_Port, LED_OLDC_Pin, 
                      (gAlarmData.OilAlarm && !blink_on) ? GPIO_PIN_RESET : GPIO_PIN_SET);
                      
    HAL_GPIO_WritePin(LED_TACHO_GPIO_Port, LED_TACHO_Pin, 
                      (gAlarmData.RPMAlarm && !blink_on) ? GPIO_PIN_RESET : GPIO_PIN_SET);
                      
    HAL_GPIO_WritePin(LED_VIN_GPIO_Port, LED_VIN_Pin, 
                      (gAlarmData.VinAlarm && !blink_on) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
