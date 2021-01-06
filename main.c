#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "make_wav.h"

#define S_RATE  (11025L) //(44100L)

#define BUF_SIZE 352L //1411L

short int buf_pip_ZERO[BUF_SIZE+1] = {0};
short int buf_pip_ONE[BUF_SIZE+1] = {0};
short int buf_pip_PREAMBLE[BUF_SIZE+1] = {0};
short int buf_pip_GAP[BUF_SIZE+1] = {0};
char * iFileName = NULL;
unsigned int startAddr=0xffff;
long int sfile_len = 0;
long int dfile_len = 0;
unsigned char *sfile = NULL;
unsigned char *dfile = NULL;
int bInvert=0, bPreamble=0, b4MHzXtal=0;


//convert two ascii characters to a byte value
unsigned int twochar(unsigned char *dfile){
    int i = 0;
    unsigned int thisByte = 0;
    if(dfile[i] >= 'A') {
        thisByte = (dfile[i] - 55)<<4;
    } else {
        thisByte = (dfile[i] - '0')<<4;
    }
    if(dfile[i+1] >= 'A') {
        thisByte += dfile[i+1] - 55;
    } else {
        thisByte += dfile[i+1] - '0';
    }
    return thisByte;
}

/* write out a 16b int to chars in little endian orientation */
void write_little_endian(unsigned int word, int num_bytes, FILE *wav_file)
{
    unsigned int buf;
    while(num_bytes>0)
    {   buf = word & 0xff;
        fwrite(&buf, 1,1, wav_file);
        num_bytes--;
        word >>= 8;
    }
}

//write of all data bits for SubChunk2data.  Will return length of data.
//if dummyWrite is true, then no data is actually written to the file.
unsigned long write_SubChunk2(int dummyWrite, unsigned long num_samples, unsigned int bytes_per_sample, FILE* wav_file) {
    unsigned long i, byteCnt=0;
    unsigned char thisByte;


    //preamble
    if(bPreamble) {
        for (i=0; i<15; i++)
        {
            for(int nb=0; nb<=7; nb++) {
                if(!dummyWrite) {
                    for(int k=0; k<BUF_SIZE; k++) {
                        write_little_endian((unsigned int)(buf_pip_PREAMBLE[k]),bytes_per_sample, wav_file);
                    }
                }
            }
            byteCnt++;
        }
        if(!bInvert) {
            for (i=0; i<8; i++)
            {
                for(int nb=0; nb<=7; nb++) {
                    if(!dummyWrite) {
                        for(int k=0; k<BUF_SIZE; k++) {
                            write_little_endian((unsigned int)(buf_pip_GAP[k]),bytes_per_sample, wav_file);
                        }
                    }
                }
                byteCnt++;
            }
        }
    }


    for(i = 0; i<=dfile_len; i+=2)
    {
        //ignore cr lf etc
        if(dfile[i] >= '0')
        {
            byteCnt++;
            //extract hex value
            thisByte = twochar(&dfile[i]);

            //create pips for each bit (LSB first)
            for(int nb=0; nb<=7; nb++)
            {
                unsigned int mask = (1<<nb);
                if(thisByte & mask)
                {
                    if(!dummyWrite) {
                        for(int k=0; k<BUF_SIZE; k++) {
                            write_little_endian((unsigned int)(buf_pip_ONE[k]),bytes_per_sample, wav_file);
                        }
                    }
                } else {
                    if(!dummyWrite) {
                        for(int k=0; k<BUF_SIZE; k++) {
                            write_little_endian((unsigned int)(buf_pip_ZERO[k]),bytes_per_sample, wav_file);
                        }
                    }
                }
            }
            //SubChunk2data
        }
    }
    return byteCnt*BUF_SIZE*8;
}


/* information about the WAV file format from
    http://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 */
