#include <Arduino.h>

#define DEBUG

#define ICP1_PIN 8
#define POWER_PIN 10

//buffer size
#define MAX_RECEIVES 128
//microseconds for one timer tick (4us for 64 prescaler and F_CPU=16MHz)
#define TIMER_PULSE_MICROSECONDS 4
//weather station channel
#define SELECTED_CHANNEL 3
//pulse intervals
#define START_SEQUENCE_MIN 9000 
#define START_SEQUENCE_MAX 10000
#define PULSE_MIN 400
#define PULSE_MAX 600
#define SPACE_ONE_MIN 4000
#define SPACE_ONE_MAX 5000
#define SPACE_ZERO_MIN 1350
#define SPACE_ZERO_MAX 2500
//UART baud rate
#define UART_SPEED  115200
//number of received pin level changes
volatile int receives = 0;
//buffers for pin levels
volatile uint32_t receive_times[MAX_RECEIVES];
volatile bool levels[MAX_RECEIVES];
//buffer for decoded data (theoretical maximum is half of pin changes)
bool decodedData[MAX_RECEIVES/2];
//current watched edge flag 
bool risingEdge = false;
//time to try decoding flag
volatile bool stop_receive = false;
//result of decoded data
float outdoorTemp = 0.0;


void setup() {
   //optional pin for power RF receives
   pinMode(POWER_PIN, OUTPUT);
   digitalWrite(POWER_PIN,HIGH);
   //input capture T1 pin
   pinMode(ICP1_PIN, INPUT);
   //T1 initialization
   initTimer1();

   Serial.begin(UART_SPEED);
   
#ifdef DEBUG   
   Serial.println("Ready");
#endif   
}

void loop() {
   //decode request
   if(stop_receive)
   {      
      decodeData();     
      stop_receive = false;      
   }   
}

ISR(TIMER1_COMPA_vect) {
   //no pin change captured long time -> buffer not empty -> try decode received data
   if(receives>0)
      stop_receive = true;
}

ISR(TIMER1_CAPT_vect) {
   //reset time
   TCNT1H = 0;
   TCNT1L = 0;  
   //read timestamp
   byte low = ICR1L;   
   byte high = ICR1H;
   //calc gap between level changes (microseconds)
   uint32_t gap = TIMER_PULSE_MICROSECONDS * (((uint32_t)high << 8) | (uint32_t)low);
   //write current level change 
   intRecv(gap);

   //change edge detection for next capture
   if (risingEdge)  
      TCCR1B &= ~(1<<ICES1);
   else 
      TCCR1B |= (1<<ICES1);  

   risingEdge = !risingEdge;  
}

//----------------------------------------- T1 initilalization ---------------------------------------------
void initTimer1() { 
   TCCR1A = 0;
   TCCR1B = 3; //64 prescaler
   TCCR1C = 0;

   //OC = 30ms
   OCR1AH = 0x1D;
   OCR1AL = 0x4C;

   TIMSK1 = (1<<ICIE1) | (1<<OCIE1A);
}

//----------------------------------------- start measuring ---------------------------------------------
void startMeasure()
{
   digitalWrite(POWER_PIN,HIGH);
   TCCR1B = 3;   
}

//----------------------------------------- stop measuring ---------------------------------------------
void stopMeasure()
{
   digitalWrite(POWER_PIN,LOW);
   TCCR1B = 0; 
}

//----------------------------------------- write current level change  ---------------------------------------------
void intRecv(uint32_t gap)
{
   if(stop_receive)
   return;
   
   if(receives < MAX_RECEIVES)
   {
      if(receives == 1)
      {
         if(gap > START_SEQUENCE_MIN and gap < START_SEQUENCE_MAX)
         {
            levels[1] = risingEdge;
            receive_times[1] = gap;
            
            receives++; 
         }
         else
         {
            levels[0] = risingEdge;
            receive_times[0] = 0;          
         }
      }
      else if(gap > PULSE_MIN)
      {
         levels[receives] = risingEdge;
         receive_times[receives] = gap;

         receives++;      
      }
      else if(receives > 1)
      {
         stop_receive = true;
      }
      else
      {
         levels[0] = risingEdge;
         receive_times[0] = 0;          
         receives = 0;
      }
   }
   else
      stop_receive = true;
}

