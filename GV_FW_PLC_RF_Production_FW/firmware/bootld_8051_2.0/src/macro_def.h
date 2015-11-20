//#define CPLUSSIM
//#define CPLUSDEBUG
//#define CPLUS_READFILE
//#define KEYBRD_CONSOLE
//#define CPU8051

//#define PALMCHIP_BR
//#define KEILDEBUG



//#define PROGRAM_CONFIGURATION   /*Turn on/off the feature of programming configuration data*/
//#define FEATURE_CHIPERASE       /*Turn on/off the feature of erasing the whole chip*/
#define DNLD_UART2CRAM_FUNC
#define TEST_LEVEL1
#define CRAM_START 0x2100
//#define TEST_MEM_FUNC
//#define LOAD_SFLASH2CRAM_FUNC

#define COMMONADDR_H  	0x9FFF //32K + 8K = 40K
#define COMMONADDR_L  	0x2000 //8k
#define BANKADDR_L   	0xA000 //8K + 32K = 40K
#define BANKADDR_H   	0xFFFF //
#define CBANK_LEN 		0x6000 //=24566   BYTES
#define COMMON_LEN 		0x8000 //=32768 BYTES
#define COM_BANK0_LEN 	0xE000 // 57344 BYTES
#define NUM_OF_BANK 	8
#define SFLASH_BANK1	0x10000
#define SFLASH_CRAM_OFFSET 0x2000

#define SPIFLASHCFG 	0x0304 //

//*************************************

#define GVTY_STARTING_TIMEOUT 80000
#define GVTY_PRG_CONFIG_TIMEOUT 120000

//*************************************

#define SFLASH_PROTECT_ALL 0xBC    /*Winbond, Microchip, Macronix, SST*/
#define SFLASH_PROTECT_L512K 0xB0  /*Winbond, */
#define SFLASH_PROTECT_L1M 0xB4    /*Winbond, */
#define SFLASH_PROTECT_NONE 0x0	   /*Winbond, Microchip, Macronix, SST*/


//*************************************

#define SetDataFF	0xFF	/* Values to set the ERAM before downloading code*/
#define SetData00	0x00

//*************************************

#define GVTY_BACKUP_START_ADDR 0x50000
#define GVTY_BACKUP_END_ADDR   0x8FFFF