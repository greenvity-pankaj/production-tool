
extern void test_cram(void);
extern void load_sflash2cram();
extern void dump_code(u8);
extern void Program_Config_Data();
extern u32 swUtil_SwapEndian(u32);
extern void Load_Config_Data(u8);
extern u32 swUtil_SwapEndian(u32);
extern void memUtil_ClearEram(u8);
extern void Download_Uart2Sflash(u8);
extern u8 spiflash_BackupCodeImage(u8);
extern u8 spiflash_RestoreCodeImage(u8);
extern void dump_BackupCode();


