// Tiek deklarētas bibliotēkas, kuras nodrošinās noteiktu funkciju darbību.
#include <DHT.h> // DHT un DHT_U nodrošina DHT22, gaisa temperatūras un mitruma sensora, darbību.
#include <DHT_U.h>
#include <ESP32Servo.h> // Nodrošina ESP32 mikrokontrollera iespēju rīkoties ar servo motoriem.
#include <analogWrite.h> // Nodrošina iespēju nolasīt OKY3339 gaisa kvalitātes sensora analogo signālu. 
#include <ESP32Tone.h> // Nodrošina ESP32 mikrokontrollera iespēju rīkoties ar servo motoriem.
#include <ESP32PWM.h> // Nodrošina ESP32 mikrokontrollera iespēju rīkoties ar servo motoriem.

// Tiek deklarētas nemainīgas vērtības, kuras tiks atkārtoti izmantotas pārējā kodā.
#define ledPinR 23 // LED sarkanās diodes kontakts uz ESP32 mikrokontrollera.
#define ledPinG 19 // LED zaļās diodes kontakts uz ESP32 mikrokontrollera.
#define ledPinB 18 // LED zilās diodes kontakts uz ESP32 mikrokontrollera.
#define servoPinH 16 // Augstā spēka servo motora kontakts uz ESP32 mikrokontrollera.
#define servoPinL 5 // Zemā spēka servo motora kontakts uz ESP32 mikrokontrollera.
#define sensorPinTmp 17 // Gaisa temperatūras un mitruma sensora kontakts uz ESP32 mikrokontrollera.
#define sensorPinAq 33 // Gaisa kvalitātes sensora kontakts uz ESP32 mikrokontrollera.
#define pAqOpen 1000 // CO2 vērtība pie kuras tiek atvērts logs, mērvienība PPM (Parts per million).
#define pAqClose 700 // CO2 vērtība pie kuras tiek aizvērts logs, mērvienība PPM (Parts per million).

// Tiek deklarētas nemainīgas vērtības, kuras tiks izmantotas CO2 PPM vērtības aprēķināšanai.
#define aqpValA ...
#define aqpValB ...
#define aqpValR0 ...
#define aqpValRl ...
#define aqpMaxVolt 3.3 

// DHT22 sensora uzstādīšana.
DHT my_sensor(sensorPinTmp, DHT22);

// Servo morotu uzstādīšana.
Servo myServoH;
Servo myServoL;

// Galvenā uzstādīšanas funkcija, kura notiek tikai vienu reizi, kad ierīce ieslēdzas.
void setup() {
  // RGB LED diodes izeju izstādīšana.
  pinMode(ledPinR, OUTPUT);
  pinMode(ledPinG, OUTPUT);
  pinMode(ledPinB, OUTPUT);
  // RGB LED krāsas nozīmes:
  // Zils - DHT22 un OKY3339 vērtību nolasīšana, 
  // Zaļš - CO2 vērtības izvadīšana, mērvienība PPM (Parts per million),
  // Dzeltens - Gaisa temperatūras vērtības izvadīšana, mērvienība grādi pēc Celsija,
  // Ciāna - Gaisa mitruma vērtības izvadīšana, mērvienība %,
  // Sarkans - Servo motor darbība, 
  // Rozā - OKY3339 analogais signāls tiek pārveidots par PPM vērtību,
  // Balts - Vērtības izvadīšanai nepieciešamo aprēķinu process. 

  // OKY3339 sensora uzstādīšana.
  pinMode(sensorPinAq, INPUT);
  
  // DHT22 sensora uzstādīšana.
  my_sensor.begin();
  
  // Servo morotu uzstādīšana.
  myServoH.attach(servoPinH);
  myServoL.attach(servoPinL);
}

// Tiek deklarēti globālie mainīgie.
int aqOutRaw; // OKY3339 analogā vērtība, 0 - 4095.
int aqOutPPM; // CO2 koncentrācija (PPM).
int TmpOutTemp; // DHT22 izvadītā temperatūra.
int TmpOutHumid; // DHT22 izvadītais gaisa mitrums.
int servoStateH; // Augstā spēka servo motora pozīcija.
int servoStateL; // Zemā spēka servo motora pozīcija.

