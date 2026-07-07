#include "flash_lock.h"
#include "sm.h"

#if(SYS_CONFIG_USE_SDK4_EN == 1)
void flash_lock(int deepRetWakeUp)
{
#if FLASH_LOCK_EN
    u32 mid;

    if(deepRetWakeUp)
    {
        return;
    }
	
	flash_vdd_f_calib();

    mid = flash_read_mid();
    printf("mid = 0x%x\n",mid);
    switch(mid)
    {
        // case 0x1060c8:
        //     flash_lock_mid1060c8(FLASH_LOCK_ALL_64K_MID1060C8);
        //     break;

        case 0x1360c8:
            flash_lock_mid1360c8(FLASH_LOCK_LOW_256K_MID1360C8);
            break;

        case 0x1360eb:
            flash_lock_mid1360eb(FLASH_LOCK_LOW_256K_MID1360EB);
            break;

        // case 0x1460c8:
        //     flash_lock_mid1460c8(FLASH_LOCK_LOW_768K_MID1460C8);
        //     break;

        case 0x11460c8:
            flash_lock_mid011460c8(FLASH_LOCK_LOW_256K_MID011460C8);
            break;

        case 0x13325e:
            flash_lock_mid13325e(FLASH_LOCK_LOW_256K_MID13325E);
            break;

        // case 0x14325e:
        //     flash_lock_mid14325e(FLASH_LOCK_LOW_768K_MID14325E);
        //     break;

        case 0x134051:
            flash_lock_mid134051(FLASH_LOCK_LOW_256K_MID134051);
            break;

        case 0x136085:
            flash_lock_mid136085(FLASH_LOCK_LOW_256K_MID136085);
            break;

        default:
            printf("Flash MID Error!!!\n");
            break;
    }
#endif
}


void flash_unlock(void)
{
#if FLASH_LOCK_EN
    u32 mid;
    mid = flash_read_mid();

    switch(mid)
    {
        case 0x1060c8:
            flash_unlock_mid1060c8();
            break;

        case 0x1360c8:
            flash_unlock_mid1360c8();
            break;

        case 0x1360eb:
            flash_unlock_mid1360eb();
            break;

        case 0x1460c8:
            flash_unlock_mid1460c8();
            break;

        case 0x11460c8:
            flash_unlock_mid011460c8();
            break;

        case 0x13325e:
            flash_unlock_mid13325e();
            break;

        case 0x14325e:
            flash_unlock_mid14325e();
            break;

        case 0x134051:
            flash_unlock_mid134051();
            break;

        case 0x136085:
            flash_unlock_mid136085();
            break;

        default:
            break;
    }
#endif
}
#endif

