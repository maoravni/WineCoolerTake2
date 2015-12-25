/* ===========================================================================================
 * Digital Thermometer demo project
 *
 * uses
 * DS18x4: MAXIM DS18B20 Digital Thermometer component (v 0.0)
 * Read temperature simultaneously from up to 8 DS18B20 sensors
 * 
 *   
 *
 * ===========================================================================================
 * PROVIDED AS-IS, NO WARRANTY OF ANY KIND, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * FREE TO SHARE, USE AND MODIFY UNDER TERMS: CREATIVE COMMONS - SHARE ALIKE
 * ===========================================================================================
*/

#include <project.h>
//#include <stdio.h> //sprintf

/*
#if defined (__GNUC__)
    // Add an explicit reference to the floating point printf library
    // to allow the usage of floating point conversion specifiers.
    // This is not linked in by default with the newlib-nano library.
    asm (".global _printf_float");
#endif
*/

static const int PwmPeriodArray[] = {200, 0, 800, 1000};

#define M_MIN_SETPOINT 10
#define M_MAX_SETPOINT 20
#define M_MIN_TEMPERATURE 0
#define M_MAX_TEMPERATURE 50

#define M_SLEEP_INTERVAL 3*60
#define M_MAX_COOL_INTERVAL 2*60

#define M_STATE_SLEEP 0
#define M_STATE_MONITOR 1
#define M_STATE_COOLING 2
#define M_STATE_ERROR 3


int secCounter = 0;
int msecCounter = 0;

void TimerInterrupt_Interrupt_InterruptCallback()
{
    ++msecCounter;
    if (msecCounter >= 1000)
    {
        ++secCounter;
        msecCounter = 0;
    }
}


//===========================================
// Function prototypes
//===========================================

#define M_BRIGHTNESS 10

void Initialize(void)
{
    CyGlobalIntEnable;  // Uncomment this line to enable global interrupts.
        
    UART_1_Start();
    CyDelay(100);// waiting for clear start after power on
    UART_1_PutCRLF(' ');
    UART_1_PutString("Temperature sensor Maxim DS18B20:\r\n");
    
    OneWire_Start();
    OneWire_SendTemperatureRequest();

    LED_Driver_Start();
    
    LED_Driver_SetBrightness(M_BRIGHTNESS, 0);
    LED_Driver_SetBrightness(M_BRIGHTNESS, 1);
    LED_Driver_SetBrightness(M_BRIGHTNESS, 2);
    LED_Driver_SetBrightness(M_BRIGHTNESS, 3);

    Clock1kHz_Start();

    TimerInterrupt_Start();
    
    
}
float g_temperature;
float g_setpoint = 14;

#define M_FILTER_COEFF (1.0/10.0)

float lowPassFilter(float newVal, float prevVal)
{
    return M_FILTER_COEFF * newVal + (1 - M_FILTER_COEFF) * prevVal;
}

int monitorState = M_STATE_SLEEP;

void readAndDisplayTemperature()
{
    static int firstRead = 1;
    
    if (OneWire_DataReady) // DS18 completed temperature measurement - begin read dataa
	{   
        OneWire_ReadTemperature();
        if (firstRead)
        {
            g_temperature = OneWire_GetTemperatureAsFloat(0);
            firstRead = 0;
        }
        else
            g_temperature = lowPassFilter(OneWire_GetTemperatureAsFloat(0), g_temperature);
        LED_Driver_Write7SegNumberDec((int)(g_temperature*100), 0, 4, 0);
        LED_Driver_PutDecimalPoint(1, 1);
        OneWire_SendTemperatureRequest();
    }    
    if (msecCounter < PwmPeriodArray[monitorState])
        LED_Driver_PutDecimalPoint(1, 3);
    else
        LED_Driver_PutDecimalPoint(0, 3);
        
}

int main()
{

    Initialize();
    
    for(;;) 
    { 
        readAndDisplayTemperature();
        
        if (g_temperature < M_MIN_TEMPERATURE || g_temperature > M_MAX_TEMPERATURE)
        {
            monitorState = M_STATE_ERROR;
            SsrOut_Write(0);
        }

        switch (monitorState)
        {
            case M_STATE_SLEEP:
            {
                if (secCounter >= M_SLEEP_INTERVAL)
                {
                    monitorState = M_STATE_MONITOR;
                }
                break;
            }
            case M_STATE_MONITOR:
            {
                if (g_temperature > (g_setpoint))
                {
                    monitorState = M_STATE_COOLING;
                    secCounter = 0;
                    SsrOut_Write(1);
                }
                break;
            }
            // קירורww
            case M_STATE_COOLING:
            {
                if (/*temperature <= g_setpoint || */secCounter >= M_MAX_COOL_INTERVAL)
                {
                    monitorState = M_STATE_SLEEP;
                    secCounter = 0;
                    SsrOut_Write(0);
                }
                break;
            }
            case M_STATE_ERROR:
            {
                if (g_temperature > M_MIN_SETPOINT && g_temperature < M_MAX_TEMPERATURE)
                {
                    monitorState = M_STATE_SLEEP;
                    secCounter = 0;
                }
                break;
            }
        }
    }   
    
    return 0;
} //main





/* [] END OF FILE */
