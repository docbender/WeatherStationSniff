# Weather station sensor receiver

Project allows read data from weather station Hyundai WS1815 temperature sensor. 

Sensor communicates with station at radio frequency 433MHz. Data are tramsmited using a pulse modulation where length of space specifies meaning of tranferred bit. Same modulation is often used for TV remotes. 

To obtain data some amplitude (ASK) demodulator is needed. I used ET-RXB-12 433MHz heterodyne receiver. This receiver receives a lot of noise from the environment so filter out this noise (search for expected packet) is necessary.

##Modulation
Used pulse modulation gives three types of information: start bit, logic "1" and logic "0". Each of this bit start with pulse (duration around 500 microseconds) followed by space. Duration of this space determines bit type. Bits flow are draw below. The values are average from my measure.

                  _________         9500us        __
    start bit   _|  500us  |_____________________|
                  _________      4500us     __
    logic "1"   _|  500us  |_______________|
                  _________     1500us    __
    logic "0"   _|  500us  |_____________|   

##Protocol
Each packet consist of one start bit and 28 data bits. Data bits in packet are represented as follows:

    data bits    | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24| 25| 26|27|28|
    description  | checksum  | ??? - maybe sensor ID |   temperature (int = 10*real value)  |channel| ?| ?|
    example      |
