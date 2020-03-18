
#include "sensor_sr_shtc1.h"
#include "shtc1.h"
#include "stdio.h"

#define DBG_ENABLE
#define DBG_LEVEL DBG_INFO
#define DBG_SECTION_NAME  "sensor.sr.shtc1"
#define DBG_COLOR
#include <rtdbg.h>

#define SENSOR_HUMI_RANGE_MAX 80
#define SENSOR_HUMI_RANGE_MIN 20
#define SENSOR_TEMP_RANGE_MAX 60
#define SENSOR_TEMP_RANGE_MIN 5

static rt_size_t _shtc1_polling_get_data(rt_sensor_t sensor, struct rt_sensor_data *data)
{

    if (sensor->info.type == RT_SENSOR_CLASS_TEMP)
    {

        int32_t temperature, humidity;
        /* Measure temperature and relative humidity and store into variables
         * temperature, humidity .
         */
        int8_t ret = shtc1_measure_blocking_read(&temperature, &humidity);
        if (ret == STATUS_OK) {

            data->type = RT_SENSOR_CLASS_TEMP;
            data->data.temp = temperature/100;
            data->timestamp = rt_sensor_get_ts();
        } else {
             LOG_W("error reading measurement\n"); 
             return 0;
        }

    }
    else if (sensor->info.type == RT_SENSOR_CLASS_HUMI)
    {
        int32_t temperature, humidity;
        /* Measure temperature and relative humidity and store into variables
         * temperature, humidity .
         */
        int8_t ret = shtc1_measure_blocking_read(&temperature, &humidity);
        if (ret == STATUS_OK) {

            data->type = RT_SENSOR_CLASS_HUMI;
            data->data.humi = humidity/100;
            data->timestamp = rt_sensor_get_ts();
        } else {
             LOG_W("error reading measurement\n"); 
             return 0;
        }
    }
    else
    {
        LOG_E("only RT_SENSOR_CLASS_TEMP,RT_SENSOR_CLASS_HUMI could get");
        return 0;			
    }
    return 1;
}
static rt_size_t _shtc1_fetch_data(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    RT_ASSERT(buf);

    if (sensor->config.mode == RT_SENSOR_MODE_POLLING)
    {
        return _shtc1_polling_get_data(sensor, buf);
    }
    else
    {
        LOG_E("only RT_SENSOR_MODE_POLLING could get");
        return 0;
    }
        
}
static rt_err_t _shtc1_get_id(rt_sensor_t sensor, uint16_t * args)
{
    if(shtc1_getid(args)==0)
    {
        LOG_E("shtc1_getid error");
        return -RT_ERROR;
    }
    return RT_EOK;
}
static rt_err_t _shtc1_set_POWER(rt_sensor_t sensor, rt_uint32_t args)
{
    if(args==RT_SENSOR_POWER_LOW||args==RT_SENSOR_POWER_NORMAL)
    {
        /* Always set the power mode after setting the configuration */
        if(args==RT_SENSOR_POWER_LOW)
        {
            shtc1_enable_low_power_mode(1);
        }
        else if(args==RT_SENSOR_POWER_NORMAL)
        {
            shtc1_enable_low_power_mode(0);
        }
        else
        {
            LOG_E("only RT_SENSOR_POWER_LOW,RT_SENSOR_POWER_NORMAL could set");
            return -RT_ERROR;
        }
    }
    else if (args==RT_SENSOR_POWER_DOWN)
    {
        return -RT_ERROR;
    }
    else
    {
//        LOG_E("only RT_SENSOR_POWER_LOW,RT_SENSOR_POWER_NORMAL could set");
        return -RT_ERROR;
    }
    return RT_EOK;
}
static rt_err_t _shtc1_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    switch (cmd)
    {
    case RT_SENSOR_CTRL_GET_ID:
        result = _shtc1_get_id(sensor, args);
        break;
    // case RT_SENSOR_CTRL_SET_RANGE:
    //     result = -RT_ERROR;
    //     break;
    // case RT_SENSOR_CTRL_SET_ODR:
    //     result = -RT_ERROR;
    //     break;
    case RT_SENSOR_CTRL_SET_MODE:
        break;
    case RT_SENSOR_CTRL_SET_POWER:
        result = _shtc1_set_POWER(sensor,(rt_uint32_t)args & 0xff);
        break;
    // case RT_SENSOR_CTRL_SELF_TEST:
    //     result = -RT_ERROR;
//        break;
    default:
//        LOG_E("only RT_SENSOR_CTRL_GET_ID,RT_SENSOR_CTRL_SET_POWER could set");
        return -RT_ERROR;
    }
    return result;
}
static struct rt_sensor_ops sensor_ops =
{
    _shtc1_fetch_data,
    _shtc1_control
};

