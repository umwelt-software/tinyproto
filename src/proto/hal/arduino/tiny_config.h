#ifndef __TINY_CONFIG_H__
#define __TINY_CONFIG_H__

#ifdef ARDUINO_AVR_NANO
/////////////////////////////////////////////////////////////////
//      Tiny protocol configuration for Nano boards
/////////////////////////////////////////////////////////////////

#   define CONFIG_ENABLE_CHECKSUM

#else
/////////////////////////////////////////////////////////////////
//      Tiny protocol configuration for All other boards
/////////////////////////////////////////////////////////////////

#ifndef CONFIG_ENABLE_CHECKSUM
#   define CONFIG_ENABLE_CHECKSUM
#endif

#ifndef CONFIG_ENABLE_FCS16
#   define CONFIG_ENABLE_FCS16
#endif

#ifndef CONFIG_ENABLE_FCS32
#   define CONFIG_ENABLE_FCS32
#endif

#endif


#endif /* __TINY_CONFIG_H__ */

