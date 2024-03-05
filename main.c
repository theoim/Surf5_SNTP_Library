/**
 ******************************************************************************
 * @file    WZTOE/WZTOE_DHCPClient/main.c
 * @author  WIZnet
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2018 WIZnet</center></h2>
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "wizchip_conf.h"
#include "dhcp.h"
#include "sntp.h"

/** @addtogroup W7500x_StdPeriph_Examples
 * @{
 */

/** @addtogroup WZTOE_DHCPClient
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DATA_BUF_SIZE 2048
/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_SNTP 0

/* Timeout */
#define RECV_TIMEOUT (1000 * 10) // 10 seconds

/* Timezone */
#define TIMEZONE 40 // Korea
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
uint8_t test_buf[DATA_BUF_SIZE];
wiz_NetInfo gWIZNETINFO;
extern volatile uint32_t sys_cnt;

/* SNTP */
static uint8_t g_sntp_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_sntp_server_ip[4] = {216, 239, 35, 0}; // time.google.com
/* Private function prototypes -----------------------------------------------*/
static void UART_Config(void);
static void Network_Config(void);
void dhcp_assign(void);
void dhcp_update(void);
void dhcp_conflict(void);
void delay(__IO uint32_t milliseconds);
void TimingDelay_Decrement(void);
static time_t millis(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    uint32_t ret;
    uint8_t dhcp_retry = 0;
        /* Initialize */
    uint8_t retval = 0;
    uint32_t start_ms = 0;
    datetime time;
    SystemInit();

    /* SysTick_Config */
    SysTick_Config((GetSystemClock() / 1000));

    /* Set WZ_100US Register */
    setTIC100US((GetSystemClock() / 10000));

    UART_Config();

    printf("W7500x Standard Peripheral Library version : %d.%d.%d\r\n", __W7500X_STDPERIPH_VERSION_MAIN, __W7500X_STDPERIPH_VERSION_SUB1, __W7500X_STDPERIPH_VERSION_SUB2);

    printf("SourceClock : %d\r\n", (int) GetSourceClock());
    printf("SystemClock : %d\r\n", (int) GetSystemClock());

    /* Initialize PHY */
#ifdef W7500
    printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_15, GPIO_Pin_14) == SET ? "Success" : "Fail");
#elif defined (W7500P)
    printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_14, GPIO_Pin_15) == SET ? "Success" : "Fail");
#endif

    /* Check Link */
    printf("Link : %s\r\n", PHY_GetLinkStatus() == PHY_LINK_ON ? "On" : "Off");

    /* Network information setting before DHCP operation. Set only MAC. */
    Network_Config();

    /* DHCP Process */
    DHCP_init(0, test_buf);
    reg_dhcp_cbfunc(dhcp_assign, dhcp_assign, dhcp_conflict);
    if (gWIZNETINFO.dhcp == NETINFO_DHCP) {       // DHCP
        printf("Start DHCP\r\n");
        while (1) {
            ret = DHCP_run();

            if (ret == DHCP_IP_LEASED) {
                printf("DHCP Success\r\n");
                break;
            }
            else if (ret == DHCP_FAILED) {
                dhcp_retry++;
            }

            if (dhcp_retry > 3) {
                printf("DHCP Fail\r\n");
                break;
            }
        }
    }
 SNTP_init(SOCKET_SNTP, g_sntp_server_ip, TIMEZONE, g_sntp_buf);
    start_ms = millis();
    /* Network information setting */
    Network_Config();   
    /* Get time */

    while(1){
        do
        {
            retval = SNTP_run(&time);

            if (retval == 1)
            {
                break;
            }
        } while ((millis() - start_ms) < RECV_TIMEOUT);

        if (retval != 1)
        {
            printf(" SNTP failed : %d\n", retval);

            while (1)
                ;
        }
        delay(1000);

    }
    

    
	
	return 0;
}

/**
 * @brief  Configures the UART Peripheral.
 * @note
 * @param  None
 * @retval None
 */
static void UART_Config(void)
{
    UART_InitTypeDef UART_InitStructure;

    UART_StructInit(&UART_InitStructure);

#if defined (USE_WIZWIKI_W7500_EVAL)
    UART_Init(UART1, &UART_InitStructure);
    UART_Cmd(UART1, ENABLE);
#else
    S_UART_Init(115200);
    S_UART_Cmd(ENABLE);
#endif
}