//----------------------------------------- decode data stored in receive buffers ---------------------------------------------
void decodeData()
{
   noInterrupts();    

#ifdef DEBUG   
   uint32_t duration = 0;
   
   for(int i=0;i<receives;i++)
   {
      duration += receive_times[i];
   }   

   Serial.print(F("Changed "));Serial.print(receives,DEC);
   Serial.print(F(" times. Duration "));Serial.print(duration, DEC);
   Serial.println(" us");
   
   for(int i=0;i<receives;i++)
   {
      if(i>0)
      {
         Serial.print(receive_times[i], DEC);
         Serial.print("");
      }
      
      if(levels[i])
         Serial.print("/");
      else
         Serial.print("\\");  
   }

   Serial.println("");
   
#endif   

   int len = 0;
   bool decodingOk = false;   

   for(int i=0;i<receives;i++)
   {
      if(i>0)
      {
         unsigned long gap = receive_times[i];

         //start sequence
         if((levels[i-1] == 0) && (gap > START_SEQUENCE_MIN) && (gap < START_SEQUENCE_MAX))
         {
            if(len > 0)
            {
               analyzeDecodedData(len);
            }
            
#ifdef DEBUG          
            Serial.println(F("New data:"));   
#endif
            len = 0;
            decodingOk = true;
         }
         //pulse active
         else if(decodingOk && (levels[i-1] == 1))
         {
            if((gap > PULSE_MIN) && (gap < PULSE_MAX))
            {            
               
            }
            else
            {
               decodingOk = false;
#ifdef DEBUG               
               Serial.println(""); Serial.print(F("Wrong pulse length:")); Serial.println(gap, DEC);  
#endif              
            }
         }
         //space
         else if(decodingOk && (levels[i-1] == 0))
         {
            //one
            if((gap > SPACE_ONE_MIN) && (gap < SPACE_ONE_MAX))
            {
               decodedData[len++] = true;
            }
            //zero
            else if((gap > SPACE_ZERO_MIN) && (gap < SPACE_ZERO_MAX))
            {
               decodedData[len++] = false;
            }
            else
            {
               decodingOk = false;
#ifdef DEBUG                   
               Serial.println(""); Serial.print(F("Wrong space length:")); Serial.println(gap, DEC);              
#endif              
            }
         }  
      }
   }

   if(len > 0)
   {
      analyzeDecodedData(len);
   }
   
   receives = 0;
   
   interrupts();
}

//------------------------- analyze decoded data and make decision that received packet is that what is expected -------------------
void analyzeDecodedData(int len)
{
#ifdef DEBUG        
   for(int i=0;i<len;i++)
   {
      Serial.print(decodedData[i] ? "1|" : "0|");
   }
   Serial.println("");
   Serial.print(F("Decoded "));  Serial.print(len);  Serial.println(F("bytes"));  
#endif
   //temp packet?
   if(len == 28)
   {        
      uint32_t raw = 0;

      //put all bits into one double word
      for(int i=0;i<28;i++)
      {
         raw |= ((uint32_t)decodedData[i]) << 27-i;
      }     

      //calculate and compare checksum
      if(checksum(raw))
      {
         #ifdef DEBUG     
         Serial.print(F("Temp received. Raw=0x")); Serial.println(raw,HEX);
         #endif    
         //retrieve channel and temperature data
         int channel = (raw >> 2) & 3;
         int temp = (raw >> 4) & 0xFFF;

#ifdef DEBUG         
         Serial.print(F("Channel="));Serial.print(channel);  Serial.print(F(";Temp="));  Serial.print(temp); Serial.println(F("C"));
#endif  
         //
         if(channel == SELECTED_CHANNEL)
         {
            setOutdoorTemp(temp);       
         }
      }
      else
      Serial.println(F("Checksum error"));  
   }
}

//----------------------------------------- checksum ---------------------------------------------
bool checksum(uint32_t data)
{
   byte packetChecksum = (data >> 24) & 0xF;
   byte calcChecksum = 0xF;
   
   for(int i=0;i<6;i++)
   {
      calcChecksum += (data >> (20 - 4*i)) & 0xF;
   }   

   calcChecksum &= 0xF;

   return calcChecksum == packetChecksum; 
}

//----------------------------------------- outdoor temperature set ---------------------------------------------
void setOutdoorTemp(int temp)
{
   outdoorTemp = (float)temp / 10;
   
   Serial.print("OutdoorTemp=");Serial.print(outdoorTemp,DEC);Serial.println("C");
}

