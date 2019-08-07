# Hyundai WS1815 temperature sensor receiver

Sensor communicates with station at radio frequency 433MHz. Data are tramsmited using a pulse modulation where length of space specifies meaning of tranferred bit. Same modulation is often used for TV remotes. 

To obtain data some amplitude (ASK) demodulator is needed. I used ET-RXB-12 433MHz heterodyne receiver. This receiver receives a lot of noise from the environment so filter out this noise (search for expected packet) is necessary.

## Modulation
Used pulse modulation gives three types of information: start bit, logic "1" and logic "0". Each of this bit start with pulse (duration around 500 microseconds) followed by space. Duration of this space determines bit type. Bits flow are draw below. The values are average from my measure.

                  _________         9500us        __
    start bit   _|  500us  |_____________________|
                  _________      4500us     __
    logic "1"   _|  500us  |_______________|
                  _________     1500us    __
    logic "0"   _|  500us  |_____________|   

## Protocol
Each packet consist of one start bit and 28 data bits. Data bits in packet are represented as follows:

    data bits     | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|12|13|14|15|16|17|18|19|20|21|22|23| 24| 25|26|27|
    description   | checksum  | ??? - maybe sensor ID | temperature (int = 10*real value) |channel| ?| ?|
    example-14.6° | 0| 0| 1| 1| 0| 0| 1| 1| 1| 0| 0| 0| 0| 0| 0| 0| 1| 0| 0| 1| 0| 0| 1| 0|  1|  1| 1| 0|
    example-14.3° | 1| 1| 1| 1| 0| 0| 1| 1| 1| 0| 0| 0| 0| 0| 0| 0| 1| 0| 0| 0| 1| 1| 1| 1|  1|  1| 1| 0|    

Temperature is represent as 12 bit integer value. This value need to be divide by 10 to get real temperature value.
Channel number is represent by 2 bit value (channel 0-3).
Checksum value is calculated as sum of rest fours of bits plus binary 1111. Only last four bits of sum is needed.


# Hyundai WS1070 temperature sensor receiver

Same as previous. Carrier frequency is 433MHz. OOK (OnOff Keying) modulation with 500us bit lenght is used. Several data packets are sent without any gap between them.

## Coding
Data is encoded using a variable pulse width of "0". No start bit is presented this time.
                  _________       2000us      __
    logic "1"   _|  500us  |_________________|
                  _________     1000us    __
    logic "0"   _|  500us  |_____________|   

## Protocol
Each packet consist 38 data bits. Data bits in packet are represented as follows:

    data bits     | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9| 10| 11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|
    description   |    ??? - maybe sensor ID    |channel| temperature (int = 10*real value) |    ???    |       checksum ?      | ?| ?|
    example-24.6° | 0| 0| 0| 1| 0| 1| 1| 1| 1| 0|  1|  0| 0| 0| 0| 0| 1| 1| 1| 1| 0| 1| 1| 0| 1| 1| 1| 1| 0| 1| 0| 0| 0| 1| 0| 0| 0| 1|
    example-24.9° | 0| 0| 0| 1| 0| 1| 1| 1| 1| 0|  1|  0| 0| 0| 0| 0| 1| 1| 1| 1| 1| 0| 0| 1| 1| 1| 1| 1| 0| 0| 1| 0| 1| 0| 0| 1| 0| 1|    
Temperature is represent as 12 bit integer value. This value need to be divide by 10 to get real temperature value.
Channel number is represent by 2 bit value (channel 0-3).