/**
 * @brief  Configures the Network Information.
 * @note
 * @param  None
 * @retval None
 */
static void Network_Config(void)
{
    uint8_t mac_addr[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 };

    memcpy(gWIZNETINFO.mac, mac_addr, 6);
    gWIZNETINFO.dhcp = NETINFO_DHCP;

    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);

    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
    printf("IP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    printf("GW: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
    printf("SN: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
    printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
}

/**
 * @brief  The call back function of ip assign.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_assign(void)
{
    getIPfromDHCP(gWIZNETINFO.ip);
    getGWfromDHCP(gWIZNETINFO.gw);
    getSNfromDHCP(gWIZNETINFO.sn);
    getDNSfromDHCP(gWIZNETINFO.dns);

    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
}

/**
 * @brief  The call back function of ip update.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_update(void)
{
    ;
}

/**
 * @brief  The call back function of ip conflict.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_conflict(void)
{
    ;
}

/**
 * @brief  Inserts a delay time.
 * @param  nTime: specifies the delay time length, in milliseconds.
 * @retval None
 */
void delay(__IO uint32_t milliseconds)
{
    TimingDelay = milliseconds;

    while (TimingDelay != 0)
        ;
}

/**
 * @brief  Decrements the TimingDelay variable.
 * @param  None
 * @retval None
 */
void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00) {
        TimingDelay--;
    }
}

static time_t millis(void)
{
    return sys_cnt;
}
#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

/**
 * @}
 */

/**
 * @}
 */

/******************** (C) COPYRIGHT WIZnet *****END OF FILE********************/


// /**
//  ******************************************************************************
//  * @file    WZTOE/WZTOE_SNTP/main.c
//  * @author  WIZnet
//  * @brief   Main program body
//  ******************************************************************************
//  * @attention
//  *
//  * <h2><center>&copy; COPYRIGHT 2018 WIZnet</center></h2>
//  *
//  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  * THE POSSIBILITY OF SUCH DAMAGE.
//  *
//  ******************************************************************************
//  */
// /* Includes ------------------------------------------------------------------*/
// #include "main.h"
// #include "wizchip_conf.h"
// #include "sntp.h"
// #include "loopback.h"

// /** @addtogroup W7500x_StdPeriph_Examples
//  * @{
//  */

// /** @addtogroup WZTOE_Loopback
//  * @{
//  */

// /* Private typedef -----------------------------------------------------------*/
// /* Private define ------------------------------------------------------------*/
// #define DATA_BUF_SIZE 2048


// /* Buffer */
// #define ETHERNET_BUF_MAX_SIZE (1024 * 2)

// /* Socket */
// #define SOCKET_SNTP 0

// /* Timeout */
// #define RECV_TIMEOUT (1000 * 10) // 10 seconds

// /* Timezone */
// #define TIMEZONE 40 // Korea

// /* Private macro -------------------------------------------------------------*/
// /* Private variables ---------------------------------------------------------*/
// static __IO uint32_t TimingDelay;
// uint8_t test_buf[DATA_BUF_SIZE];
// extern volatile uint32_t sys_counter;

// /* SNTP */
// static uint8_t g_sntp_buf[ETHERNET_BUF_MAX_SIZE] = {
//     0,
// };
// static uint8_t g_sntp_server_ip[4] = {216, 239, 35, 0}; // time.google.com


// /* Private function prototypes -----------------------------------------------*/
// static void UART_Config(void);
// static void Network_Config(void);
// void delay(__IO uint32_t milliseconds);
// void TimingDelay_Decrement(void);
// static time_t millis(void);

// /* Private functions ---------------------------------------------------------*/

// /**
//  * @brief  Main program.
//  * @param  None
//  * @retval None
//  */
// int main(void)
// {
//     /* Initialize */
//     uint8_t retval = 0;
//     uint32_t start_ms = 0;
//     datetime time;

//     SystemInit();

//     /* SysTick_Config */
//     SysTick_Config((GetSystemClock() / 1000));

//     /* Set WZ_100US Register */
//     setTIC100US((GetSystemClock() / 10000));

//     UART_Config(); 
 
//     printf("W7500x Standard Peripheral Library version : %d.%d.%d\r\n", __W7500X_STDPERIPH_VERSION_MAIN, __W7500X_STDPERIPH_VERSION_SUB1, __W7500X_STDPERIPH_VERSION_SUB2);

