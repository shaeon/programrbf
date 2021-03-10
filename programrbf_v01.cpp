// Copyright (c) 2017-2021 - Victor Trucco
//
// All rights reserved
//
// Redistribution and use in source and synthezised forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// Redistributions in synthesized form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// Neither the name of the author nor the names of other contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// You are responsible for any legal issues arising from your use of this code.
//
//
// Modified program for use with RPI GPIO for: Carlos Palmero
// Thanks to Fernando Mosquera (Benitoss) for help
// Date 2021.03.09
// Version 0.01
//
// Notes:
// For compile dont forget include wiringPi lib like:
// g++ programrbf.cpp -lwiringPi
// if you want use this program with cyclone II fpga please uncomment line:
// "//if (bitcount<45) continue;"


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPi.h>


//Atlas-FPGA Pines
//TCK	GPIO17	PIN11	WIRINGPIN0
//TDI	GPIO22	PIN15	WIRINGPIN3
//TMS	GPIO4	PIN7	WIRINGPIN7
//TDO	GPIO27	PIN13	WIRINGPIN2

//I will use pass parameters
//#define PIN_TCK     17
//#define PIN_TDI     22
//#define PIN_TMS     4
//#define PIN_TDO     27
//Global var for pass by argc
int PIN_TCK;
int PIN_TDI;
int PIN_TMS;
int PIN_TDO;

char* FileNameRBF;
#define BUFSIZE 1000


#define MaxIR_ChainLength 100

int IRlen = 0;
int nDevices = 0;

struct codestr  
{    
  unsigned char onebit:1;     
  unsigned int manuf:11; 
  unsigned int size:9;  
  unsigned char family:7; 
  unsigned char rev:4; 
};

union 
{
    unsigned long code = 0;
    codestr b;
} idcode;

void error( ) {
    printf("JTAG ERROR!!!!");
}



void JTAG_clock()  
{
 //   digitalWrite(TCKpin, LOW); 
    digitalWrite(PIN_TCK, HIGH);  
    digitalWrite(PIN_TCK, LOW);
}

void TDI_HIGH()
{
	digitalWrite(PIN_TDI, HIGH);

}

void TDI_LOW()
{
        digitalWrite(PIN_TDI, LOW);

}

void setupJTAG( ) {
    pinMode( PIN_TCK, OUTPUT );
    pullUpDnControl (PIN_TDO, PUD_UP);
    pinMode( PIN_TDO, INPUT );
    pinMode( PIN_TMS, OUTPUT );
    pinMode( PIN_TDI, OUTPUT );

    digitalWrite( PIN_TCK, LOW );
    digitalWrite( PIN_TMS, LOW );
    digitalWrite( PIN_TDI, LOW );
}


void releaseJTAG( ) {
  pullUpDnControl (PIN_TCK, PUD_UP);
    pinMode( PIN_TCK, INPUT);
    pullUpDnControl (PIN_TDO, PUD_UP);
    pinMode( PIN_TDO, INPUT);
    pullUpDnControl (PIN_TMS, PUD_UP);
    pinMode( PIN_TMS, INPUT);
    pullUpDnControl (PIN_TDI, PUD_UP);
    pinMode( PIN_TDI, INPUT);
}


void JTAG_reset()   
{   
    int i;   
    
    digitalWrite(PIN_TMS, HIGH);
          
    // go to reset state   
    for(i=0; i<10; i++) 
    {
        JTAG_clock();
    }
}


void JTAG_EnterSelectDR()   
{ 
    // go to select DR   
    digitalWrite(PIN_TMS, LOW); JTAG_clock();   
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();  
} 
 
void JTAG_EnterShiftIR()   
{   
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
     
} 

void JTAG_EnterShiftDR()   
{   
    digitalWrite(PIN_TMS, LOW); JTAG_clock();
    digitalWrite(PIN_TMS, LOW); JTAG_clock();   

   // digitalWrite(PIN_TMS, LOW); JTAG_clock(); //extra ?
} 

void JTAG_ExitShift()   
{   
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();   
}   

void JTAG_ReadData(int bitlength)   
// note: call this function only when in shift-IR or shift-DR state   
{   
    int bitofs = 0;

    unsigned long temp;
  
     
    bitlength--;   
    while(bitlength--)   
    {   
        digitalWrite(PIN_TCK, HIGH);

        temp = digitalRead(PIN_TDO);


       // Serial.println(temp, HEX); 
        
        temp = temp << bitofs ;
        idcode.code |= temp;
        
        digitalWrite(PIN_TCK, LOW);  
        bitofs++;   

        
    }  

    digitalWrite(PIN_TMS, HIGH);
    digitalWrite(PIN_TCK, HIGH);
 
    temp = digitalRead(PIN_TDO);

   // Serial.println(temp, HEX); 
    
    temp = temp << bitofs ;
    idcode.code |= temp;
        
    digitalWrite(PIN_TCK, LOW); 
     
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();   
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();  // go back to select-DR   
}


