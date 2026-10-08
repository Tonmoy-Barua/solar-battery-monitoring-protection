#include <xc.h>

#define _XTAL_FREQ 4000000

// ============================================================
// PROTECTION THRESHOLDS
// ============================================================

#define LOW_LIMIT_MV             10500
#define HIGH_LIMIT_MV            14400

#define OVERCURRENT_TRIP_MA      1500

#define MAX_RETRIES              3
#define COOLDOWN_MS              3000

#define CURRENT_ZERO_DEADBAND_MA 20

#define CHARGE_STOP_MV           14400
#define CHARGE_RESUME_MV         13800


// ============================================================
// PROTECTION STATES
// ============================================================

#define STATE_NORMAL       0
#define STATE_COOLDOWN     1
#define STATE_RETRY        2
#define STATE_FAULT_LOCK   3


// ============================================================
// PIC CONFIGURATION
// ============================================================

#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF


// ============================================================
// LCD CONNECTIONS
// ============================================================

#define LCD_RS RB1
#define LCD_EN RB2


// ============================================================
// LCD FUNCTIONS
// ============================================================

void LCD_Pulse(void)
{
    LCD_EN = 1;
    __delay_us(2);

    LCD_EN = 0;
    __delay_us(100);
}


void LCD_Send4Bits(unsigned char data)
{
    RB4 = (data >> 0) & 1;
    RB5 = (data >> 1) & 1;
    RB6 = (data >> 2) & 1;
    RB7 = (data >> 3) & 1;

    LCD_Pulse();
}


void LCD_Command(unsigned char command)
{
    LCD_RS = 0;

    LCD_Send4Bits(command >> 4);
    LCD_Send4Bits(command & 0x0F);

    __delay_ms(2);
}


void LCD_Char(unsigned char data)
{
    LCD_RS = 1;

    LCD_Send4Bits(data >> 4);
    LCD_Send4Bits(data & 0x0F);

    __delay_us(100);
}


void LCD_String(const char *text)
{
    while (*text)
    {
        LCD_Char(*text);
        text++;
    }
}


void LCD_Init(void)
{
    TRISB = 0x00;

    LCD_RS = 0;
    LCD_EN = 0;

    __delay_ms(20);

    LCD_Send4Bits(0x03);
    __delay_ms(5);

    LCD_Send4Bits(0x03);
    __delay_us(150);

    LCD_Send4Bits(0x03);
    __delay_us(150);

    LCD_Send4Bits(0x02);

    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x01);
    LCD_Command(0x06);
}


// ============================================================
// ADC INITIALIZATION
// ============================================================
// AN0 / RA0 -> Battery voltage
// AN1 / RA1 -> Load current
// AN3 / RA3 -> Solar charging current
//
// RD0 -> Fault reset button
// ============================================================

void ADC_Init(void)
{
    TRISA0 = 1;
    TRISA1 = 1;
    TRISA3 = 1;

    ADCON1 = 0x80;

    ADCON0 = 0x41;
}


// ============================================================
// ADC READ
// ============================================================

unsigned int ADC_Read(unsigned char channel)
{
    ADCON0 &= 0xC7;
    ADCON0 |= (channel << 3);

    __delay_us(20);

    GO_nDONE = 1;

    while (GO_nDONE);

    return ((unsigned int)ADRESH << 8) | ADRESL;
}


// ============================================================
// BATTERY VOLTAGE CALCULATION
// ============================================================

unsigned int ADC_To_BatteryMillivolts(unsigned int adc_value)
{
    return (unsigned int)
           (((unsigned long)adc_value * 20000UL) / 1023UL);
}


// ============================================================
// ACS712 CURRENT CALCULATION
// ============================================================

int ADC_To_Current_mA(unsigned int adc_value)
{
    unsigned long sensor_millivolts;
    long current_mA;

    sensor_millivolts =
        ((unsigned long)adc_value * 5000UL) / 1023UL;

    current_mA =
        ((long)sensor_millivolts - 2500L) * 1000L / 185L;

    if (current_mA > -CURRENT_ZERO_DEADBAND_MA &&
        current_mA < CURRENT_ZERO_DEADBAND_MA)
    {
        current_mA = 0;
    }

    return (int)current_mA;
}


