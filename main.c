#define F_CPU 8000000UL

#include<avr/io.h>
#include<util/delay.h>
#include<avr/interrupt.h>
#include<stdio.h>

#define MPU9250_ADDRESS 0x68


char serialSendData[256];
volatile int serialSend_i, i2c_i;
float temp1,temp2,ax,ay,az,mx,my,mz,gx,gy,gz,pre,alt;
int MPU_WHOAMI=0;
uint8_t MPU9250data[16];

//Prototype declaration-------------
ISR(USART_TX_vect);
ISR(TWI_vect);
void sendSerial();
void i2c_start();
void i2c_write(int data);
void i2c_writeSet(uint8_t addres, uint8_t registerID, uint8_t writeData);
uint8_t i2c_readSet(uint8_t addres, uint8_t registerID);
void i2c_stop();
void calculateAccel();
void calculateGyro();
void calculateTemp1();
int i2c_interruptWrite();
int i2c_interruptRead();
int i2c_MPU9250Read();
//----------------------------------

int main(void){
	DDRD = 0b11111100;
    PORTD = 0b00000000;
 
    //UART setting
    UCSR0A = 0b00000000;
    UCSR0B = 0b01011000;
    UCSR0C = 0b00000110;
    UBRR0 = 51;//9600bps

    //i2c setting
    TWBR = 2;//400kHz
    TWSR = 0b00000010;
    TWCR = (1<<TWEN)|(1<<TWIE);
    i2c_writeSet(MPU9250_ADDRESS,0x6B,0x00);
    i2c_writeSet(MPU9250_ADDRESS,0x1B,0b00011000);
    i2c_writeSet(MPU9250_ADDRESS,0x1C,0b00011000);
    _delay_ms(10);
    sei();
    while(1){
//        PORTD = 0b00010000;
        _delay_ms(100);
//        PORTD = 0b00000100;
        i2c_start();
       _delay_ms(100);
        MPU_WHOAMI = i2c_readSet(MPU9250_ADDRESS,0x75);
        calculateAccel();
        sprintf(serialSendData,"%f,%f,%f,%x\r\n",ax,ay,az,MPU_WHOAMI);
        sendSerial();
    }
}

//serialSendData内の文字列を１つづつ送信.終端文字で停止
ISR(USART_TX_vect)
{
  if(serialSendData[serialSend_i] != '\0'){
    UDR0 = serialSendData[serialSend_i]; 
    serialSend_i++;
  }else if(serialSendData[serialSend_i] == '\0'){
    serialSend_i=0;
  }
}

ISR(TWI_vect){
    i2c_MPU9250Read();
}

void calculateAccel(){
    uint16_t A1 = (MPU9250data[0] << 8) | MPU9250data[1] ;
    uint16_t A2 = (MPU9250data[2] << 8) | MPU9250data[3] ;
    uint16_t A3 = (MPU9250data[4] << 8) | MPU9250data[5] ;
    ax = ( A1 * 16.0 ) / 32768.0;//[G]に変換
    ay = ( A2 * 16.0 ) / 32768.0;//[G]に変換
    az = ( A3 * 16.0 ) / 32768.0;//[G]に変換
    return;
}

//serialSendData内の文字列の送信開始
void sendSerial(){
    serialSend_i = 0;
    UDR0 = serialSendData[serialSend_i];
    serialSend_i++;
    return;
}

void i2c_start(){
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE);
    return;
}

void i2c_write(int data){
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
    return;
}

void i2c_writeSet(uint8_t addres, uint8_t registerID, uint8_t writeData){

    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while(!(TWCR & 1<<TWINT));
    i2c_write((addres << 1) + 0);
    i2c_write(registerID);
    i2c_write(writeData);
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
    _delay_ms(10);
    return;
}

uint8_t i2c_readSet(uint8_t addres, uint8_t registerID){
    
    uint8_t readData = 0;
    
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while(!(TWCR & 1<<TWINT));
    TWDR = (addres<<1) + 0;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & 1<<TWINT));
    TWDR = registerID;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & 1<<TWINT));

    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while(!(TWCR & 1<<TWINT));
    TWDR = (addres<<1) + 1;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & 1<<TWINT));
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & 1<<TWINT));
    readData = TWDR;
    TWCR = ((1<<TWSTO)|1<<TWINT)|(1<<TWEN);

    return readData;
}

void i2c_stop(){
    TWCR = ((1<<TWSTO)|1<<TWINT)|(1<<TWEN)|(1<<TWIE);
}

int i2c_interruptRead(){

    switch(TWSR & 0xF8){
        case 0x08:
            TWDR = (MPU9250_ADDRESS<<1) + 0;
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x18:
            TWDR = 0x75;
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x28:
            TWCR = (1<<TWSTA)|(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x10:
            TWDR = (MPU9250_ADDRESS<<1) + 1;
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x40:
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x58:
            MPU_WHOAMI = TWDR;
            TWCR = ((1<<TWSTO)|1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        default:
            i2c_stop();
            return 1;
            break;
    }
    return 0;
}

int i2c_MPU9250Read(){

    switch(TWSR & 0xF8){
        case 0x08:
            i2c_i=0;
            TWDR = (MPU9250_ADDRESS<<1) + 0;
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x18:
            TWDR = 0x3B;
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x28:
            TWCR = (1<<TWSTA)|(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x10:
            TWDR = (MPU9250_ADDRESS<<1) + 1;
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        case 0x40:
            TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)|(1<<TWIE);
            break;
        case 0x50:
            if(0 <= i2c_i && i2c_i < 15){
                if(i2c_i == 14){
                    MPU9250data[i2c_i]=TWDR; 
                    i2c_i++;
                    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
                }else{
                    MPU9250data[i2c_i]=TWDR; 
                    i2c_i++;
                    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)|(1<<TWIE);
                }
            }else{
                MPU9250data[i2c_i]=TWDR; 
                i2c_stop();
                return 1;
            }
            break;
        case 0x58:
            MPU9250data[i2c_i]=TWDR; 
            TWCR = ((1<<TWSTO)|1<<TWINT)|(1<<TWEN)|(1<<TWIE);
            break;
        default:
            i2c_stop();
            return 1;
            break;
    }
    return 0;
}