// Informācijas vizuālai izvadei nepieciešamās nemainīgās vērtības.
#define ditL 500 // Īsais vai 0 vērtības ilgums milisekundēs. 
#define dahL 1500 // Garais vai 1 vērtības ilgums milisekundēs. 
#define midGapL 500 // Iekšējās spraugas ilgums milisekundēs.
#define endGapL 500 // Galējās spraugas ilgums milisekundēs.
#define infoLoops 2 // Reizes cik atkārtosies 30 sekunžu intervāls, kura laikā tiks izvadītas  CO2 koncentrācijas PPM, gaisa temperatūras un gaisa mitruma vērtības.

// Mērogs, kurā tiek izvadītas CO2 koncentrācijas (PPM), gaisa temperatūras un gaisa mitruma vērtības.
#define binPpmScale 100 // CO2 koncentrācijas PPM. 100 PPM = 1
#define binTempScale 3 // Gaisa temperatūra. 3°C = 1
#define binHumidScale 4 // Gaisa mitrums. 4% = 1

// Izvadāmās binārās vērtības,
int outputBit1; // 1. bita vērtība,
int outputBit2; // 2. bita vērtība,
int outputBit3; // 3. bita vērtība,
int outputBit4; // 4. bita vērtība,
int outputBit5; // 5. bita vērtība.

// Galvenā cilpas funkcija, kura atkārtojas bezgalīgi.
void loop() {
  // Iekļauj tikai citu funkciju izsaukšanas kodu.
  checkSensor();
  rawAqToPpm();
  setServoPos();
  outputLedCodeAQ();
}

// Sensoru nolasīšanas funkcija.
void checkSensors() {
  digitalWrite(ledPinB, HIGH); // Iedegas zilā LED diode.
  TmpOutTemp = my_sensor.readTemperature(); // Nolasa gaisa temperatūru ar DHT22 sensoru.
  TmpOutHumid = my_sensor.readHumidity(); // Nolasa gaisa mitrumu ar DHT22 sensoru. 
  aqOutRaw = analogRead(sensorPinAq); // Nolasa OKY3339 analogo vērtību (gaisa kvalitāti).
  digitalWrite(ledPinB, LOW); // Nodziest zilā LED diode.
}

// OKY3339 analogās vērtības pārveidošana uz CO2 koncentrācijas (PPM) vērtību.
void rawAqToPpm() {
  digitalWrite(ledPinR, HIGH); // Iedegas sarkanā LED diode.
  digitalWrite(ledPinB, HIGH); // Iedegas zilā LED diode un izveidojas rozā krāsa.

  // Pietrūks kods priekš OKY3339 analogā signāla pārveidošanas.

  digitalWrite(ledPinR, LOW); // Nodziest sarkanā LED diode.
  digitalWrite(ledPinB, LOW); // Nodziest zilā LED diode.
}

// Servo motoro pozīcijas izmaiņa, atkarībā no CO2 koncentrācijas (PPM).
void setServoPos() {
  digitalWrite(ledPinR, HIGH); // Iedegas sarkanā LED diode.

  // Ja gaisa kvalitāte ir pārāk slikta, attaisīt logu.
  if(AqOut > pAqOpen){
    myServoL.write(180) // Zemā spēka servo motors tiek pagriezts uz 180 grādu pozīciju.
    servoStateL = 180; // Mainīgais tiek iestatīts uz pašreizējo zemā spēka servo motora pozīciju.
    delay(3000); // 3000 milisekundes nekas nenotiek, lai motors paspēj veikt savu darbību.
    myServoH.write(90) // Augstā spēka servo motors tiek pagriezts uz 90 grādu pozīciju.
    servoStateH = 90; // Mainīgais tiek iestatīts uz pašreizējo augstā spēka servo motora pozīciju.
    digitalWrite(ledPinR, LOW); // Nodziest sarkanā LED diode.
    delay(200); // 200 milisekundes nekas nenotiek, lai motors paspēj veikt savu darbību.
  }

  // Ja gaisa kvalitāte ir pietiekami laba, aiztaisīt logu.
  if(AqOut < pAqClose){
    myServoH.write(0) // Tas pats kods, kas atver logu, tikai ar citām motoru pozīcijām un augstā spēka motors pirmais veic kustību.
    servoStateH = 0;
    delay(3000);
    myServoL.write(0)
    servoStateL = 0;
    digitalWrite(ledPinR, LOW);
    delay(200);
  }
  
  digitalWrite(ledPinR, HIGH); // Iedegas sarkanā LED diode.
  delay(300); // 300 milisekundes nekas nenotiek, lai signalizētu, kas motoriem bija jāveic: 
  // Sarkanais LED nomirgo - motoru pozīcijas nemainījās.
  // Sarkanais LED deg 3.2 sekundes un tad nomirgo - motoru pozīcijas mainījās.
  digitalWrite(ledPinR, LOW); // Nodziest sarkanā LED diode.
}

