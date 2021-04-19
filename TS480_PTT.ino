//--------------------------------------------------------
// MW0LGE - Richard Samphire - 2021 - See LICENSE file
// Crude implementation of TS-480 Cat Command Parser
// Used to set a relay state based on TX/RX
// Other commands handled so that connecting software plays ball (such as HRD/flrig/MMSSTV)
//--------------------------------------------------------

#include <errno.h>

//-
#define PTT_RELAY_PIN 7
//-
#define TOT_SECONDS 240 // Time Out Timer of 4 mins // 0 to disable
#define DEF_FREQ 145000000
//-
static char m_buf[256];
static char m_tmpBuf[64];
//-
int m_nBufferPos = 0;
unsigned long m_lStartMillis = 0;
//-
static char m_vfoReceiver[] = "0";
static char m_vfoTransmitter[] = "0";
//-
bool m_bTxState = false;
unsigned long m_lFrequencyA = DEF_FREQ;
unsigned long m_lFrequencyB = DEF_FREQ;
//-

void setTXState()
{
  if(m_bTxState)
  {
      m_lStartMillis = millis();
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(PTT_RELAY_PIN, HIGH);
  }
  else
  {
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(PTT_RELAY_PIN, LOW);
  }  
}

void handleMsg()
{
  if (m_nBufferPos < 2) return; // need at least 2 chars !

  bool bReply = true; // set to false to prevent reply msg
  
  sprintf(m_tmpBuf, "%c%c", tolower(m_buf[0]), tolower(m_buf[1]));
   
  if(strcmp(m_tmpBuf, "tx") == 0) // set TX
  {
    m_bTxState = true;
    setTXState();

    sprintf(m_tmpBuf, "TX0;");  // NOTE: spec says we should return TX0; as a reply, this causes issues with HAMLIB

    bReply = false;
  }
  else if(strcmp(m_tmpBuf, "rx") == 0) // set RX
  {
    m_bTxState = false;
    setTXState();

    sprintf(m_tmpBuf, "RX0;");

    bReply = false;  // NOTE: spec says we should return RX0; as a reply, this causes issues with HAMLIB
  }
  else if(strcmp(m_tmpBuf, "fa") == 0) // vfo A freq
  {
    if(m_nBufferPos == 13) {    
      // new frequency
      char freq[12];
      strncpy(freq, m_buf + 2, 11);
      freq[11] = 0;

      m_lFrequencyA = strtol(freq, NULL, 10);
      if(errno == ERANGE) m_lFrequencyA = DEF_FREQ;
      
      bReply = false;
    }
    sprintf(m_tmpBuf, "FA%011lu;", m_lFrequencyA);
  }
  else if(strcmp(m_tmpBuf, "fb") == 0)  // vfo B freq
  {
    if(m_nBufferPos == 13) {    
      // new frequency
      char freq[12];
      strncpy(freq, m_buf + 2, 11);
      freq[11] = 0;

      m_lFrequencyB = strtol(freq, NULL, 10);
      if(errno == ERANGE) m_lFrequencyB = DEF_FREQ;
      
      bReply = false;
    }
    sprintf(m_tmpBuf, "FB%011lu;", m_lFrequencyB);
  }
  else if(strcmp(m_tmpBuf, "if") == 0) // transceiver status
  {
    //IF 00144174000 00000 +0000 0 0 0 00 0 4 0 0 0 0 00 0;
    //^^ from eesdr
    
    char hz[] = "00000000000";
    char space[] = "00000"; // "     ";  // note spec says this should be space but other implementions return 0's, so do we
    char ritxit[] = "+0000";
    char ritOn[] = "0";
    char xitOn[] = "0";
    char memChanBankNo[] = "0";
    char memNumber[] = "00";
    char rxtx[] = "0";
    char opMode[] = "4"; // fm
    char frft[] = "0"; // vfo a
    char scanStatus[] = "0";
    char simplexSplit[] = "0";
    char txtone[] = "0";
    char toneNumber[] = "00";
    char anotherSpace[] = "0"; // " ";  // note spec says this should be a space but other implementions return 0, so do we

    sprintf(hz, "%011lu", m_lFrequencyA);

    if(m_bTxState)
    {
      sprintf(rxtx, "1");
      sprintf(frft, "%s", m_vfoTransmitter);
    }
    else
    {
      sprintf(frft, "%s", m_vfoReceiver);
    }

    if(strcmp(m_vfoTransmitter, m_vfoReceiver) != 0)
    {
      // if vfos are different for both trans + rec, then assume split
      sprintf(simplexSplit, "1");  
    }
    
    sprintf(m_tmpBuf, "IF%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s;", hz, space, ritxit, ritOn, xitOn, memChanBankNo, memNumber, rxtx, opMode, frft, scanStatus, simplexSplit, txtone, toneNumber, anotherSpace);
  }
  else if(strcmp(m_tmpBuf, "sm") == 0) // smeter
  {   
    sprintf(m_tmpBuf, "SM00020;");
  }   
  else if(strcmp(m_tmpBuf, "rm") == 0) // read meter
  {
    char sMeterNo[] = "1";
    if(m_nBufferPos == 3) {
      // we have a meter number
      sprintf(sMeterNo, "%c", m_buf[2]);
    }
    sprintf(m_tmpBuf, "RM%s0010;", sMeterNo);
  }   
  else if(strcmp(m_tmpBuf, "md") == 0) // transceiver status
  {
    sprintf(m_tmpBuf, "MD4;"); // force to FM
  }      
  else if(strcmp(m_tmpBuf, "rs") == 0) // transceiver status
  {
    sprintf(m_tmpBuf, "RS0;");
  }
  else if(strcmp(m_tmpBuf, "rt") == 0) // rit status
  {
    sprintf(m_tmpBuf, "RT0;");
  }  
  else if(strcmp(m_tmpBuf, "xt") == 0) // xit status
  {
    sprintf(m_tmpBuf, "XT0;");
  }    
  else if(strcmp(m_tmpBuf, "ps") == 0) // power on statis
  {
    sprintf(m_tmpBuf, "PS1;");
  }
  else if(strcmp(m_tmpBuf, "ty") == 0) // firmware type
  {
    sprintf(m_tmpBuf, "TY000;");
  }
  else if(strcmp(m_tmpBuf, "by") == 0) // busy signal
  {
    sprintf(m_tmpBuf, "BY00;");
  }  
  else if(strcmp(m_tmpBuf, "id") == 0) // trx id number (020 is TS-480)
  {
    sprintf(m_tmpBuf, "ID020;");
  }   
  else if(strcmp(m_tmpBuf, "xi") == 0) // read tx freq
  {
    char opMode[] = "4"; // fm
    char stepSize[] = "00";
    
    sprintf(m_tmpBuf, "XI00145000000%s%s;", opMode, stepSize);
  }
  else if(strcmp(m_tmpBuf, "ai") == 0) // auto info function
  {   
    sprintf(m_tmpBuf, "AI0;");
  }
  else if(strcmp(m_tmpBuf, "qr") == 0) // quick memory channel data
  {   
    sprintf(m_tmpBuf, "QR00;");
  }  
  else if(strcmp(m_tmpBuf, "cn") == 0) // ctcss tone number
  {   
    sprintf(m_tmpBuf, "CN00;");
  }    
  else if(strcmp(m_tmpBuf, "ct") == 0) // ctcss status
  {   
    sprintf(m_tmpBuf, "CT0;");
  }     
  else if(strcmp(m_tmpBuf, "tn") == 0) // tone number
  {   
    sprintf(m_tmpBuf, "TN00;");
  }  
  else if(strcmp(m_tmpBuf, "to") == 0) // tone on/off
  {   
    sprintf(m_tmpBuf, "TO0;");
  }  
  else if(strcmp(m_tmpBuf, "ft") == 0) // vfo/mch mode of transmitter
  {   
    if(m_nBufferPos == 3) {
      // we have a parameter
      sprintf(m_vfoTransmitter, "%c", m_buf[2]);
    }        
    sprintf(m_tmpBuf, "FT%s;", m_vfoTransmitter);
  } 
  else if(strcmp(m_tmpBuf, "fr") == 0) // vfo/mch mode of receiver
  {   
    if(m_nBufferPos == 3) {
      // we have a parameter
      sprintf(m_vfoReceiver, "%c", m_buf[2]);
    }    
    sprintf(m_tmpBuf, "FR%s;", m_vfoReceiver);
  }   
  else if(strcmp(m_tmpBuf, "sh") == 0) // dsp
  {   
    sprintf(m_tmpBuf, "SH00;");
  } 
  else if(strcmp(m_tmpBuf, "bc") == 0) // beat canceller
  {   
    sprintf(m_tmpBuf, "BC0;");
  }  
  else if(strcmp(m_tmpBuf, "an") == 0) // antenna
  {   
    sprintf(m_tmpBuf, "AN1;");
  }    
  else if(strcmp(m_tmpBuf, "fw") == 0) // dsp bw
  {   
    sprintf(m_tmpBuf, "FW0000;"); // fm normal width
  }  
  else if(strcmp(m_tmpBuf, "fs") == 0) // fine tune
  {   
    sprintf(m_tmpBuf, "FS0;");
  }   
  else if(strcmp(m_tmpBuf, "nb") == 0) // noise blanker
  {   
    sprintf(m_tmpBuf, "NB0;");
  }   
  else if(strcmp(m_tmpBuf, "nr") == 0) // noise reduction
  {   
    sprintf(m_tmpBuf, "NR0;");
  }
  else if(strcmp(m_tmpBuf, "pa") == 0) // PreA status
  {   
    sprintf(m_tmpBuf, "PA00;");
  } 
  else if(strcmp(m_tmpBuf, "pr") == 0) // speech proc
  {   
    sprintf(m_tmpBuf, "PR0;");
  } 
  else if(strcmp(m_tmpBuf, "mf") == 0) // menu A
  {   
    sprintf(m_tmpBuf, "MF0;");
  }  
  else if(strcmp(m_tmpBuf, "vx") == 0) // vox status
  {   
    sprintf(m_tmpBuf, "VX0;");
  }   
  else if(strcmp(m_tmpBuf, "ca") == 0) // cw auto zero beat
  {   
    sprintf(m_tmpBuf, "CA0;");
  }  
  else if(strcmp(m_tmpBuf, "ts") == 0) // tf-set status
  {   
    sprintf(m_tmpBuf, "TS0;");
  }  
  else if(strcmp(m_tmpBuf, "ac") == 0) // antenna tuner status
  {   
    sprintf(m_tmpBuf, "AC000;");
  }    
  else if(strcmp(m_tmpBuf, "ag") == 0) // af gain
  {   
    sprintf(m_tmpBuf, "AG0128;");
  }  
  else if(strcmp(m_tmpBuf, "ra") == 0) // attenuator status
  {   
    sprintf(m_tmpBuf, "RA0000;");
  }    
  else if(strcmp(m_tmpBuf, "rg") == 0) // rf gain status
  {   
    sprintf(m_tmpBuf, "RG050;");
  }   
  else if(strcmp(m_tmpBuf, "pc") == 0) // output power
  {   
    sprintf(m_tmpBuf, "PC050;");
  }    
  else if(strcmp(m_tmpBuf, "mg") == 0) // mic gain
  {   
    sprintf(m_tmpBuf, "MG050;");
  }  
  else if(strcmp(m_tmpBuf, "nl") == 0) // NB level
  {   
    sprintf(m_tmpBuf, "NL005;");
  }   
  else if(strcmp(m_tmpBuf, "rl") == 0) // NR level
  {   
    sprintf(m_tmpBuf, "RL00;");
  } 
  else if(strcmp(m_tmpBuf, "gt") == 0) // agc constant status
  {   
    sprintf(m_tmpBuf, "GT000;");
  }  
  else if(strcmp(m_tmpBuf, "dl") == 0) // digital noise limiter status
  {   
    sprintf(m_tmpBuf, "DL000;");
  }    
  else if(strcmp(m_tmpBuf, "sl") == 0) // dsp settings
  {   
    sprintf(m_tmpBuf, "SL00;");
  }    

  else if(strcmp(m_tmpBuf, "vg") == 0) // vox gain
  {   
    sprintf(m_tmpBuf, "VG000;");
  }    
  else if(strcmp(m_tmpBuf, "vd") == 0) // vox delay
  {   
    sprintf(m_tmpBuf, "VD1500;");
  }    
  else if(strcmp(m_tmpBuf, "sq") == 0) // sql level
  {   
    sprintf(m_tmpBuf, "SQ0128;");
  }    
  else if(strcmp(m_tmpBuf, "is") == 0) // if shift
  {   
    sprintf(m_tmpBuf, "IS 0000;");
  }    
  else if(strcmp(m_tmpBuf, "ks") == 0) // key speed
  {   
    sprintf(m_tmpBuf, "KS030;");
  }    
  else if(strcmp(m_tmpBuf, "sd") == 0) // cw break in delay
  {   
    sprintf(m_tmpBuf, "SD0000;");
  }    
  else if(strcmp(m_tmpBuf, "ex") == 0) // extension menu (filter BW for data OFF)
  {   
    sprintf(m_tmpBuf, "EX0450000;");
  }  
  else if(strcmp(m_tmpBuf, "vv") == 0) // vfoA = vfoB
  {   
    m_lFrequencyA = m_lFrequencyB;
    bReply = false;
  }  
  
  else
  {
    char tmp[10];
    sprintf(tmp, "[%s]", m_tmpBuf);
    sprintf(m_tmpBuf, "%s", tmp);
  }

  if(bReply) 
  {
    Serial.write(m_tmpBuf);
    Serial.flush();
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PTT_RELAY_PIN, OUTPUT);

  m_bTxState = false;
  setTXState();
  
  Serial.begin(19200, SERIAL_8N1);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
}
  
// the loop function runs over and over again forever
void loop() {
  int incByte;
  
  while (Serial.available() > 0)
  {
    incByte = Serial.read();
    
    if(incByte == 59) // the ; terminator
    {
      if(m_nBufferPos > 0)
      {
        m_buf[m_nBufferPos] = 0; // null terminate the string
        
        // handle string
        handleMsg();
      
        // reset buffer pos
        m_nBufferPos = 0;
      }
    }
    else if(incByte >= 32 && incByte <= 126) // most printable
    {
      // add to buffer
      m_buf[m_nBufferPos] = incByte;
      m_nBufferPos++;
      
      if(m_nBufferPos > 250)
      {
        // EEEK !!!!! some crude overflow
        // we have not had a ; so just dump everything
        m_nBufferPos = 0;
      }
    }    
  }

  // ToT check
  if((TOT_SECONDS > 0) && m_bTxState)
  {
    unsigned long lNow = millis();
    unsigned long lElapsed = lNow - m_lStartMillis;
    unsigned long lDuration = TOT_SECONDS * 1000ul;
    
    if( lElapsed >= lDuration )
    {
      m_bTxState = false;
      setTXState();
    }
  }
}