unsigned long write_wav(char * filename, unsigned long num_samples, int s_rate)
{
    unsigned long retval=0;
    FILE* wav_file;
    unsigned int sample_rate;
    unsigned int num_channels;
    unsigned int bytes_per_sample;
    unsigned int byte_rate;

    num_channels = 1;   /* monoaural */
    bytes_per_sample = 2;

    if (s_rate<=0) sample_rate = 44100;
    else sample_rate = (unsigned int) s_rate;

    byte_rate = sample_rate*num_channels*bytes_per_sample;

    wav_file = fopen(filename, "wb");
    assert(wav_file);   /* make sure it opened */

    /* write RIFF header */
    fwrite("RIFF", 1, 4, wav_file);
    write_little_endian(36 + bytes_per_sample* num_samples*num_channels, 4, wav_file);
    fwrite("WAVE", 1, 4, wav_file);

    /* write fmt  subchunk */
    fwrite("fmt ", 1, 4, wav_file); /*SubChunk1ID*/
    write_little_endian(16, 4, wav_file);   /* SubChunk1Size is 16 */
    write_little_endian(1, 2, wav_file);    /* PCM is format 1 */
    write_little_endian(num_channels, 2, wav_file);
    write_little_endian(sample_rate, 4, wav_file);
    write_little_endian(byte_rate, 4, wav_file);
    write_little_endian(num_channels*bytes_per_sample, 2, wav_file);  /* block align */
    write_little_endian(8*bytes_per_sample, 2, wav_file);  /* bits/sample */


    //dummy write of all data bits to get the data length
    //(this method avoids large buffers or intermediate files)
    unsigned long num_bytes = bytes_per_sample*num_channels*
        write_SubChunk2(1, num_samples, bytes_per_sample, wav_file);

    //now write the SubChunk2size
    fwrite("data", 1, 4, wav_file); /*SubChunk2ID*/
    write_little_endian(num_bytes, 4, wav_file); /*SubChunk2size*/
    //and send data to the file
    write_SubChunk2(0, num_bytes, bytes_per_sample, wav_file);

    fclose(wav_file);

    return retval;
}

//read Intel Hex records and copy out data only
//StartCode   ByteCount   Address   RecordType   Data   Checksum
void parseFile(unsigned char *sfile, unsigned char *dfile, long int sfile_len) {

    long int sidx=0, didx=0;
    unsigned int nybbleCnt=0, byteCnt = 0, thisAddr=0xffff;
    //clear destination array
    memset(dfile, 0, sfile_len);
    startAddr = 0xffff;
    for (sidx = 0; sidx < sfile_len; sidx++){
        //scan for next StartCode
        if(sfile[sidx] == ':') {
            //skip startCode
            sidx+=1;
            //extract byteCount
            byteCnt = twochar(&sfile[sidx]);
            sidx += 2;
            nybbleCnt = byteCnt*2;
            //extract 4-byte address and keep the first one
            thisAddr = 256*twochar(&sfile[sidx]);
            sidx += 2;
            thisAddr += twochar(&sfile[sidx]);
            sidx += 2;
            if(startAddr == 0xffff ) {
                startAddr = thisAddr;
            }
            //skip recordType
            sidx += 2;
            //copy out the data
            memcpy(&dfile[didx], &sfile[sidx], nybbleCnt);
            didx += nybbleCnt;
            sidx += nybbleCnt;
            //ignore checksum, lf, cf
        }
    }
    dfile_len = didx+2;
}

int read_hex_file( char *file_name) {
    //FILE * hexfile;
// opening the file in read mode
    FILE* fp = fopen(file_name, "rb");

    // checking if the file exist or not
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);

    // calculating the size of the file
    sfile_len = ftell(fp);
    fseek(fp, 0L, 0);

    sfile = (unsigned char *)calloc(sfile_len +2 , sizeof(unsigned char));
    dfile = (unsigned char *)calloc(sfile_len +2 , sizeof(unsigned char));

    fread(sfile, sizeof(unsigned char), sfile_len, fp);

    // closing the file
    fclose(fp);

    parseFile(sfile, dfile, sfile_len);

    return 0;
}


