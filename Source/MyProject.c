    // LCD module connections
sbit LCD_RS at LATB7_bit;
sbit LCD_EN at LATB6_bit;
sbit LCD_D4 at LATB5_bit;
sbit LCD_D5 at LATB4_bit;
sbit LCD_D6 at LATB3_bit;
sbit LCD_D7 at LATB2_bit;

sbit LCD_RS_Direction at TRISB7_bit;
sbit LCD_EN_Direction at TRISB6_bit;
sbit LCD_D4_Direction at TRISB5_bit;
sbit LCD_D5_Direction at TRISB4_bit;
sbit LCD_D6_Direction at TRISB3_bit;
sbit LCD_D7_Direction at TRISB2_bit;
// End LCD module connections

long cnt_pf = 0;
unsigned int i;
unsigned int avg_pf = 0;
unsigned int final_pf_cnt = 0;
unsigned int sample[100];
unsigned int pf;
float power_factor = 0.00,result = 0.00;
float voltage = 0,Current=0;
long v_long,C_long,P_long,P_long1,P_long2;
long tlong=980,tlong1=980;
short cap_no=0;


#define      Button     RD5_bit
#define      Cap_1      LATD7_bit
#define      Cap_2      LATD6_bit
#define      Cap_3      LATD2_bit
#define      Cap_4      LATD3_bit
#define      ON         1
#define      OFF        0


void Get_PF(void);
void Get_Voltage(void);
void Get_Current(void);
void Measure_Power(void);
void Correct_PF(void);
short flag;
void Interrupt()
{
    if (TMR1IF_bit)
    {
       TMR1IF_bit = 0;
       TMR1H = 0xFF;
       TMR1L = 0xEB;//0.01ms interrupt
       //Enter your code here
       cnt_pf++;
       //if(cnt_pf>5000)TMR1IE_bit = 0;
    }

     while(INT0IF_bit)// first zerocrossing found
     {
         TMR1IE_bit = 1;// enables timer1
         TMR1IF_bit = 0;// clear flag
         cnt_pf = 0;
         INT0IF_bit = 0;// clear flag for INT1

         break;
     }
     while(INT1IF_bit)// second zerocrossing found
     {
          TMR1IE_bit = 0; //disable timer1
          TMR1IF_bit = 0; // clear flag
          INT1IF_bit = 0;//clear flag for INT0
          i++;
          sample[i] = cnt_pf;
          avg_pf+=sample[i];
          if(i>=100)
          {
             i = 0;
             avg_pf/=100;
             final_pf_cnt = avg_pf;
             avg_pf = 0;
             cnt_pf = 0;
          }

          break;
     }
}
char message1[] = "PFI-ATOM";
char message2[] = "Power Factor:0.00   ";
char message3[] = "V:   V   ";
char message4[] = "I: .  A    ";
char message5[]= "P:000W 000VA 000VAR ";

void main()
{
   // port  settings...
   TRISA0_bit = 1;// RA0 as input
   TRISA1_bit = 1;// RA1 as input
   TRISC = 0x00;//all out
   PORTC = 0x00;//clear port C
   TRISD2_bit = 0;//output
   TRISD3_bit = 0;//output
   TRISD6_bit = 0;//output
   TRISD7_bit = 0;//output
   TRISD5_bit = 1;//set as input
   if(!RD5_bit)PORTD = 0x00;


   // Initialize LCD configuration...
   Lcd_Init();
   Lcd_Cmd(_LCD_CLEAR); // Clear display
   Lcd_Cmd(_LCD_CURSOR_OFF); // Cursor off
   Lcd_Out(1,1,message1);
   Lcd_Out(2,1,"TUTORIALS");
   Lcd_Out(3,1,"PFI");
   Lcd_Out(4,1,"2019");
   ADCON1 = 0x0A;
   ADCON2 = 0x00;

   // Timer1 settings
   T1CON = 0x01;
   TMR1IF_bit = 0;
   TMR1H = 0xF8;
   TMR1L = 0x2F;

   INTCON = 0xC0;

   // Int0 settings...
   INTEDG0_bit = 0;//interrupt on falling edge
   INT0IF_bit = 0;//clear flag

   // INT1 settings...
   INTEDG1_bit = 0;//interrupt on falling edge
   INT1IF_bit = 0;//clear flag
   Delay_ms(3000);
   Lcd_Cmd(_LCD_CLEAR); // Clear display
   WDTCON = 0xFF;
   while(1)
   {
       asm CLRWDT;
       Get_Voltage(void);
       Get_Current(void);
       Get_PF(void);
       Measure_Power(void);

   }//while(1)
}//void