// ============================================================
// LCD DISPLAY - BATTERY VOLTAGE
// ============================================================

void LCD_DisplayBatteryVoltage(unsigned int millivolts)
{
    unsigned int volts;
    unsigned int decimal;

    volts = millivolts / 1000;
    decimal = (millivolts % 1000) / 10;

    LCD_Command(0x80);

    LCD_String("BATTERY ");

    LCD_Char((volts / 10) + '0');
    LCD_Char((volts % 10) + '0');

    LCD_Char('.');

    LCD_Char((decimal / 10) + '0');
    LCD_Char((decimal % 10) + '0');

    LCD_String(" V ");
}


// ============================================================
// LCD DISPLAY - LOAD CURRENT
// ============================================================

void LCD_DisplayCurrent(int current_mA)
{
    unsigned int absolute_current;
    unsigned int amps;
    unsigned int decimal;

    LCD_Command(0xC0);

    LCD_String("CURRENT ");

    if (current_mA < 0)
    {
        LCD_Char('-');
        absolute_current = (unsigned int)(-current_mA);
    }
    else
    {
        LCD_Char(' ');
        absolute_current = (unsigned int)current_mA;
    }

    amps = absolute_current / 1000;
    decimal = (absolute_current % 1000) / 10;

    LCD_Char((amps / 10) + '0');
    LCD_Char((amps % 10) + '0');

    LCD_Char('.');

    LCD_Char((decimal / 10) + '0');
    LCD_Char((decimal % 10) + '0');

    LCD_String(" A");
}


// ============================================================
// LCD DISPLAY - SOLAR CURRENT
// ============================================================

void LCD_DisplaySolarCurrent(int current_mA,
                             unsigned char charging)
{
    unsigned int absolute_current;
    unsigned int amps;
    unsigned int decimal;

    LCD_Command(0x80);

    LCD_String("SOLAR   ");

    if (current_mA < 0)
    {
        LCD_Char('-');
        absolute_current = (unsigned int)(-current_mA);
    }
    else
    {
        LCD_Char(' ');
        absolute_current = (unsigned int)current_mA;
    }

    amps = absolute_current / 1000;
    decimal = (absolute_current % 1000) / 10;

    LCD_Char((amps / 10) + '0');
    LCD_Char((amps % 10) + '0');

    LCD_Char('.');

    LCD_Char((decimal / 10) + '0');
    LCD_Char((decimal % 10) + '0');

    LCD_String(" A ");

    LCD_Command(0xC0);

    if (charging)
    {
        LCD_String("CHARGING        ");
    }
    else
    {
        LCD_String("CHARGE OFF      ");
    }
}


// ============================================================
// LCD DISPLAY - BATTERY + LOAD
// ============================================================

void LCD_DisplayBatteryLoad(unsigned int battery_mV,
                             int load_mA)
{
    unsigned int battery_int;
    unsigned int battery_decimal;

    unsigned int absolute_current;
    unsigned int current_int;
    unsigned int current_decimal;

    battery_int = battery_mV / 1000;
    battery_decimal = (battery_mV % 1000) / 10;

    if (load_mA < 0)
        absolute_current = (unsigned int)(-load_mA);
    else
        absolute_current = (unsigned int)load_mA;

    current_int = absolute_current / 1000;
    current_decimal = (absolute_current % 1000) / 10;

    LCD_Command(0x80);

    LCD_String("BATTERY ");

    LCD_Char((battery_int / 10) + '0');
    LCD_Char((battery_int % 10) + '0');
    LCD_Char('.');
    LCD_Char((battery_decimal / 10) + '0');
    LCD_Char((battery_decimal % 10) + '0');

    LCD_String(" V ");

    LCD_Command(0xC0);

    LCD_String("LOAD    ");

    LCD_Char((current_int / 10) + '0');
    LCD_Char((current_int % 10) + '0');
    LCD_Char('.');
    LCD_Char((current_decimal / 10) + '0');
    LCD_Char((current_decimal % 10) + '0');

    LCD_String(" A ");
}


