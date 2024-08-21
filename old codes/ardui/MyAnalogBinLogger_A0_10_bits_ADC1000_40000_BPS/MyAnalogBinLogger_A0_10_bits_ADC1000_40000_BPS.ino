/**
 * This program logs data from the Arduino ADC to a binary file.
 *
 * Samples are logger at regular intervals. Each Sample consists of the ADC
 * values for the analog pins defined in the PIN_LIST array.  The pins numbers
 * can be in any order.
 * 
 * Edit the configuration constants below to set the sample pins, sample rate,
 * and other configuration values.
 *
 * If your SD card has a long write latency, it may be necessary to use
 * slower sample rates.  Using a Mega Arduino helps overcome latency
 * problems since 13 512 byte buffers will be used.
 *
 * Each 512 byte data block in the file has a four byte header followed by up
 * to 508 bytes of data. (508 values in 8-bit mode or 254 values in 10-bit mode)
 * Each block contains an integral number of samples with unused space at the
 * end of the block.
 *
 * Data is written to the file using SD multiple block write commands.
 */
#include <SdFat.h>
#include <SdFatUtil.h>
#include <BufferedWriter.h>
#include "AnalogBinLogger.h"
//------------------------------------------------------------------------------
// Analog pin number list for a sample.  Pins may be in any order and pin
// numbers can be repeated.
const uint8_t PIN_LIST[] = {0};
//------------------------------------------------------------------------------
// Sample rate in samples per second.
const float SAMPLE_RATE = 40000;  // Must be 0.25 or greater.

// The interval between samples in seconds, SAMPLE_INTERVAL, can be set to a
// constant instead of being calculated from SAMPLE_RATE.  SAMPLE_RATE is not
// used in the code below.  For example, setting SAMPLE_INTERVAL = 2.0e-4
// will result in a 200 microsecond sample interval.
const float SAMPLE_INTERVAL = 1/SAMPLE_RATE;

// Setting ROUND_SAMPLE_INTERVAL non-zero will cause the sample interval to
// be rounded to a a multiple of the ADC clock period and will reduce sample
// time jitter.
#define ROUND_SAMPLE_INTERVAL 1
//------------------------------------------------------------------------------
// ADC clock rate.
// The ADC clock rate is normally calculated from the pin count and sample
// interval.  The calculation attempts to use the lowest possible ADC clock
// rate.
//
// You can select an ADC clock rate by defining the symbol ADC_PRESCALER to
// one of these values.  You must choose an appropriate ADC clock rate for
// your sample interval. 
// #define ADC_PRESCALER 7 // F_CPU/128 125 kHz on an Uno
// #define ADC_PRESCALER 6 // F_CPU/64  250 kHz on an Uno
// #define ADC_PRESCALER 5 // F_CPU/32  500 kHz on an Uno
 #define ADC_PRESCALER 4 // F_CPU/16 1000 kHz on an Uno
// #define ADC_PRESCALER 3 // F_CPU/8  2000 kHz on an Uno (8-bit mode only)
//------------------------------------------------------------------------------
// Reference voltage.  See the processor data-sheet for reference details.
// uint8_t const ADC_REF = 0; // External Reference AREF pin.
uint8_t const ADC_REF = (1 << REFS0);  // Vcc Reference.
// uint8_t const ADC_REF = (1 << REFS1);  // Internal 1.1 (only 644 1284P Mega)
// uint8_t const ADC_REF = (1 << REFS1) | (1 << REFS0);  // Internal 1.1 or 2.56
//------------------------------------------------------------------------------
// File definitions.
//
// Maximum file size in blocks.
// The program creates a contiguous file with FILE_BLOCK_COUNT 512 byte blocks.
// This file is flash erased using special SD commands.  The file will be
// truncated if logging is stopped early.
const uint32_t FILE_BLOCK_COUNT = 256000;

// log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "ANALOG"