void Get_PF(void)
{
        for(i=0;i<100;i++)
        {
           sample[i] = 0;
           i++;
        }
        Delay_ms(100);
        GIE_bit = 1;//Enable Interrupt
        INT0IE_bit = 1;//enable INT1
        INT1IE_bit = 1;//enable INT1
        TMR1IE_bit = 1;
        Delay_ms(2000);
        result = (float)cos((final_pf_cnt*2*3.1416)/1000);
        tlong1 = abs(ceil(result*1000));

        if(tlong>(tlong1+10))tlong-=10;
        else if(tlong<(tlong1-10))tlong+=10;
        else tlong = tlong1;



          //char message2[] = "Power Factor:0.00";
           message2[13] = (tlong/1000)+48;
           message2[14] = '.';
           message2[15] = (tlong/100)%10+48;
           message2[16] = (tlong/10)%10 + 48;
           power_factor = (float)result;

        Lcd_Out(1,1,message2);
        Delay_ms(20);
        GIE_bit = 0;//Disable Interrupt
        INT0IE_bit = 0;//enable INT1
        INT1IE_bit = 0;//enable INT1
        TMR1IE_bit = 0;

        //PFI interface
         if(RD5_bit == 0)
         {
            Correct_PF(void);
            Lcd_Out(4,1," PFC ON .           ");
            Delay_ms(50);
         }
         else
         {
            Cap_1 = OFF;
            Cap_2 = OFF;
            Cap_3 = OFF;
            Cap_4 = OFF;
            Lcd_Out(4,1," PFC OFF.           ");
            Delay_ms(50);
            cap_no = 0;
         }
}
void Get_Voltage(void)
{
      unsigned int temp0=0;
      unsigned int maxpoint0=0;
      unsigned int k=0;

      ADCON0 = 0b00000001;// AN0 selected
      for(k=0;k<1000;k++)
      {
        if(temp0 = ADC_Read(0),temp0>maxpoint0)
        {
          maxpoint0 = temp0;
        }
      }
      maxpoint0 = abs(ceil(maxpoint0));
      voltage = (float)maxpoint0*0.6;// (5/1023)*11.00*0.707*220/15;
      v_long = (int)voltage;

      message3[2] = (v_long/100) +48;
      message3[3] = (v_long/10)%10 +48;
      message3[4] =  v_long%10 +48;
      Lcd_Out(2,1, message3);
      Delay_ms(50);
      maxpoint0 = 0;
}
void Get_Current(void)
{
      //char message4[] = "0.000A";
      unsigned int temp1=0;
      unsigned int maxpoint1=0;
      int kk=0;

      ADCON0 = 0b00000101;// AN1 selected
      for(kk=0;kk<1000;kk++)
      {
          if(temp1 = ADC_Read(1),temp1>maxpoint1)
          {
              maxpoint1 = temp1;
          }
      }
      Current = (float)maxpoint1*0.465;// get the load current
      C_long = (int)Current;

      message4[2] = C_long/100+48;
      message4[3] = '.';
      message4[4] = (C_long/10)%10 +48;
      message4[5] = (C_long/1)%10 +48;
      Lcd_Out(2,10,message4);
      Delay_ms(50);
      maxpoint1 = 0;
}
void Measure_Power(void)
{
     P_long = abs(ceil((int)voltage*Current*result/100));//watt
     P_long1 = (int)v_long*C_long/100; //va
     P_long2 = (int)sqrt(P_long1*P_long1 - P_long*P_long);
     //char message5[]= "P:000W 000VA 000VAR ";

     message5[2] = P_long/100 +48;
     message5[3] = (P_long/10)%10 +48;
     message5[4] = (P_long)%10 +48;

     message5[7] = P_long1/100 + 48;
     message5[8] = (P_long1/10)%10 + 48;
     message5[9] = P_long1%10 + 48;

     message5[13] = P_long2/100 + 48;
     message5[14] = (P_long2/10)%10 + 48;
     message5[15] = P_long2%10 + 48;

     Lcd_Out(3,1,message5);
     Delay_ms(50);
}
void Correct_PF(void)
{

    if(Current>20)
    {
        if(tlong>=980)
        {
            if(cap_no>0)cap_no --;
            else cap_no = 0;
            Delay_ms(1000);
        }
        if(tlong<=960)
        {
           if(cap_no<4)cap_no++;
           else cap_no = 4;
           Delay_ms(1000);
        }


        // Capacitor connection
        if(cap_no==4)
        {
           Cap_1 = ON;
           Cap_2 = ON;
           Cap_3 = ON;
           Cap_4 = ON;
        }
        else if(cap_no==3)
        {
           Cap_1 = ON;
           Cap_2 = ON;
           Cap_3 = ON;
           Cap_4 = OFF;
        }
        else if(cap_no==2)
        {
           Cap_4 = OFF;
           Cap_3 = OFF;
           Cap_2 = ON;
           Cap_1 = ON;
        }
        else if(cap_no==1)
        {
           Cap_4 = OFF;
           Cap_3 = OFF;
           Cap_2 = OFF;
           Cap_1 = ON;
        }
        else
        {
           Cap_1 = OFF;
           Cap_2 = OFF;
           Cap_3 = OFF;
           Cap_4 = OFF;
        }
    }
    else
    {
           Cap_1 = OFF;
           Cap_2 = OFF;
           Cap_3 = OFF;
           Cap_4 = OFF;
    }

}