// ============================================================
// MAIN PROGRAM
// ============================================================

void main(void)
{
    unsigned int battery_adc;
    unsigned int current_adc;
    unsigned int solar_current_adc;

    unsigned int battery_millivolts;

    int current_mA;
    int solar_current_mA;

    unsigned char protection_state;
    unsigned char retry_count;

    unsigned int display_timer;
    unsigned char charging_enabled;


    // ========================================================
    // INITIALIZATION
    // ========================================================

    protection_state = STATE_NORMAL;
    retry_count = 0;
    display_timer = 0;

    LCD_Init();
    ADC_Init();

    charging_enabled = 1;


    // ========================================================
    // OUTPUT / INPUT CONFIGURATION
    // ========================================================

    TRISBbits.TRISB0 = 0;     // LOW LED
    TRISBbits.TRISB3 = 0;     // NORMAL LED

    TRISCbits.TRISC0 = 0;     // Load relay
    TRISCbits.TRISC1 = 0;     // HIGH LED
    TRISCbits.TRISC2 = 0;     // Buzzer
    TRISCbits.TRISC3 = 0;     // Charging relay

    TRISDbits.TRISD0 = 1;     // Fault reset button


    // Initial states

    RB0 = 0;
    RB3 = 0;

    RC0 = 0;
    RC1 = 0;
    RC2 = 0;

    RC3 = 1;                  // Charging enabled


    // ========================================================
    // MAIN LOOP
    // ========================================================

    while (1)
    {

        // ----------------------------------------------------
        // READ BATTERY VOLTAGE
        // ----------------------------------------------------

        battery_adc = ADC_Read(0);

        battery_millivolts =
            ADC_To_BatteryMillivolts(battery_adc);


        // ----------------------------------------------------
        // SOLAR CHARGING CONTROL
        // ----------------------------------------------------

        if (charging_enabled)
        {
            if (battery_millivolts >= CHARGE_STOP_MV)
            {
                charging_enabled = 0;
                RC3 = 0;
            }
        }
        else
        {
            if (battery_millivolts <= CHARGE_RESUME_MV)
            {
                charging_enabled = 1;
                RC3 = 1;
            }
        }


        // ----------------------------------------------------
        // BATTERY STATUS LEDs
        // ----------------------------------------------------

        if (battery_millivolts < LOW_LIMIT_MV)
        {
            RB0 = 1;
            RB3 = 0;
            RC1 = 0;
        }
        else if (battery_millivolts > HIGH_LIMIT_MV)
        {
            RB0 = 0;
            RB3 = 0;
            RC1 = 1;
        }
        else
        {
            RB0 = 0;
            RB3 = 1;
            RC1 = 0;
        }


        // ----------------------------------------------------
        // READ LOAD ACS712 CURRENT
        // ----------------------------------------------------

        current_adc = ADC_Read(1);

        current_mA =
            ADC_To_Current_mA(current_adc);


        // ----------------------------------------------------
        // READ SOLAR ACS712 CURRENT
        // ----------------------------------------------------

        solar_current_adc = ADC_Read(3);

        solar_current_mA =
            ADC_To_Current_mA(solar_current_adc);


        // ----------------------------------------------------
        // PROTECTION CONTROL
        // ----------------------------------------------------

        if (battery_millivolts < LOW_LIMIT_MV)
        {
            RC0 = 0;
        }
        else
        {
            switch (protection_state)
            {

                // =================================================
                // NORMAL STATE
                // =================================================

                case STATE_NORMAL:

                    if (current_mA >= OVERCURRENT_TRIP_MA)
                    {
                        RC0 = 0;

                        protection_state = STATE_COOLDOWN;

                        LCD_Command(0x01);
                        __delay_ms(2);

                        LCD_Command(0x80);
                        LCD_String("OVERCURRENT!");

                        LCD_Command(0xC0);
                        LCD_String("COOLDOWN...");
                    }
                    else
                    {
                        RC0 = 1;
                    }

                    break;


                // =================================================
                // COOLDOWN STATE
                // =================================================

                case STATE_COOLDOWN:

                    RC0 = 0;

                    __delay_ms(COOLDOWN_MS);

                    protection_state = STATE_RETRY;

                    break;


                // =================================================
                // RETRY STATE
                // =================================================

                case STATE_RETRY:

                    if (retry_count >= MAX_RETRIES)
                    {
                        RC0 = 0;

                        protection_state = STATE_FAULT_LOCK;

                        LCD_Command(0x01);
                        __delay_ms(2);

                        LCD_Command(0x80);
                        LCD_String("FAULT LOCK!");

                        LCD_Command(0xC0);
                        LCD_String("LOAD OFF");
                    }
                    else
                    {
                        retry_count++;

                        RC0 = 1;

                        __delay_ms(500);

                        current_adc = ADC_Read(1);

                        current_mA =
                            ADC_To_Current_mA(current_adc);

                        if (current_mA >= OVERCURRENT_TRIP_MA)
                        {
                            RC0 = 0;

                            protection_state = STATE_COOLDOWN;

                            LCD_Command(0x01);
                            __delay_ms(2);

                            LCD_Command(0x80);
                            LCD_String("RETRY FAILED");

                            LCD_Command(0xC0);
                            LCD_String("COOLDOWN...");
                        }
                        else
                        {
                            retry_count = 0;

                            protection_state = STATE_NORMAL;
                        }
                    }

                    break;


                // =================================================
                // FAULT LOCK STATE
                // =================================================

                case STATE_FAULT_LOCK:

                    RC0 = 0;

                    /*
                     * RD0 = 1 -> button released
                     * RD0 = 0 -> button pressed
                     */

                    if (RD0 == 0)
                    {
                        __delay_ms(50);

                        if (RD0 == 0)
                        {
                            retry_count = 0;
                            protection_state = STATE_NORMAL;

                            LCD_Command(0x01);
                            LCD_String("FAULT RESET");

                            __delay_ms(1000);
                        }
                    }

                    break;


                // =================================================
                // DEFAULT
                // =================================================

                default:

                    RC0 = 0;

                    protection_state = STATE_NORMAL;
                    retry_count = 0;

                    break;
            }
        }


        // ----------------------------------------------------
        // BUZZER CONTROL
        // ----------------------------------------------------

        if (protection_state == STATE_NORMAL)
        {
            RC2 = 0;
        }
        else
        {
            RC2 = !RC2;
        }


        // ----------------------------------------------------
        // LCD DISPLAY
        // ----------------------------------------------------

        if (protection_state == STATE_FAULT_LOCK)
        {
            /*
             * Keep FAULT LOCK message visible.
             */
        }
        else if (protection_state == STATE_COOLDOWN)
        {
            /*
             * Keep overcurrent/cooldown message visible.
             */
        }
        else
        {
            /*
             * NORMAL DISPLAY ROTATION
             *
             * display_timer 0-3:
             * Battery + Load screen
             *
             * display_timer 4-7:
             * Solar + Charging screen
             *
             * Main loop = 500 ms
             * Therefore each screen = 2 seconds
             */

            if (display_timer < 4)
            {
                LCD_DisplayBatteryLoad(
                    battery_millivolts,
                    current_mA
                );
            }
            else
            {
                LCD_DisplaySolarCurrent(
                    solar_current_mA,
                    charging_enabled
                );
            }

            display_timer++;

            if (display_timer >= 8)
            {
                display_timer = 0;
            }
        }


        // ----------------------------------------------------
        // LOOP DELAY
        // ----------------------------------------------------

        __delay_ms(500);
    }
}