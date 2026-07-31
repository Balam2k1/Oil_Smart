#include "max7219.h"
#include "HC595.h"
#include "main.h"
#include "app_config.h"
#include "Flash_definition.h"
#include "w25q32.h"
#include "log.h"

void MAX7219_SelectChip(uint8_t chip) {
    // Đưa tất cả CS về mức HIGH trước
    HAL_GPIO_WritePin(MAX7219_CS1_PORT, MAX7219_CS1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MAX7219_CS2_PORT, MAX7219_CS2_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MAX7219_CS3_PORT, MAX7219_CS3_PIN, GPIO_PIN_SET);

    // Chọn IC tương ứng
    switch (chip) {
        case 1:
            HAL_GPIO_WritePin(MAX7219_CS1_PORT, MAX7219_CS1_PIN, GPIO_PIN_RESET);
            break;
        case 2:
            HAL_GPIO_WritePin(MAX7219_CS2_PORT, MAX7219_CS2_PIN, GPIO_PIN_RESET);
            break;
        case 3:
            HAL_GPIO_WritePin(MAX7219_CS3_PORT, MAX7219_CS3_PIN, GPIO_PIN_RESET);
            break;
    }
}


void MAX7219_SendByteToChip(uint8_t chip, uint8_t data) {
    GPIO_TypeDef *DIN_PORT, *CLK_PORT;
    uint16_t DIN_PIN, CLK_PIN;

    // Xác định chân DIN và CLK của từng IC
    switch (chip) {
        case 1:
            DIN_PORT = MAX7219_DIN1_PORT;
            DIN_PIN = MAX7219_DIN1_PIN;
            CLK_PORT = MAX7219_CLK1_PORT;
            CLK_PIN = MAX7219_CLK1_PIN;
            break;
        case 2:
        	DIN_PORT = MAX7219_DIN2_PORT;
			DIN_PIN = MAX7219_DIN2_PIN;
			CLK_PORT = MAX7219_CLK2_PORT;
			CLK_PIN = MAX7219_CLK2_PIN;
            break;
        case 3:
        	DIN_PORT = MAX7219_DIN3_PORT;
			DIN_PIN = MAX7219_DIN3_PIN;
			CLK_PORT = MAX7219_CLK3_PORT;
			CLK_PIN = MAX7219_CLK3_PIN;
            break;
        default:
            return; // Nếu không đúng IC nào thì thoát
    }

    // Gửi từng bit
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) {
            HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_SET);  // Set DIN
        } else {
            HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_RESET);  // Reset DIN
        }
        for(volatile int d = 0; d < 20; d++); // Delay for setup time

        // Kích xung CLK
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
        for(volatile int d = 0; d < 20; d++); // Delay for pulse width high
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
        for(volatile int d = 0; d < 20; d++); // Delay for pulse width low

        data <<= 1;  // Dịch bit tiếp theo
    }
}


void MAX7219_SendCommandToChip(uint8_t chip, uint8_t address, uint8_t data) {
    MAX7219_SelectChip(chip);               // Chọn IC
    MAX7219_SendByteToChip(chip, address);  // Gửi địa chỉ thanh ghi
    MAX7219_SendByteToChip(chip, data);     // Gửi dữ liệu
    MAX7219_SelectChip(0);                  // Kết thúc giao tiếp
}

void MAX7219_InitAll(void) {
    uint8_t intensity = 0x08; // Default level 8
    uint16_t val = W_Flash_Read_Int(AD_LED_INTENSITY);
    if (val != 0xFFFF && val <= 0x0F) {
        intensity = (uint8_t)val;
    } else {
        W_Flash_Safe_Write_Int(AD_LED_INTENSITY, intensity);
    }
    
    LOG_Printf("MAX7219 LED Intensity: %u/15\r\n", intensity);

    for (uint8_t chip = 1; chip <= 3; chip++) {
        MAX7219_SendCommandToChip(chip, 0x09, 0xFF);  // Chế độ giải mã cho tất cả các vị trí
        MAX7219_SendCommandToChip(chip, 0x0A, intensity); // Độ sáng
        MAX7219_SendCommandToChip(chip, 0x0B, 0x07);  // Quét 8 hàng
        MAX7219_SendCommandToChip(chip, 0x0C, 0x01);  // Bật hiển thị
        MAX7219_SendCommandToChip(chip, 0x0F, 0x00);  // Không dùng chế độ kiểm tra

    }
    show_temp(000);
	show_oil(000);
	show_rpm(0000);
	show_pressure(000);
	show_volt(000);
}


//------------------------------------------------------------------------------------------------------