// Set RECORD_EIGHT_BITS non-zero to only record the high 8-bits of the ADC.
// For 10 bits leave RECORD_EIGHT_BITS= 0
#define RECORD_EIGHT_BITS 0
//------------------------------------------------------------------------------
// Pin definitions.
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 3;

// SD chip select pin.
const uint8_t SD_CS_PIN = 10;
//------------------------------------------------------------------------------
// Buffer definitions.
//
// logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional buffers
// QUEUE_DIM must be a power of two larger than (BUFFER_BLOCK_COUNT + 1)
//
#if RAMEND < 0X8FF
#error Too little SRAM
//
#elif RAMEND < 0X10FF
// Use total of two 512 byte buffers
const uint8_t BUFFER_BLOCK_COUNT = 1;
// Dimension for queues of 512 byte SD blocks
const uint8_t QUEUE_DIM = 4;  // Must be a power of two!
//
#elif RAMEND < 0X20FF
// Use total of five 512 byte buffers
const uint8_t BUFFER_BLOCK_COUNT = 4;
// Dimension for queues of 512 byte SD blocks
const uint8_t QUEUE_DIM = 8;  // Must be a power of two!
//
#elif RAMEND < 0X40FF
// Use total of 13 512 byte buffers
const uint8_t BUFFER_BLOCK_COUNT = 12;
// Dimension for queues of 512 byte SD blocks
const uint8_t QUEUE_DIM = 16;  // Must be a power of two!
//
#else  // RAMEND
// Use total of 29 512 byte buffers
const uint8_t BUFFER_BLOCK_COUNT = 28;
// Dimension for queues of 512 byte SD blocks
const uint8_t QUEUE_DIM = 32;  // Must be a power of two!
#endif  // RAMEND
//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if reset or power fails.
#define TMP_FILE_NAME "TMP_LOG.BIN"

// Size of file base name.  Must not be larger than six.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

// Number of analog pins to log.
const uint8_t PIN_COUNT = sizeof(PIN_LIST)/sizeof(PIN_LIST[0]);

// Minimum ADC clock cycles per sample interval
const uint16_t MIN_ADC_CYCLES = 15;

// Extra cpu cycles to setup ADC with more than one pin per sample.
const uint16_t ISR_SETUP_ADC = 100;

// Maximum cycles for timer0 system interrupt, millis, micros.
const uint16_t ISR_TIMER0 = 160;
//==============================================================================
SdFat sd;

SdBaseFile binFile;

char binName[13] = FILE_BASE_NAME "00.BIN";

#if RECORD_EIGHT_BITS
const size_t SAMPLES_PER_BLOCK = DATA_DIM8/PIN_COUNT;
typedef block8_t block_t;
#else  // RECORD_EIGHT_BITS
const size_t SAMPLES_PER_BLOCK = DATA_DIM16/PIN_COUNT;
typedef block16_t block_t;
#endif // RECORD_EIGHT_BITS

block_t* emptyQueue[QUEUE_DIM];
uint8_t emptyHead;
uint8_t emptyTail;

block_t* fullQueue[QUEUE_DIM];
volatile uint8_t fullHead;  // volatile insures non-interrupt code sees changes.
uint8_t fullTail;

// queueNext assumes QUEUE_DIM is a power of two
inline uint8_t queueNext(uint8_t ht) {return (ht + 1) & (QUEUE_DIM -1);}
//==============================================================================
// Interrupt Service Routines

//pointer to current buffer
block_t* isrBuf;

// Need new buffer
bool isrBufNeeded = true;

// overrun count
uint16_t isrOver = 0;
// ADC values for each pin
uint8_t adcmux[PIN_COUNT];
uint8_t adcsra[PIN_COUNT];
uint8_t adcsrb[PIN_COUNT];
uint8_t adcindex = 1;