void JTAG_ReadDR(int bitlength)   
{   
    JTAG_EnterShiftDR();   
    JTAG_ReadData(bitlength);   
}  

int JTAG_DetermineChainLength(char* s)   
{   
    int i; 
  // Serial.println("JTAG_DetermineChainLength"); 

    
   
    // empty the chain (fill it with 0's)
    digitalWrite(PIN_TDI, LOW);   
    for(i=0; i<MaxIR_ChainLength; i++) { digitalWrite(PIN_TMS, LOW); JTAG_clock();   }
    
    digitalWrite(PIN_TCK, LOW);
         
    // feed the chain with 1's   
    digitalWrite(PIN_TDI, HIGH);
    for(i=0; i<MaxIR_ChainLength; i++) 
    { 
 
  
      digitalWrite(PIN_TCK, HIGH);

       if(digitalRead(PIN_TDO) == HIGH) break;  
        
           digitalWrite(PIN_TCK, LOW);
    }
   
      digitalWrite(PIN_TCK, LOW);

   
       //  Serial.print(s); 
       //  Serial.print(" = ");  
       //  Serial.println(i); 
      
      
    JTAG_ExitShift();   
    return i;   
} 


int JTAG_scan()
{
 int i=0;

  JTAG_reset();  
  JTAG_EnterSelectDR();
  JTAG_EnterShiftIR() ;  
  
  IRlen = JTAG_DetermineChainLength((char*)"tamanho do IR");   

  JTAG_EnterShiftDR();   
  nDevices = JTAG_DetermineChainLength((char*)"Qtd devices");  

  if (IRlen == MaxIR_ChainLength || nDevices == MaxIR_ChainLength )
  {
    error();
    return 1;
  }

  // read the IDCODEs (assume all devices support IDCODE, so read 32 bits per device)   
    JTAG_reset();   
    JTAG_EnterSelectDR();
    JTAG_ReadDR(32*nDevices); 

    for(i=0; i<nDevices; i++)   
    {   
       // assert(idcode[i].onebit);  // if the bit is zero, that means IDCODE is not supported for this device   
       // printf("Device %d IDCODE: %08X (Manuf %03X, Part size %03X, Family code %02X, Rev %X)\n", i+1, idcode[i], idcode[i].manuf, idcode[i].size, idcode[i].family, idcode[i].rev);   

      
          printf("Device IDCODE: %i\n", idcode.code); 
          
          printf(" rev: %i\n",idcode.b.rev);
          
          printf(" family: %i\n",idcode.b.family);
          
          printf(" size: %i\n",idcode.b.size); 
          
          printf(" manuf: %i\n", idcode.b.manuf); 
          
          printf(" onebit: %i\n",idcode.b.onebit); 
       
    }  

    return 0;

}

void JTAG_PREprogram()
{
    int n;
    
    JTAG_reset();  
    JTAG_EnterSelectDR();
    JTAG_EnterShiftIR() ;  

      //  digitalWrite(PIN_TMS, LOW); JTAG_clock(); //extra ?

    // aqui o TMS jÃ¡ esta baixo, nao precisa de outro comando pra abaixar.

    // IR = PROGRAM =   00 0000 0010    // IR = CONFIG_IO = 00 0000 1101
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, HIGH); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();

    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
                        
    digitalWrite(PIN_TDI, LOW);   
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();

    // aqui o modo Ã© exit IR 
    
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    
     // aqui o modo Ã© update IR
        
     // Drive TDI HIGH while moving to SHIFTDR */
    digitalWrite(PIN_TDI, HIGH);  

    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    
    // aqui o modo Ã© select dr scan
    
    digitalWrite(PIN_TMS, LOW); JTAG_clock();
    digitalWrite(PIN_TMS, LOW); JTAG_clock();
    
    // aqui o modo estÃ¡ em shift dr

 //digitalWrite(PIN_TMS, LOW); JTAG_clock(); //extra ?


    /* Issue MAX_JTAG_INIT_CLOCK clocks in SHIFTDR state */
   digitalWrite(PIN_TDI, HIGH);
   for(n=0;n<300;n++)
    {
        JTAG_clock();
    }

     digitalWrite(PIN_TDI, LOW);  
}