void MAX7219_SetIntensity(uint8_t level) {
    if (level > 0x0F) level = 0x0F;
    
    for (uint8_t chip = 1; chip <= 3; chip++) {
        MAX7219_SendCommandToChip(chip, 0x0A, level);
    }
    
    W_Flash_Safe_Write_Int(AD_LED_INTENSITY, level);
    LOG_Printf("\r\n--- MAX7219 LED INTENSITY UPDATED TO: %u/15 ---\r\n", level);
}

//------------------------------------------------------------------------------------------------------


void MAX7219_DisplayTest(void) {
	MAX7219_SendCommandToChip(1, 0x01, 0x00);
	MAX7219_SendCommandToChip(1, 0x02, 0x00);
	MAX7219_SendCommandToChip(1, 0x03, 0x00);
	MAX7219_SendCommandToChip(1, 0x04, 0xFF);
	MAX7219_SendCommandToChip(1, 0x05, 0xFF);
	MAX7219_SendCommandToChip(1, 0x06, 0xFF);
	MAX7219_SendCommandToChip(1, 0x07, 0xFF);
	MAX7219_SendCommandToChip(1, 0x08, 0xFF);
	MAX7219_SendCommandToChip(1, 0x01, 0x00);
	MAX7219_SendCommandToChip(1, 0x02, 0xF5);
	MAX7219_SendCommandToChip(1, 0x03, 0x06);


	MAX7219_SendCommandToChip(2, 0x01, 0x00);
	MAX7219_SendCommandToChip(2, 0x02, 0xFF);
	MAX7219_SendCommandToChip(2, 0x03, 0xFF);
	MAX7219_SendCommandToChip(2, 0x04, 0x00);
	MAX7219_SendCommandToChip(2, 0x05, 0xFF);
	MAX7219_SendCommandToChip(2, 0x06, 0xFF);
	MAX7219_SendCommandToChip(2, 0x07, 0xFF);
	MAX7219_SendCommandToChip(2, 0x08, 0xFF);
	MAX7219_SendCommandToChip(2, 0x01, 0x08);
	MAX7219_SendCommandToChip(2, 0x04, 0x09);



	MAX7219_SendCommandToChip(3, 0x01, 0x00);
	MAX7219_SendCommandToChip(3, 0x02, 0xFF);
	MAX7219_SendCommandToChip(3, 0x03, 0xFF);
	MAX7219_SendCommandToChip(3, 0x04, 0x00);
	MAX7219_SendCommandToChip(3, 0x05, 0xFF);
	MAX7219_SendCommandToChip(3, 0x06, 0xFF);
	MAX7219_SendCommandToChip(3, 0x07, 0xFF);
	MAX7219_SendCommandToChip(3, 0x08, 0xFF);
	MAX7219_SendCommandToChip(3, 0x01, 0x06);
	MAX7219_SendCommandToChip(3, 0x04, 0x08);
}

void MAX7219_ClearSensor(uint8_t sensor) {
    uint8_t chip = 0;
    uint8_t start = 0, len = 0;
    switch(sensor) {
        case 0: chip = 1; start = 1; len = 3; break; // Temp
        case 1: chip = 2; start = 1; len = 4; break; // RPM
        case 2: chip = 3; start = 1; len = 3; break; // Pressure
        case 3: chip = 1; start = 4; len = 3; break; // Oil
        case 4: chip = 2; start = 5; len = 3; break; // Volt
        default: return;
    }
    for (int i = 0; i < len; i++) {
        MAX7219_SendCommandToChip(chip, start + i, 0x0F);
    }
}

void MAX7219_DashesSensor(uint8_t sensor) {
    uint8_t chip = 0;
    uint8_t start = 0, len = 0;
    switch(sensor) {
        case 0: chip = 1; start = 1; len = 3; break; // Temp
        case 1: chip = 2; start = 1; len = 4; break; // RPM
        case 2: chip = 3; start = 1; len = 3; break; // Pressure
        case 3: chip = 1; start = 4; len = 3; break; // Oil
        case 4: chip = 2; start = 5; len = 3; break; // Volt
        default: return;
    }
    for (int i = 0; i < len; i++) {
        MAX7219_SendCommandToChip(chip, start + i, 0x0A); // 0x0A is Dash in Decode Mode
    }
}

//---------------------------------------------------------------------------------