// Insure no timer events are missed
volatile bool timerError = false;
volatile bool timerFlag = false;
//------------------------------------------------------------------------------
// ADC done interrupt
ISR(ADC_vect) {
  // read ADC
#if RECORD_EIGHT_BITS
  uint8_t d = ADCH;
#else  // RECORD_EIGHT_BITS
  // this will access ADCL first. 
  uint16_t d = ADC;
#endif  // RECORD_EIGHT_BITS

  if (isrBufNeeded && emptyHead == emptyTail) {
       // no buffers - count overrun
      if (isrOver < 0XFFFF) isrOver++;
      // Avoid missed timer error.
      timerFlag = false;
      return;
  }
  // Start ADC
  if (PIN_COUNT > 1) {
    ADMUX = adcmux[adcindex];
    ADCSRB = adcsrb[adcindex];
    ADCSRA = adcsra[adcindex];
    if (adcindex == 0) timerFlag = false;
    adcindex =  adcindex < (PIN_COUNT - 1) ? adcindex + 1 : 0;
  } else {
    timerFlag = false;
  }
  // check for buffer needed
  if (isrBufNeeded) {
    isrBufNeeded = false;
    
    // remove buffer from empty queue
    isrBuf = emptyQueue[emptyTail];
    emptyTail = queueNext(emptyTail);
    isrBuf->count = 0;
    isrBuf->overrun = isrOver;
  }
  // store ADC data
  isrBuf->data[isrBuf->count++] = d;

  // check for buffer full
  if (isrBuf->count >= PIN_COUNT*SAMPLES_PER_BLOCK) {
    // Put buffer isrIn full queue.  
    uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.
    fullQueue[tmp] = (block_t*)isrBuf;
    fullHead = queueNext(tmp);
   
    // set buffer needed and clear overruns
    isrBufNeeded = true;
    isrOver = 0;
  }
}
//------------------------------------------------------------------------------
// timer1 interrupt to clear OCF1B
ISR(TIMER1_COMPB_vect) {
  if (timerFlag) timerError = true;
  timerFlag = true;
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) error_P(PSTR(msg))
//------------------------------------------------------------------------------
void error_P(const char* msg) {
  sd.errorPrint_P(msg);
  fatalBlink();
}
//------------------------------------------------------------------------------
//
void fatalBlink() {
  while (true) {
    if (ERROR_LED_PIN >= 0) {
      digitalWrite(ERROR_LED_PIN, HIGH);
      delay(200);
      digitalWrite(ERROR_LED_PIN, LOW);
      delay(200);
    }
  }
}
//==============================================================================
#if ADPS0 != 0 || ADPS1 != 1 || ADPS2 != 2
#error unexpected ADC prescaler bits
#endif
//------------------------------------------------------------------------------
// initialize ADC and timer1
void adcInit(metadata_t* meta) {
  uint8_t adps;  // prescaler bits for ADCSRA 
  uint32_t ticks = F_CPU*SAMPLE_INTERVAL + 0.5;  // Sample interval cpu cycles.

  if (ADC_REF & ~((1 << REFS0) | (1 << REFS1))) {
    error("Invalid ADC reference");
  }
#ifdef ADC_PRESCALER
  if (ADC_PRESCALER > 7 || ADC_PRESCALER < 2) {
    error("Invalid ADC prescaler");
  }
  adps = ADC_PRESCALER;
#else  // ADC_PRESCALER
  // Allow extra cpu cycles to change ADC settings if more than one pin.
  int32_t adcCycles = (ticks - ISR_TIMER0)/PIN_COUNT
                      - (PIN_COUNT > 1 ? ISR_SETUP_ADC : 0);
                      

  for (adps = 7; adps > 0; adps--) {
     if (adcCycles >= (MIN_ADC_CYCLES << adps)) break;
  }
#endif  // ADC_PRESCALER
  meta->adcFrequency = F_CPU >> adps;
  if (meta->adcFrequency > (RECORD_EIGHT_BITS ? 2000000 : 1000000)) {
    error("Sample Rate Too High");
  }
#if ROUND_SAMPLE_INTERVAL
  // Round so interval is multiple of ADC clock
  ticks += 1 << (adps - 1);
  ticks >>= adps;
  ticks <<= adps;
#endif  // ROUND_SAMPLE_INTERVAL

  if (PIN_COUNT > sizeof(meta->pinNumber)/sizeof(meta->pinNumber[0])) {
    error("Too many pins");
  }
  meta->pinCount = PIN_COUNT;
  meta->recordEightBits = RECORD_EIGHT_BITS;
  
  for (int i = 0; i < PIN_COUNT; i++) {
    uint8_t pin = PIN_LIST[i];
    if (pin >= NUM_ANALOG_INPUTS) error("Invalid Analog pin number");
    meta->pinNumber[i] = pin;
   // Set ADC reference and low three bits of analog pin number.   
    adcmux[i] = (pin & 7) | ADC_REF;
    if (RECORD_EIGHT_BITS) adcmux[i] |= 1 << ADLAR;
    // If this is the first pin, trigger on timer/counter 1 compare match B.
    adcsrb[i] = i == 0 ? (1 << ADTS2) | (1 << ADTS0) : 0;
#ifdef MUX5
    if (pin > 7) adcsrb[i] |= (1 << MUX5);
#endif  // MUX5
    adcsra[i] = (1 << ADEN) | (1 << ADIE) | adps;
    adcsra[i] |= i == 0 ? 1 << ADATE : 1 << ADSC;
  }

  // Setup timer1
  TCCR1A = 0;
  uint8_t tshift;
  if (ticks < 0X10000) {
    // no prescale, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    tshift = 0;
  } else if (ticks < 0X10000*8) {
    // prescale 8, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    tshift = 3;
  } else if (ticks < 0X10000*64) {
    // prescale 64, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
    tshift = 6;
  } else if (ticks < 0X10000*256) {
    // prescale 256, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12);
    tshift = 8;
  } else if (ticks < 0X10000*1024) {
    // prescale 1024, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) | (1 << CS10);
    tshift = 10;
  } else {
    error("Sample Rate Too Slow");
  }
  // divide by prescaler
  ticks >>= tshift;
  // set TOP for timer reset
  ICR1 = ticks - 1;
  // compare for ADC start
  OCR1B = 0;
  
  // multiply by prescaler
  ticks <<= tshift;
  
  // Sample interval in CPU clock ticks.
  meta->sampleInterval = ticks;
  meta->cpuFrequency = F_CPU;
  float sampleRate = (float)meta->cpuFrequency/meta->sampleInterval;
  Serial.print(F("Sample pins:"));
  for (int i = 0; i < meta->pinCount; i++) {
    Serial.print(' ');
    Serial.print(meta->pinNumber[i], DEC);
  }
  Serial.println(); 
  Serial.print(F("ADC bits: "));
  Serial.println(meta->recordEightBits ? 8 : 10);
  Serial.print(F("ADC clock kHz: "));
  Serial.println(meta->adcFrequency/1000);
  Serial.print(F("Sample Rate: "));
  Serial.println(sampleRate);  
  Serial.print(F("Sample interval usec: "));
  Serial.println(1000000.0/sampleRate, 4); 
}
//------------------------------------------------------------------------------
// enable ADC and timer1 interrupts
void adcStart() {
  // initialize ISR
  isrBufNeeded = true;
  isrOver = 0;
  adcindex = 1;

  // Clear any pending interrupt.
  ADCSRA |= 1 << ADIF;
  ADMUX = adcmux[0];
  ADCSRB = adcsrb[0];
  ADCSRA = adcsra[0];

  // enable timer1 interrupts
  timerError = false;
  timerFlag = false;
  TCNT1 = 0;
  TIFR1 = 1 << OCF1B;
  TIMSK1 = 1 << OCIE1B;
}
//------------------------------------------------------------------------------
void adcStop() {
  TIMSK1 = 0;
  ADCSRA = 0;
}
//------------------------------------------------------------------------------
// convert binary file to CSV file
void binaryToCsv() {
  uint8_t lastPct = 0;
  block_t buf;
  metadata_t* pm;
  uint32_t t0 = millis();
  uint32_t syncCluster = 0;
  SdFile csvFile;
  char csvName[13];
  BufferedWriter bw;
  
  if (!binFile.isOpen()) {
    Serial.println(F("No current binary file"));
    return;
  }
  binFile.rewind();
  if (!binFile.read(&buf , 512) == 512) error("Read metadata failed");
  // create a new csvFile
  strcpy(csvName, binName);
  strcpy_P(&csvName[BASE_NAME_SIZE + 3], PSTR("CSV"));

  if (!csvFile.open(csvName, O_WRITE | O_CREAT | O_TRUNC)) {
    error("open csvFile failed");  
  }
  Serial.println();
  Serial.print(F("Writing: "));
  Serial.print(csvName);
  Serial.println(F(" - type any character to stop"));
  pm = (metadata_t*)&buf;
  bw.init(&csvFile);
  bw.putStr_P(PSTR("Interval,"));
  float intervalMicros = 1.0e6*pm->sampleInterval/(float)pm->cpuFrequency;
  bw.putNum(intervalMicros, 4);
  bw.putStr_P(PSTR(",usec"));
  bw.putCRLF();
  for (uint8_t i = 0; i < pm->pinCount; i++) {
    if (i) bw.putChar(',');
    bw.putStr_P(PSTR("pin"));
    bw.putNum(pm->pinNumber[i]);
  }
  bw.putCRLF();
  uint32_t tPct = millis();
  while (!Serial.available() && binFile.read(&buf, 512) == 512) {
    uint16_t i;
    if (buf.count == 0) break;
    if (buf.overrun) {
      bw.putStr("OVERRUN,");
      bw.putNum(buf.overrun);
      bw.putCRLF();
    }
    for (i = 0; i < buf.count; i++) {
      bw.putNum(buf.data[i]);
      if((i + 1)%PIN_COUNT) {
        bw.putChar(',');
      } else {
        bw.putCRLF();
      }
    }
    if (csvFile.curCluster() != syncCluster) {
      bw.writeBuf();
      csvFile.sync();
      syncCluster = csvFile.curCluster();
    }
    if ((millis() - tPct) > 1000) {
      uint8_t pct = binFile.curPosition()/(binFile.fileSize()/100);
      if (pct != lastPct) {
        tPct = millis();
        lastPct = pct;
        Serial.print(pct, DEC);
        Serial.println('%');
      }
    }
    if (Serial.available()) break;
  }
  bw.writeBuf();
  csvFile.close();
  Serial.print(F("Done: "));
  Serial.print(0.001*(millis() - t0));
  Serial.println(F(" Seconds"));
}
//------------------------------------------------------------------------------
// read data file and check for overruns
void checkOverrun() {
  bool headerPrinted = false;
  block_t buf;
  uint32_t bgnBlock, endBlock;
  uint32_t bn = 0;
  
  if (!binFile.isOpen()) {
    Serial.println(F("No current binary file"));
    return;
  }
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
    error("contiguousRange failed");
  }
  binFile.rewind();
  Serial.println();
  Serial.println(F("Checking overrun errors - type any character to stop"));
  if (!binFile.read(&buf , 512) == 512) {
    error("Read metadata failed");
  }
  bn++;
  while (binFile.read(&buf, 512) == 512) {
    if (buf.count == 0) break;
    if (buf.overrun) {
      if (!headerPrinted) {
        Serial.println();
        Serial.println(F("Overruns:"));
        Serial.println(F("fileBlockNumber,sdBlockNumber,overrunCount"));
        headerPrinted = true;
      }
      Serial.print(bn);
      Serial.print(',');
      Serial.print(bgnBlock + bn);
      Serial.print(',');
      Serial.println(buf.overrun);
    }
    bn++;
  }
  if (!headerPrinted) {
    Serial.println(F("No errors found"));
  } else {
    Serial.println(F("Done"));
  }
}
//------------------------------------------------------------------------------
// dump data file to Serial
void dumpData() {
  block_t buf;
  if (!binFile.isOpen()) {
    Serial.println(F("No current binary file"));
    return;
  }
  binFile.rewind();
  if (binFile.read(&buf , 512) != 512) {
    error("Read metadata failed");
  }
  Serial.println();
  Serial.println(F("Type any character to stop"));
  delay(1000);
  while (!Serial.available() && binFile.read(&buf , 512) == 512) {
    if (buf.count == 0) break;
    if (buf.overrun) {
      Serial.print(F("OVERRUN,"));
      Serial.println(buf.overrun);
    }
    for (uint16_t i = 0; i < buf.count; i++) {
      Serial.print(buf.data[i], DEC);
      if ((i+1)%PIN_COUNT) {
        Serial.print(',');
      } else {
        Serial.println();
      }
    }
  }
  Serial.println(F("Done"));
}
//------------------------------------------------------------------------------
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;
void logData() {
  uint32_t bgnBlock, endBlock;  
  // allocate extra buffer space
  block_t block[BUFFER_BLOCK_COUNT];
  
  Serial.println();
  
  // initialize ADC and timer1
  adcInit((metadata_t*) &block[0]);
  
  // Find unused file name.
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(binName)) {
    if (binName[BASE_NAME_SIZE + 1] != '9') {
      binName[BASE_NAME_SIZE + 1]++;
    } else {
      binName[BASE_NAME_SIZE + 1] = '0';
      if (binName[BASE_NAME_SIZE] == '9') {
        error("Can't create file name");
      }
      binName[BASE_NAME_SIZE]++;
    }
  }
  // delete old log file
  if (sd.exists(TMP_FILE_NAME)) {
    Serial.println(F("Deleting tmp file"));
    if (!sd.remove(TMP_FILE_NAME)) {
      error("Can't remove tmp file");
    }
  }
  // create new file
  Serial.println(F("Creating new file"));
  binFile.close();
  if (!binFile.createContiguous(sd.vwd(),
    TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT)) {
    error("createContiguous failed");
  }
  // get address of file on SD
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
    error("contiguousRange failed");
  }
  // use SdFat's internal buffer
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0) error("cacheClear failed"); 
 
  // flash erase all data in file
  Serial.println(F("Erasing all data"));
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) {
    endErase = bgnErase + ERASE_SIZE;
    if (endErase > endBlock) endErase = endBlock;
    if (!sd.card()->erase(bgnErase, endErase)) {
      error("erase failed");
    }
    bgnErase = endErase + 1;
  }
  // start multiple block write
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) {
    error("writeBegin failed");
  }
  // Write metadata.
  if (!sd.card()->writeData((uint8_t*)&block[0])) {
    error("Write metadata failed");
  } 
  // initialize queues
  emptyHead = emptyTail = 0;
  fullHead = fullTail = 0;
  
  // use SdFat buffer for one block
  emptyQueue[emptyHead] = (block_t*)cache;
  emptyHead = queueNext(emptyHead);
  
  // put rest of buffers in empty queue
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) {
    emptyQueue[emptyHead] = &block[i];
    emptyHead = queueNext(emptyHead);
  }
  delay(1000);
  Serial.println(F("Logging - type any character to stop"));
  // wait for Serial Idle
  Serial.flush();
  delay(10);
  uint32_t bn = 1;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;

  // start logging interrupts
  adcStart();
  while (1) {
    if (fullHead != fullTail) {
      // block to write
      block_t* pBlock = fullQueue[fullTail];
      
      // write block to SD
      uint32_t usec = micros();
      if (!sd.card()->writeData((uint8_t*)pBlock)) {
        error("write data failed");
      }
      usec = micros() - usec;
      t1 = millis();
      if (usec > maxLatency) maxLatency = usec;
      count += pBlock->count;
      
      // check for overrun
      if (pBlock->overrun) {
        overruns += pBlock->overrun;
        if (ERROR_LED_PIN >= 0) {
          digitalWrite(ERROR_LED_PIN, HIGH);
        }
      }
      // move block to empty queue
      emptyQueue[emptyHead] = pBlock;
      emptyHead = queueNext(emptyHead);
      fullTail = queueNext(fullTail);
      bn++;
      if (bn == FILE_BLOCK_COUNT) {
        // stop ISR calls
        adcStop();
        break;
      }
    }
    if (timerError) {
      error("Missed timer event - rate too high");
    }
    if (Serial.available()) {
      // stop ISR calls
      adcStop();
      if (isrBuf != 0 && isrBuf->count >= PIN_COUNT) {
        // Truncate to last complete sample.
        isrBuf->count = PIN_COUNT*(isrBuf->count/PIN_COUNT);
        // put buffer in full queue
        fullQueue[fullHead] = isrBuf;
        fullHead = queueNext(fullHead);
        isrBuf = 0;
      }
      if (fullHead == fullTail) break;
    }
  }
  if (!sd.card()->writeStop()) {
    error("writeStop failed");
  }
  // Truncate file if recording stopped early.
  if (bn != FILE_BLOCK_COUNT) {    
    Serial.println(F("Truncating file"));
    if (!binFile.truncate(512L * bn)) {
      error("Can't truncate file");
    }
  }
  if (!binFile.rename(sd.vwd(), binName)) {
     error("Can't rename file");
   }
  Serial.print(F("File renamed: "));
  Serial.println(binName);
  Serial.print(F("Max block write usec: "));
  Serial.println(maxLatency);
  Serial.print(F("Record time sec: "));
  Serial.println(0.001*(t1 - t0), 3);
  Serial.print(F("Sample count: "));
  Serial.println(count/PIN_COUNT);
  Serial.print(F("Samples/sec: "));
  Serial.println((1000.0/PIN_COUNT)*count/(t1-t0));
  Serial.print(F("Overruns: "));
  Serial.println(overruns);
  Serial.println(F("Done"));
}
//------------------------------------------------------------------------------
void setup(void) {
  if (ERROR_LED_PIN >= 0) {
    pinMode(ERROR_LED_PIN, OUTPUT);
  }
  Serial.begin(9600);
  
  // Read the first sample pin to init the ADC.
  analogRead(PIN_LIST[0]);
  
  Serial.print(F("FreeRam: "));
  Serial.println(FreeRam());

  // initialize file system.
  if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) {
    sd.initErrorPrint();
    fatalBlink();
  }
}
//------------------------------------------------------------------------------
void loop(void) {
  // discard any input
  while (Serial.read() >= 0) {}
  Serial.println();
  Serial.println(F("type:"));
  Serial.println(F("c - convert file to CSV")); 
  Serial.println(F("d - dump data to Serial"));  
  Serial.println(F("e - overrun error details"));
  Serial.println(F("r - record ADC data"));

  while(!Serial.available()) {}
  char c = tolower(Serial.read());
  if (ERROR_LED_PIN >= 0) {
    digitalWrite(ERROR_LED_PIN, LOW);
  }
  if (c == 'c') {
    binaryToCsv();
  } else if (c == 'd') {
    dumpData();
  } else if (c == 'e') {    
    checkOverrun();
  } else if (c == 'r') {
    logData();
  } else {
    Serial.println(F("Invalid entry"));
  }
}