// CO2 koncentrācijas (PPM), gaisa temperatūras un gaisa mitruma vērtības izvadīšana binārā Morzes kodā ar RGB LED palīdzību.
void outputLedCode() {
  infoLoopsDone = 0; // Mainīgais, kurš skaita cik reizes izvadīšanas procesā ir veikts. Uzsāk ar vērtību 0.

  // Veic ietverto kodu, līdz izvadīšanas process ir veikts noteikto reižu daudzumu (divas).
  while(infoLoopsDone < infoLoops){

    int outputLoops = 0; // Mainīgais, kurš skaita cik iekšējās cilpas izvadīšanas procesā ir veiktas. Uzsāk ar vērtību 0.

    // Veic ietverto kodu, līdz iekšējās cilpas izvadīšanas procesā ir veiktas 3 reizes. Vienu reizi priekš katras izvadītās vērtības.
    while (outputLoops < 3){
      // Tiek izsaukta funkcija, kura izvadīs 5 bitu vērtību. 
      generateBinary(outputLoops);

      // 5. bita izvadīšana

      // Tiek izsaukta funkcija, kura ieslēgs noteiktas RGB LED diodes.
      setBinColor(outputLoops);
    
      if(outputBit5 == 1){
        delay(dahL); // 1500 milisekundes nekas nenotiek, lai izveidotu garu laika sprīdi, kurā deg LED, kurš apzīmē vērtību 1
      }
      else{
        delay(ditL); // 500 milisekundes nekas nenotiek, lai izveidotu īsu laika sprīdi, kurā deg LED, kurš apzīmē vērtību 0
      }

      // Tiek izsaukta funkcijas, kura izslēdz visas RGB LED diodes.
      rgbOffAll(); 

      // 1. iekšējā sprauga

      delay(midGapL); // 500 milisekundes nekas nenotiek, lai izveidotu īsu laika sprīdi, kurā nedeg LED, kurš apzīmē spraugu starp vērtībām

      // 4. bita izvadīšana
      setBinColor(outputLoops);  // n. bita izvadīšana notiek vēl 4 reizes
      if(outputBit4 == 1){
        delay(dahL);        
      }
      else{
        delay(ditL); 
      }
      rgbOffAll();
      
      // 2. iekšējā sprauga
      delay(midGapL); 

      // 3. bita izvadīšana
      setBinColor(outputLoops);
      if(outputBit3 == 1){
        delay(dahL);        
      }
      else{
        delay(ditL); 
      }
      rgbOffAll();
      
      // 3. iekšējā sprauga
      delay(midGapL); 
      
      // 2. bita izvadīšana
      setBinColor(outputLoops);
      if(outputBit2 == 1){
        delay(dahL);        
      }
      else{
        delay(ditL); 
      }
      rgbOffAll();
      
      // 4. iekšējā sprauga
      delay(midGapL); 
    
      // 1. bita izvadīšana
      setBinColor(outputLoops);
      if(outputBit1 == 1){
        delay(dahL);        
      }
      else{
        delay(ditL); 
      }
      rgbOffAll();

      // Tiek aprēķināts laiks, kurš jānogaida, lai izvade ilgtu apmēra 10 sekundes. Aprēķinātais laiks tiek nogaidīts.
      delay((5 - (outputBit1 + outputBit2 + outputBit3 + outputBit4 + outputBit5))*1000);
      delay(endGapL); // 500 milisekundes nekas nenotiek

      // Mainīgajam tiek pieskaitīts 1, lai atzīmētu cilpas beigas un palielinātu jau veikto cilpu skaitu.
      ++outputLoops;
    }

    // Mainīgajam tiek pieskaitīts 1, lai atzīmētu cilpas beigas un palielinātu jau veikto cilpu skaitu.
    ++infoLoopsDone;
  }
}

// Funkcija nomaina ieslēgtās LED diodes, lai izveidotu noteiktas krāsas, izejot no pašlaik izvadāmās informācijas.
void setBinColor(int outputLoopsColor){
  if (outputLoopsColor == 0){
      rgbOnGreen();     
    }

    if (outputLoopsColor == 1){
      rgbOnYellow(); 
    }

    if (outputLoopsColor == 2){
      rgbOnCyan(); 
    }
}

// Funkcija izslēdz visas RGB LED diodes.
void rgbOffAll(){
  digitalWrite(ledPinR, LOW);
  digitalWrite(ledPinG, LOW);
  digitalWrite(ledPinB, LOW);
}

