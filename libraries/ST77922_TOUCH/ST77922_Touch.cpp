#include "ST77922_Touch.h"

i2c_master_dev_handle_t touch_handle;

ST77922_TOUCH::ST77922_TOUCH(void)
{
	width = TOUCH_WIDTH;
	height = TOUCH_HEIGHT;
	rotation = 0;
	max_points = 0;
	i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t touch_i2c_cfg = {
        .i2c_port = I2C_NUM,
		.sda_io_num = TOUCH_SDA,
        .scl_io_num = TOUCH_SCL, 
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags=
        {
        	.enable_internal_pullup = true,
        }
    };
    i2c_new_master_bus(&touch_i2c_cfg, &bus_handle);
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_ADDR,
        .scl_speed_hz = I2C_SPEED,
    };
    i2c_master_bus_add_device(bus_handle, &dev_config, &touch_handle);
}

void ST77922_TOUCH::init(void)
{
	uint8_t data;
	pinMode(TOUCH_RST, OUTPUT);
	digitalWrite(TOUCH_RST, HIGH);
 	pinMode(TOUCH_INT, INPUT);
	reset();
	do
	{
		Read_Data(touch_handle, STATUS, &data, 1);
	}while(data&0x0F);
	Read_Data(touch_handle, MAX_TOUCHES, &data, 1);
	max_points = data;
}

void ST77922_TOUCH::reset(void)
{
	TOUCH_RST_LOW;
	delay(100);
	TOUCH_RST_HIGH;
	delay(100);
}

void ST77922_TOUCH::Set_Rotation(uint8_t r)
{
	rotation = r;
	switch(rotation)
	{
		case 0:
		case 2:
			width = TOUCH_WIDTH;
			height = TOUCH_HEIGHT;
			break;
		case 1:
		case 3:
			width = TOUCH_HEIGHT;
			height = TOUCH_WIDTH;
			break;			
		default:
			break;
	}
}

void ST77922_TOUCH::Read_Data(i2c_master_dev_handle_t dev, uint16_t reg, uint8_t* rbuf, size_t rlen)
{
	const uint8_t wbuf[2] = {(uint8_t)((reg>>8)&0xFF), (uint8_t)(reg&0xFF)};
	i2c_master_transmit_receive(dev, wbuf, 2, rbuf, rlen, 1000);
}

bool ST77922_TOUCH::Get_Touch(void)
{
	bool result;
	uint8_t i = 0;
	uint8_t data[7*MAX_TOUCH_POINTS] = {0};
	uint8_t update = 0;
	Read_Data(touch_handle, TOUCH_INFO, &update, 1);
	if(update&0x08)
	{
		Read_Data(touch_handle, TOUCH_POINT0, data, 7*max_points);
		for(i=0; i<max_points; i++)
		{
			if(data[i*7]&0x80)
			{
				touch.id[i] = (data[i*7]&0x80)>>7;
				switch(rotation)
				{
					case 0:
						touch.x[i] = ((data[i*7]&0x3F)<<8) | data[i*7+1];
						touch.y[i] = ((data[i*7+2]&0x3F)<<8) | data[i*7+3];
						break;
					case 1:
						touch.x[i] = ((data[i*7+2]&0x3F)<<8) | data[i*7+3];
						touch.y[i] = height - (((data[i*7]&0x3F)<<8) | data[i*7+1]);
						break;
					case 2:
						touch.x[i] = width - (((data[i*7]&0x3F)<<8) | data[i*7+1]);
						touch.y[i] = height - (((data[i*7+2]&0x3F)<<8) | data[i*7+3]);
						break;
					case 3:
						touch.x[i] = width - ((data[i*7+2]&0x3F)<<8) | data[i*7+3];
						touch.y[i] = ((data[i*7]&0x3F)<<8) | data[i*7+1];
						break;
					default:
					break;
				}
			}
		}
		result = true;
	}
	else
	{
		result = false;
	}
	return result;
}