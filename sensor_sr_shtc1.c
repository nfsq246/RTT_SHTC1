
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
/**
 * TO USE CONSOLE OUTPUT (PRINTF) AND WAIT (SLEEP) PLEASE ADAPT THEM TO YOUR
 * PLATFORM
 */


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
        }
    }
    else
    {
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
        return 0;
}

static rt_err_t _shtc1_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    return -RT_ERROR;
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
            LOG_W("SHT sensor probing failed\n"); 
            return -RT_ERROR;
            }
        }

        sensor_temp = rt_calloc(1, sizeof(struct rt_sensor_device));
        if (sensor_temp == RT_NULL)
                return -RT_ERROR;

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
                goto __exit;

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