void show_temp(unsigned int temp)
{
    if (temp < 1000) {
        // Dưới 100 độ, hiển thị dạng XX.X
        int ch = (temp / 100) % 10;
        int dv = (temp / 10) % 10;
        int tp = temp % 10;

        if (ch == 0) {
            MAX7219_SendCommandToChip(1, 0x01, 0x0f); // Blank số 0 hàng chục
        } else {
            MAX7219_SendCommandToChip(1, 0x01, ch);
        }
        
        MAX7219_SendCommandToChip(1, 0x02, (0xF << 4) | (dv & 0xF));  // Đơn vị + Dấu chấm
        MAX7219_SendCommandToChip(1, 0x03, tp);  // Phần thập phân
    } else {
        // Từ 100 độ trở lên, hiển thị dạng XXX (Bỏ dấu chấm thập phân)
        unsigned int val = temp / 10; // Bỏ phần thập phân
        int tr = (val / 100) % 10;
        int ch = (val / 10) % 10;
        int dv = val % 10;

        if (tr == 0) {
            MAX7219_SendCommandToChip(1, 0x01, 0x0f);
        } else {
            MAX7219_SendCommandToChip(1, 0x01, tr);
        }
        
        MAX7219_SendCommandToChip(1, 0x02, ch);  // Hàng chục
        MAX7219_SendCommandToChip(1, 0x03, dv);  // Hàng đơn vị
    }
}


void show_oil(unsigned int Lit)
{
    if (Lit < 1000) {
        // Dưới 100 L/h, hiển thị dạng XX.X
        int ch = Lit / 100;
        int dv = (Lit / 10) % 10;
        int tp = Lit % 10;

        if (ch == 0) {
            MAX7219_SendCommandToChip(1, 0x04, 0x0f); // Blank số 0 ở hàng chục
        } else {
            MAX7219_SendCommandToChip(1, 0x04, ch);
        }
        
        MAX7219_SendCommandToChip(1, 0x05, dv | 0x80);  // Đơn vị + Dấu thập phân (Bit 7 = 1)
        MAX7219_SendCommandToChip(1, 0x06, tp);  // Phần thập phân
    } else {
        // Từ 100 L/h trở lên, hiển thị dạng XXX (Bỏ dấu chấm thập phân)
        unsigned int val = Lit / 10; // Bỏ phần thập phân
        int tr = (val / 100) % 10;
        int ch = (val / 10) % 10;
        int dv = val % 10;

        if (tr == 0) {
            MAX7219_SendCommandToChip(1, 0x04, 0x0f);
        } else {
            MAX7219_SendCommandToChip(1, 0x04, tr);
        }
        
        MAX7219_SendCommandToChip(1, 0x05, ch);  // Hàng chục (KHÔNG có dấu thập phân)
        MAX7219_SendCommandToChip(1, 0x06, dv);  // Hàng đơn vị
    }
}

void show_rpm( unsigned int rpm)
{



	int ng = rpm / 1000;
	int tr= (rpm/100)%10;
	int ch = (rpm / 10) % 10;
	int dv= rpm %10;

	if(ng==0 && tr==0 && ch == 0){
		MAX7219_SendCommandToChip(2, 0x01, 0x0f);  // Ngàn
		MAX7219_SendCommandToChip(2, 0x02, 0x0f);  // tram
		MAX7219_SendCommandToChip(2, 0x03, 0x0f);  // chuc
		MAX7219_SendCommandToChip(2, 0x04, dv);  // Đơn vị
	}else if(ng==0 && tr==0){
		MAX7219_SendCommandToChip(2, 0x01, 0x0f);  // Ngàn
		MAX7219_SendCommandToChip(2, 0x02, 0x0f);  // tram
		MAX7219_SendCommandToChip(2, 0x03, ch);  // chuc
		MAX7219_SendCommandToChip(2, 0x04, dv);  // Đơn vị

	}else if(ng==0){
		MAX7219_SendCommandToChip(2, 0x01, 0x0f);  // Ngàn
		MAX7219_SendCommandToChip(2, 0x02, tr);  // tram
		MAX7219_SendCommandToChip(2, 0x03, ch);  // chuc
		MAX7219_SendCommandToChip(2, 0x04, dv);  // Đơn vi
	}else{
		MAX7219_SendCommandToChip(2, 0x01, ng);  // Ngàn
		MAX7219_SendCommandToChip(2, 0x02, tr);  // Trăm
		MAX7219_SendCommandToChip(2, 0x03, ch);  // Chục
		MAX7219_SendCommandToChip(2, 0x04, dv);  // Đơn vị
	}

}

void show_pressure(unsigned int Pa)
{

	int ch=Pa/100;
	int dv=(Pa/10)%10;
	int tp= Pa%10;

	    switch (ch) {
			case 0:
				MAX7219_SendCommandToChip(3, 0x04, 0x0f);
				break;
	        case 1:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x01);
	            break;
	        case 2:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x02);
	            break;
	        case 3:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x03);
	            break;
	        case 4:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x04);
	            break;
	        case 5:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x05);
	            break;
	        case 6:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x06);
	            break;
	        case 7:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x07);
	            break;
	        case 8:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x08);
	            break;
	        case 9:
	        	MAX7219_SendCommandToChip(3, 0x04, 0x09);
	            break;
	        default:
	            return; // Nếu không đúng thì thoát
	    }
	    MAX7219_SendCommandToChip(3, 0x05, (0xF << 4) | (dv & 0xF));  // Đơn vị
	    MAX7219_SendCommandToChip(3, 0x06, tp);  // Thập Phân

}

