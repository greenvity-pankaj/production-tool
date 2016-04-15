#ifndef _GV701X_SPI_COMMON_H_
#define _GV701X_SPI_COMMON_H_

struct gv701x_spi_drv_intf
{
    //int (*init)( void);
    //int (*exit)( void);
    ssize_t (*tx)(const char *buf, size_t count);
    int (*rx)(u8 *p_data, u16 len);
};
#endif // _GV701X_SPI_COMMON_H_