void JTAG_POSprogram()
{

    int n;
    
 //aqui esta no exit DR
    
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    
    // aqui esta no update DR
    
    digitalWrite(PIN_TMS, LOW); JTAG_clock();
    
    //Aqui esta no RUN/IDLE
    
    JTAG_EnterSelectDR();   
    JTAG_EnterShiftIR(); 

    // aqui em shift ir


    // IR = CHECK STATUS = 00 0000 0100
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, HIGH); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    
    digitalWrite(PIN_TDI, LOW);   
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    
    
    //aqui esta no exit IR
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    
    //aqui esta no select dr scan



    
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    digitalWrite(PIN_TMS, LOW); JTAG_clock();
    digitalWrite(PIN_TMS, LOW); JTAG_clock();
    
    //   aqui esta no shift IR
    
    
    // IR = START = 00 0000 0011
    digitalWrite(PIN_TDI, HIGH); JTAG_clock();
    digitalWrite(PIN_TDI, HIGH); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    
    digitalWrite(PIN_TDI, LOW); JTAG_clock();
    
    digitalWrite(PIN_TDI, LOW);   
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    
    
    //aqui esta no exit IR
    
    digitalWrite(PIN_TMS, HIGH); JTAG_clock();
    digitalWrite(PIN_TMS, LOW); JTAG_clock();
    
    //aqui esta no IDLE

    //espera 
    for(n=0; n<200; n++) 
    {
        JTAG_clock();
        //GPIOB->regs->ODR |= 1;
        //GPIOB->regs->ODR &= ~(1);
    }

    JTAG_reset();
 
}


void program_FPGA() {
   unsigned long bitcount = 0;
    bool last = false;
    int n = 0; 
    
    printf ("Programming ");
    
    JTAG_PREprogram(); 
    FILE *fp = fopen(FileNameRBF, "rb"); /* "r" = open for reading / b = for binary*/

    if (!fp) {  /* validate file open for reading */
      	fprintf (stderr, "error: file open failed.\n");
	exit(1); 
    }

    //char buff[BUFSIZE]; 
    int mark_pos = 0;
	fseek(fp, 0L, SEEK_END);
    unsigned long total = ftell(fp);
	rewind(fp);

    printf("total %i bytes\n",total);
    int divisor = total / 32; 
    int state = LOW;

    while(bitcount < total)  //155224
    { 
        
        unsigned char val = fgetc(fp);
        int value;
        
        if (bitcount % divisor == 0) 
        {
            printf ("*");
	    fflush( stdout );
      //      state = !state;
      //    digitalWrite(PIN_LED, state);
        }

        bitcount++;
  
      //pula os primeiros 44 caracteres do RBF (header no cyclone II)
      //if (bitcount<45) continue; 
        for (n = 0; n <= 7; n++)
        {
            value = ((val >> n) & 0x01);
            digitalWrite(PIN_TDI, value); JTAG_clock();
        }
    }
    printf(" FINISH \n");

      //writetofile ("Additional 16 bytes of 0xFF ------------------------------------------");
      /* AKL (Version1.7): Dump additional 16 bytes of 0xFF at the end of the RBF file */
    for (n=0;n<127;n++ ){
          digitalWrite(PIN_TDI, HIGH); JTAG_clock();
    // writetofile ("1");
    }
    //   writetofile ("1 - TMS 1");

    digitalWrite(PIN_TDI, HIGH); 
    digitalWrite(PIN_TMS, HIGH);

    JTAG_clock();
    // writetofile ("fim programacao ------------------------------------------");     
    printf("Programmed %i bytes\n", bitcount);
    fclose(fp);  /* close the file */

    JTAG_POSprogram();
}

int main(int argc, char *argv[])
{
    
    // Initialize with gpio numbering if want wiringpi numbering change it
    wiringPiSetupGpio ();
    //wiringPiSetup ();
	
    if( argc != 6 ) {
        printf("Use: %s [rbf_file] TCK TDI TMS TDO \n", argv[0]);
        printf("Example on Atlas-FPGA: \n");
        printf("%s msx_cyc1000.rbf 17 22 4 27\n\n", argv[0]);
    } else {
	    printf( "Programming the file %s \n", argv[1]);
		FileNameRBF = argv[1];
		PIN_TCK = atoi(argv[2]);
		PIN_TDI = atoi(argv[3]);
		PIN_TMS = atoi(argv[4]);
		PIN_TDO = atoi(argv[5]);

		setupJTAG( );
		if ( JTAG_scan( ) == 0 ) {
		   unsigned int startTime = millis();
		   unsigned int duration = 0;
		   program_FPGA();
		   duration = millis() - startTime;
		   printf( "Programming time: %i ms\n", duration);
        }
        releaseJTAG();

        printf( "OK, finished\n" );
    }
    
    return 0;

}