void show_volt(unsigned int vol)
{

	int ch= vol/100;
	int dv=(vol/10)%10;
	int tp= vol%10;

		switch (ch) {
			case 0:
				MAX7219_SendCommandToChip(3, 0x01, 0x0f);
				break;
			case 1:
				MAX7219_SendCommandToChip(3, 0x01, 0x01);
				break;
			case 2:
				MAX7219_SendCommandToChip(3, 0x01, 0x02);
				break;
			case 3:
				MAX7219_SendCommandToChip(3, 0x1, 0x03);
				break;
			case 4:
				MAX7219_SendCommandToChip(3, 0x01, 0x04);
				break;
			case 5:
				MAX7219_SendCommandToChip(3, 0x01, 0x05);
				break;
			case 6:
				MAX7219_SendCommandToChip(3, 0x01, 0x06);
				break;
			case 7:
				MAX7219_SendCommandToChip(3, 0x01, 0x07);
				break;
			case 8:
				MAX7219_SendCommandToChip(3, 0x01, 0x08);
				break;
			case 9:
				MAX7219_SendCommandToChip(3, 0x01, 0x09);
				break;
			default:
				return;
		}
		MAX7219_SendCommandToChip(3, 0x02, (0xF << 4) | (dv & 0xF));  // Đơn vị
		MAX7219_SendCommandToChip(3, 0x03, tp);  // Thập Phân

}

//---------------------------------------------------------------------------------------

void show_volt_warning(unsigned int max,unsigned int min,  unsigned int vol)
{

		if(vol <= max && vol >= min )
		{
			show_volt(vol);

		}else
		{
			if(mSec%1000<500){

				show_volt(vol);

			}else
			{

				MAX7219_SendCommandToChip(3, 0x01, 0x0f);
				MAX7219_SendCommandToChip(3, 0x02, 0x0f);  // Đơn vị
				MAX7219_SendCommandToChip(3, 0x03, 0x0f);  // Thập Phân
			}
		}
}

void show_pressure_warning(unsigned int max, unsigned int min, unsigned int Pa)
{

		if(Pa < max && Pa >= min) // Hiện thị bình thường nếu nằm trong khoảng [min đến max)
		{
				show_pressure(Pa);

		}else // Nhấp nháy nếu >= max (quá áp) HOẶC < 25 (dưới 2.5 Bar)
		{
			if(mSec%1000<500)
			{

				show_pressure(Pa);

			}else
			{

				MAX7219_SendCommandToChip(3, 0x04, 0x0f);
				MAX7219_SendCommandToChip(3, 0x05, 0x0f);  // Đơn vị
				MAX7219_SendCommandToChip(3, 0x06, 0x0f);  // Thập Phân
			}

		}
}

void show_oil_warning(unsigned int max,  unsigned int Lit)
{

		if(Lit< max )
		{

			show_oil(Lit);
		}
		else
		{
			if(mSec%1000<500)
			{

				show_oil(Lit);

			}else
			{

				MAX7219_SendCommandToChip(1, 0x04, 0x0f);
				MAX7219_SendCommandToChip(1, 0x05, 0x0f);
				MAX7219_SendCommandToChip(1, 0x06, 0x0f);
			}
		}
}

void show_temp_warning(unsigned int max,  unsigned int Temp)
{

		if(Temp< max )
		{

			show_temp(Temp);
		}else
		{
			if(mSec%1000<500)
			{

				show_temp(Temp);

			}else
			{

				MAX7219_SendCommandToChip(1, 0x01, 0x0f);
				MAX7219_SendCommandToChip(1, 0x02, 0x0f);
				MAX7219_SendCommandToChip(1, 0x03, 0x0f);
			}
		}
}

void show_rpm_warning(unsigned int max, unsigned int Rpm)
{

		if(Rpm< max )
		{

				show_rpm(Rpm);
		}
		else
		{
			if(mSec%1000<500)
			{

				show_rpm(Rpm);

			}else
			{

				MAX7219_SendCommandToChip(2, 0x01, 0x0f);
				MAX7219_SendCommandToChip(2, 0x02, 0x0f);
				MAX7219_SendCommandToChip(2, 0x03, 0x0f);
				MAX7219_SendCommandToChip(2, 0x04, 0x0f);
			}
		}
}