//     printf("SourceClock : %d\r\n", (int) GetSourceClock());
//     printf("SystemClock : %d\r\n", (int) GetSystemClock());
 
//     /* Initialize PHY */ 
// #ifdef W7500
//     printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_15, GPIO_Pin_14) == SET ? "Success" : "Fail");
// #elif defined (W7500P)
//     printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_14, GPIO_Pin_15) == SET ? "Success" : "Fail");
// #endif

//     /* Check Link */
//     printf("Link : %s\r\n", PHY_GetLinkStatus() == PHY_LINK_ON ? "On" : "Off");



//     SNTP_init(SOCKET_SNTP, g_sntp_server_ip, TIMEZONE, g_sntp_buf);
//     start_ms = millis();
//     /* Network information setting */
//     Network_Config();  
//     /* Get time */
//     do
//     {
//         retval = SNTP_run(&time);

//         if (retval == 1)
//         {
//             break;
//         }
//     } while ((millis() - start_ms) < RECV_TIMEOUT);

//     if (retval != 1)
//     {
//         printf(" SNTP failed : %d\n", retval);

//         while (1)
//             ;
//     }

//     printf(" %d-%d-%d, %d:%d:%d\n", time.yy, time.mo, time.dd, time.hh, time.mm, time.ss);

//     while (1) {
//         ;
//     }
	
// 	return 0;
// }

// /**
//  * @brief  Configures the UART Peripheral.
//  * @note
//  * @param  None
//  * @retval None
//  */
// static void UART_Config(void)
// {
//     UART_InitTypeDef UART_InitStructure;

//     UART_StructInit(&UART_InitStructure);

// #if defined (USE_WIZWIKI_W7500_EVAL)
//     UART_Init(UART1, &UART_InitStructure);
//     UART_Cmd(UART1, ENABLE);
// #else
//     S_UART_Init(115200);
//     S_UART_Cmd(ENABLE);
// #endif
// }

// /**
//  * @brief  Configures the Network Information.
//  * @note
//  * @param  None
//  * @retval None
//  */
// static void Network_Config(void)
// {
//     wiz_NetInfo gWIZNETINFO;

//     uint8_t mac_addr[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 };
//     uint8_t ip_addr[4] = { 192, 168, 0, 15 };
//     uint8_t gw_addr[4] = { 192, 168, 0, 1 }; 
//     uint8_t sub_addr[4] = { 255, 255, 255, 0 };
//     uint8_t dns_addr[4] = { 8, 8, 8, 8 };

//     memcpy(gWIZNETINFO.mac, mac_addr, 6);
//     memcpy(gWIZNETINFO.ip, ip_addr, 4);
//     memcpy(gWIZNETINFO.gw, gw_addr, 4);
//     memcpy(gWIZNETINFO.sn, sub_addr, 4);
//     memcpy(gWIZNETINFO.dns, dns_addr, 4);
//     gWIZNETINFO.dhcp = NETINFO_STATIC;

//     ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);

//     printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
//     printf("IP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
//     printf("GW: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
//     printf("SN: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
//     printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
// }

// /**
//  * @brief  Inserts a delay time.
//  * @param  nTime: specifies the delay time length, in milliseconds.
//  * @retval None
//  */
// void delay(__IO uint32_t milliseconds)
// {
//     TimingDelay = milliseconds;

//     while (TimingDelay != 0)
//         ;
// }

// /**
//  * @brief  Decrements the TimingDelay variable.
//  * @param  None
//  * @retval None
//  */
// void TimingDelay_Decrement(void)
// {
//     if (TimingDelay != 0x00) {
//         TimingDelay--;
//     }
// }

// static time_t millis(void)
// {
//     return sys_counter;
// }

// #ifdef  USE_FULL_ASSERT

// /**
//  * @brief  Reports the name of the source file and the source line number
//  *         where the assert_param error has occurred.
//  * @param  file: pointer to the source file name
//  * @param  line: assert_param error line source number
//  * @retval None
//  */
// void assert_failed(uint8_t* file, uint32_t line)
// {
//     /* User can add his own implementation to report the file name and line number,
//      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

//     /* Infinite loop */
//     while (1)
//     {
//     }
// }
// #endif

// /**
//  * @}
//  */

// /**
//  * @}
//  */

// /******************** (C) COPYRIGHT WIZnet *****END OF FILE********************/