void testArgv() {
}

int main(int argc, char ** argv)
{
    int i, iLen;
    //float t;
    float amplitude = 32000;
    float freq_Hz = 1000;
    float phase=0;

    float freq_radians_per_sample = 0;//freq_Hz*2*M_PI/S_RATE;

     //printf("\ncmdline args count=%i\n", argc);

     /* First argument is executable name only */
     //printf("\nexe name=%s", argv[1]);

     for (i=1; i< argc; i++) {
         //printf("\narg%d=%s", i, argv[i]);
         if(argv[i][0] == '-') {
            //set any internal switches
            switch (argv[i][1])
            {
                case 'i':
                case 'I':
                    bInvert = 1;
                    printf("\n* wav data will be inverted.");
                break;
                case 'p':
                case 'P':
                    bPreamble = 1;
                    printf("\n* wav will have a preamble tone.");
                break;
                case '4':
                    b4MHzXtal = 1;
                    printf("\n* pip timings will assume a 4.000MHz Xtal.");
                break;
            }
         }
     }

     printf("\n");



    if(0) {//argc == 0) {
        printf("specify an Intel Hex file\neg MK14WAVwrite DUCK.HEX\n");
    } else {

        //extract file name from DOS command line
        iFileName = argv[1];
        iLen = strcspn(argv[1] , ".");
        printf("input file %s\n",iFileName);
        read_hex_file(iFileName);


    //}
     //while(1);
//#if 0

        /* build the sine wave "pips" */
        if(b4MHzXtal) {
            freq_radians_per_sample = freq_Hz*2*M_PI/S_RATE;
        } else {
            freq_radians_per_sample = freq_Hz*2*M_PI/S_RATE;
        }

        //data stream
        for (i=0; i<BUF_SIZE; i++)
        {
            phase += freq_radians_per_sample;
            buf_pip_PREAMBLE[i] = (int)(amplitude * sin(phase));
            if(bInvert) {
                if(i>BUF_SIZE/8){ //4ms in 32ms
                    buf_pip_ZERO[i] = (int)(amplitude * sin(phase));
                } else {
                    buf_pip_ZERO[i] = (int)(0);
                }
                if(i>BUF_SIZE/2){ //16ms in 32ms
                    buf_pip_ONE[i] = (int)(amplitude * sin(phase));
                } else {
                    buf_pip_ONE[i] = (int)(0);
                }
            } else {
                if(i<BUF_SIZE/8){ //4ms in 32ms
                    buf_pip_ZERO[i] = (int)(amplitude * sin(phase));
                } else {
                    buf_pip_ZERO[i] = (int)(0);
                }
                if(i<BUF_SIZE/2){ //16ms in 32ms
                    buf_pip_ONE[i] = (int)(amplitude * sin(phase));
                } else {
                    buf_pip_ONE[i] = (int)(0);
                }
            }
        }
        // create filename
        //remove extension from the input filename
        iFileName[iLen] = 0;
        //create space for the output filename
        char *oFileStr = (char *)calloc(iLen + 5 + 4 + 1 , sizeof(char));
        //copy in the input filename
        strcpy(&oFileStr[0], iFileName);
        //append the start address
        strcat(oFileStr, "_0");
        itoa(startAddr, &oFileStr[iLen+2], 16);

        //append any options
        if(bInvert) {
            strcat(oFileStr, "_i");
        }
        if(bPreamble) {
            strcat(oFileStr, "_p");
        }
        if(b4MHzXtal) {
            strcat(oFileStr, "_4");
        }

        //append the new extension
        strcat(oFileStr, ".wav");

        printf("writing to file %s\n", oFileStr);
        write_wav(oFileStr, BUF_SIZE, S_RATE);
    }
//#endif //0
    return 0;
}