// Funkcija ieslēdz zaļo LED diodi.
void rgbOnGreen(){
  digitalWrite(ledPinG, HIGH);
}

// Funkcija ieslēdz sarkano un zaļo LED diodi, lai izveidotu dzelteno krāsu.
void rgbOnYellow(){
  digitalWrite(ledPinR, HIGH);
  digitalWrite(ledPinG, HIGH);
}

// Funkcija ieslēdz zaļo un zilo LED diodi, lai izveidotu ciāna krāsu.
void rgbOnCyan(){
  digitalWrite(ledPinG, HIGH);
  digitalWrite(ledPinB, HIGH);
}

// Funkcija pārveido CO2 koncentrācijas (PPM), gaisa temperatūras un gaisa mitruma vērtības izvadīšanai binārā Morzes kodā ar RGB LED palīdzību.
// Funkcijai tiek iedots veikto iekšējo cilpu daudzums, lai varētu noteikt kuru vērtību ir jāpārveido. 
void generateBinary(int outputLoopsBinGen){
  digitalWrite(ledPinR, HIGH); // Tiek ieslēgtas sarkanā, zaļā un zilā diode, izveidojot balto krāsu.
  digitalWrite(ledPinG, HIGH);
  digitalWrite(ledPinB, HIGH);

  int inputNumber; // Pārveidojamā vērtība.
  int binGenVal = 16; // Mainīgais, kas nosaka kurš bits pašlaik tiek iestatīts (16 ir 5. bits; 8 ir 4. bits; 4 ir 3. bits; ...).
  int tempVal; // Mainīgais, kurš īslaicīgi saglabā bita vērtību, līdz to iedod pašlaik nosakāmajam bitam un tiek atlikts atpakaļ uz 0.

  // Tiek pārbaudīts, kuru vērtību pārveidot. 
  if(outputLoopsBinGen == 0){
    int inputNumber = aqOutPPM / binPpmScale; // Ja CO2 koncentrācijas (PPM) ir jāpārveido, tā tiek pārveidota mērogam un iestatīta kā pārveidojamā vērtība.
  }
  if(outputLoopsBinGen == 1){
    int inputNumber = TmpOutTemp / binTempScale; // Ja jāpārveido gaisa temperatūra.  
  }
  if(outputLoopsBinGen == 2){
    int inputNumber = TmpOutHumid / binHumidScale; // Ja jāpārveido gaisa mitrums.
  }
  // Tiek pārbaudīts vai pārveidojamā vērtība dalās ar 2 bez atlikumiem.
  if (inputNumber % 2 == 0){
    outputBit1 = 0; // Ja dalās, vērtība ir pāra skaitlis, kas nozīme, ka 1. bita vērtība ir 0.
  }
  else{
    outputBit1 = 1; // Ja nedalās, vērtība ir nepāra skaitlis, kas nozīme, ka 1. bita vērtība ir 1.
    --inputNumber; // Atņem 1 no pārveidojamās vērtības.
  }

  // Veic cilpu, kurā, ar katru cilpu, tiek pārbaudīta katra nākošā bita vērtība. 
  // Sāk pārbaudi ar 5. bitu un iet uz leju, līdz 2. bita vērība ir pārbaudīta.
  while (binGenVal != 1){

    if(inputNumber - binGenVal >= 0){ // Ja pārveidojamā vērtība ir vienāda vai lielāka par pašreizējā bita apzīmēto skaitli,
      tempVal = 1; // pašreizējā bita vērtība tiek iestatīta uz 1 un pārveidojamās vērtības atņem bita apzīmēto skaitli.
      inputNumber = inputNumber - binGenVal;
    }
    else{
      tempVal = 0;
    }

    if (binGenVal == 16){ // Iestata 5. bitu
      outputBit5 = tempVal;
    }
    if (binGenVal == 8){ // Iestata 4. bitu
      outputBit4 = tempVal;
    }
    if (binGenVal == 4){
      outputBit3 = tempVal;
    }
    if (binGenVal == 2){
      outputBit2 = tempVal;
    }

    binGenVal = binGenVal / 2; // Katras cilpas beigās bita apzīmēto skaitli dala ar 2, lai pārietu uz nākamo bitu, jo katrs bits ir 2 kāpinājums.
  }
  digitalWrite(ledPinR, LOW); // Izslēdz visas RGB LED diodes.
  digitalWrite(ledPinG, LOW);
  digitalWrite(ledPinB, LOW);
}