int rt_hw_shtc1_init(const char *name, struct rt_sensor_config *cfg)
{
    rt_int8_t result;
    rt_sensor_t sensor_humi = RT_NULL, sensor_temp = RT_NULL;


    /* Initialize the i2c bus for the current platform */
    if(sensirion_i2c_select_bus(&cfg->intf)!=RT_EOK)
    {
        LOG_E("can't find shtc1 %s device\r\n",cfg->intf.dev_name);
        return -RT_ERROR;
    }
    else
    {
        if (shtc1_probe() != STATUS_OK) 
        {
            rt_thread_delay(500);
            if (shtc1_probe() != STATUS_OK) 
            {
                LOG_E("SHT sensor probing failed\n"); 
                return -RT_ERROR;
            }
        }

        sensor_temp = rt_calloc(1, sizeof(struct rt_sensor_device));
        if (sensor_temp == RT_NULL)
        {
            LOG_E("rt_calloc failed\n"); 
            return -RT_ERROR;
        }
                

        sensor_temp->info.type       = RT_SENSOR_CLASS_TEMP;
        sensor_temp->info.vendor     = RT_SENSOR_VENDOR_SENSIRION;
        sensor_temp->info.model      = "shtc1_temp";
        sensor_temp->info.unit       = RT_SENSOR_UNIT_DCELSIUS;
        sensor_temp->info.intf_type  = RT_SENSOR_INTF_I2C;
        sensor_temp->info.range_max  = SENSOR_TEMP_RANGE_MAX;
        sensor_temp->info.range_min  = SENSOR_TEMP_RANGE_MIN;
        sensor_temp->info.period_min = 0;

        rt_memcpy(&sensor_temp->config, cfg, sizeof(struct rt_sensor_config));
        sensor_temp->ops = &sensor_ops;

        result = rt_hw_sensor_register(sensor_temp, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
        if (result != RT_EOK)
        {
                LOG_E("device register err code: %d", result);
                goto __exit;
        }

        sensor_humi = rt_calloc(1, sizeof(struct rt_sensor_device));
        if (sensor_humi == RT_NULL)
        {
            LOG_E("rt_calloc failed\n"); 
            goto __exit;
        }


        sensor_humi->info.type       = RT_SENSOR_CLASS_HUMI;
        sensor_humi->info.vendor     = RT_SENSOR_VENDOR_SENSIRION;
        sensor_humi->info.model      = "shtc1_humi";
        sensor_humi->info.unit       = RT_SENSOR_UNIT_PERMILLAGE;
        sensor_humi->info.intf_type  = RT_SENSOR_INTF_I2C;
        sensor_humi->info.range_max  = SENSOR_HUMI_RANGE_MAX;
        sensor_humi->info.range_min  = SENSOR_HUMI_RANGE_MIN;
        sensor_humi->info.period_min = 0;

        rt_memcpy(&sensor_humi->config, cfg, sizeof(struct rt_sensor_config));
        sensor_humi->ops = &sensor_ops;

        result = rt_hw_sensor_register(sensor_humi, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
        if (result != RT_EOK)
        {
                LOG_E("device register err code: %d", result);
                goto __exit;
        }

    }

    LOG_I("shtc1_sensor init success");
    return RT_EOK;

__exit:
    if (sensor_humi)
        rt_free(sensor_humi);
    if (sensor_temp)
        rt_free(sensor_temp);

    return -RT_ERROR;
}
