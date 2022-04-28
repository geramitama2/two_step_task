
// UNIVERSAL CONVENTIONS
// Left CHOICE - movement from left to center
// Transitions : Left --> Pattern A (vertical bars + 9kHz tone)
//               Right --> Pattern B (horizontal bars + 3kHz)
// Left is 0, right is 1


#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>

#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Adafruit_NeoPixel.h>
#include <MovingAverage.h>

#define LED_PIN 4
#define LED_COUNT 256
#define R A10

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN);

//Session Settings
double degrees_per_led = 6.0; // wheel gain
unsigned long incorrect_time = 3000;
unsigned long second_step_length = 2000;
unsigned long timeOut_period = 20000;
int reward_delay = 100;

int save_to_sd = 1;   
int behavior_dependent = 0; 

int trials_in_block_low = 1;
int trials_in_block_high = 4;
int trials_in_block = random(trials_in_block_low,trials_in_block_high);
double forced_percentage = .9;

// daily session settings
int forced = 0;
int correct_side = 0;
double common_transition_probability = .8;
double high_reward_prob = .85;
double low_reward_prob = .15;
int sol_open_time = 40;
double switch_criteria_percent_accurate = .75;
double forced_threshold_lower =.3;
double forced_threshold_upper = .5;
int plus_trials = 0;
int trials_completed_in_block = 0;


int switch_criteria_trials_range_low = 15; 
int switch_criteria_trials_range_high = 25; 
int switch_criteria_trials = random(switch_criteria_trials_range_low,switch_criteria_trials_range_high+1);


unsigned long ITI_setting = random(2000,4000);
unsigned long ITI = ITI_setting;
double percent_correct = 0.5;
double weight_avg = 0.8825;
double weight_data = 1 - weight_avg; 


// hardware settings
double resolution = 3250; // for encoder
uint32_t white = strip.Color(255, 255, 255);
int pixels = 16;
int toneA_pin = 12;
int toneB_pin = 11;
int patternA_tone_freq = 3000;
int patternB_tone_freq = 9000;
int solenoid_pin = 8;

// LED screen variables
int initial_led_pos = 0;
int led_pos = 0;
int last_led_pos = 0;
int left_bounds = 112; // pixel value for left bounds
int right_bounds = 127; // pixel value for right bounds




// time variables
unsigned long t;
unsigned long t_1;
unsigned long loop_time=0;
unsigned long sample_time = 10; //ms                                                                                                                              


// Threshold Values
int number_of_switches_threshold = 20;                                                                                                                           
int consecutive_timeOut_threshold = 20; 
int trial_limit = 600;
unsigned long session_time_threshold = 90*60000;



// Session Variables - These variables will change according to behavior
int number_of_switches=0;
int consecutive_timeOut = 0;
int trial_number = 1;
int quiescent_period = 1000;
int enc_val;
int last_enc_val;
int choices_total_left = 0;
int choices_total_right = 0;
int turn = -1;
int phase = 1;
int done = 0;
int second_state = -1;
int correct = -1;
int correct_trials = 0;
int incorrect_trials = 0;
double last_encoder_value = 0.0; // for quiescent phase
int timeOut_trials = 0;
int transition = -1;
int reward = -1;
int reward_roll;
double reward_prob;
int trials_since_switch = 0;
int enable_switch = 0;
int extra_trials = 0;
int solonoid_open = 0;
Encoder myEnc(2,3);

// SD variables and settings
File myFile;
File LOG;
File params;
int first_save = 1;
String tlt;
int ct = 0;




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
//  lcd.begin();
  lcd.backlight();
  randomSeed(analogRead(R));
  pinMode(7,INPUT_PULLUP);
  pinMode(solenoid_pin,OUTPUT);
  digitalWrite(solenoid_pin,LOW);
  strip.begin();
  strip.setBrightness(10);
  strip.show();
  t_1 = millis(); //stored time
  
  pinMode(53, OUTPUT);
  int Andy = random(5000, 10001);
  tlt = String(Andy) + "_JS2.CSV";
  int ctt;
  File root;
    if (save_to_sd==1){
    if (SD.begin()) {
      root = SD.open("/");
      ctt = printDirectory(root, 0);
      ctt = ctt - 2;
      Serial.println(String(ctt));
      lcd.print(String(ctt));
      if (ctt > 1 ){
        
        Serial.println("Too many files");
        lcd.print("Too many files");
        save_to_sd = 0;
        phase = 0;
        return;
      }
  
  //  Reading in log
      LOG = SD.open("Log.txt", FILE_READ);
      if (LOG) {
        while (LOG.available()) {
          Serial.println(LOG.readStringUntil('\n') + '_' + String(ct));
          lcd.print(LOG.readStringUntil('\n') + '_' + String(ct));
          ct++;
        }
        LOG.close();
      }
      else {
        Serial.println("Log didn't open");
        lcd.print("Log didn't open");
        save_to_sd = 0;
        phase = 0;
        return;
      }
      
  //  Writing to the log
      LOG = SD.open("Log.txt", FILE_WRITE);
      if (LOG) {
        Serial.println(String(LOG.position()));
        lcd.print(String(LOG.position()));
        LOG.seek(LOG.position() + 1);
        LOG.println(String(ct) + ' ' + String(tlt));
        LOG.close();
        phase = 1;
      }
 
    }
    else{
      Serial.println("SD card initialization failed");
      lcd.print("No Card");
      save_to_sd = 0;
      phase = 1000;
      return;
    }
    
    
    params = SD.open("params.txt", FILE_WRITE);
    if (params) {

      
      params.println("CSV_Number:"+ String(Andy));
      params.println("sample_time:"+String(sample_time));
      params.println("timeOut_period:"+String(timeOut_period));
      params.println("quiescent_period:"+String(quiescent_period));
      params.println("second_step_length:"+String(second_step_length));
      params.println("incorrect_time:"+String(incorrect_time));
      params.println("reward_delay:"+String(reward_delay));
      params.println("ITI_setting:"+String(ITI_setting));
      params.println("degrees_per_led:"+String(degrees_per_led));  
      params.println("switch_criteria_trials_range_low:"+String(switch_criteria_trials_range_low));
      params.println("switch_criteria_trials_range_high:"+String(switch_criteria_trials_range_high));
      params.println("switch_criteria_percent_accurate:"+String(switch_criteria_percent_accurate));
      params.println("session_time_threshold:"+String(session_time_threshold));
      params.println("number_of_switches_threshold:"+String(number_of_switches_threshold));
      params.println("switch_criteria_trials:"+String(switch_criteria_trials));
      params.println("trial_limit:"+String(trial_limit));
      params.println("consecutive_timeOut_threshold:"+String(consecutive_timeOut_threshold));
      
      params.println("correct_side:"+String(correct_side));
      params.println("forced_threshold_upper:"+String(forced_threshold_upper));
      params.println("forced_threshold_lower:"+String(forced_threshold_lower));
      params.println("sol_open_time:"+String(sol_open_time));
      params.println("common_transition_probability:"+String(common_transition_probability));
      params.println("high_reward_prob:"+String(high_reward_prob));
      params.println("low_reward_prob:"+String(low_reward_prob));
      params.println("plus_trials:"+String(plus_trials));   
      params.println("switch_criteria_trials_range_low:"+String(switch_criteria_trials_range_low));    
      params.println("switch_criteria_trials_range_high:"+String(switch_criteria_trials_range_high));    
           
      params.close();
    }
    
    else{
      Serial.println("did not print");
      phase = 1000;
      save_to_sd = 0;
    }
  }
  lcd.clear();
  lcd.setCursor(0,0);
}

void loop() {
    t = millis();
    if(t-loop_time>=sample_time){
      loop_time=t;
      if (number_of_switches >= number_of_switches_threshold or t>session_time_threshold or consecutive_timeOut>=consecutive_timeOut_threshold or trial_number>=trial_limit){
        phase = 0;
        done = 1;
        strip.clear();
        strip.show();
      }
    }
    switch(phase){
      case 1: // quiescent phase - mouse must keep wheel steady for this time
        last_encoder_value = abs((360.0*myEnc.read())/resolution);
        if(last_encoder_value>=2.0){
          strip.clear();
          strip.show();
          myEnc.readAndReset();
          last_encoder_value = 0;
          t_1 = t;
        }
        if((t-t_1)>=quiescent_period){
          strip.clear();
          strip.show();

          // every trial, there is a percentage chance that there will be a forced trial
          if(random(0,101)>forced_percentage*100){
            forced = 0;
          }else{
            forced = 1;
          }
          setTarget2(forced, correct_side);
          phase = 2;
          t_1 = t;
        }
        break;
        
      case 2: // turning on the vertical lines
        verticalLinesOn(119);
        strip.show();
        phase = 3;
        last_enc_val = 0;
        enc_val = 0; 
        break;
        
      case 3:
        fix_encoder();
        enc_val = force_encoder(forced,correct_side,last_enc_val,myEnc.read()); 
        led_pos = 119 +get_led_position(((360.0*enc_val))/resolution,degrees_per_led,pixels);
        led_pos = constrain(led_pos,left_bounds,right_bounds); 
        verticalLinesOn(led_pos);
        
        if (last_led_pos!=led_pos){
            verticalLinesOff(last_led_pos);
            strip.show();
        }
        last_enc_val = enc_val;
        last_led_pos = led_pos;
        
        if(led_pos<=115){    // left target hit
          
          choices_total_left++;
          turn = 0;
        }
        if(led_pos>=123){   // right target hit
       
          choices_total_right++;
          turn = 1;
        }
        if (turn != -1) { // non-timeout
          consecutive_timeOut = 0;
          myEnc.readAndReset();
          strip.clear();
          strip.show();
          t_1 = t;
          phase = 5; // reward roll phase
          
          if (correct_side == 0) {// left side is correct side
            last_led_pos = 0;
            led_pos = 0;
            second_state = transition_assignment(turn,100*common_transition_probability,forced); 
           
             if (turn == 0) {
              Serial.println("Correct Choice!");
              //correct_trials +=1;
              correct = 1;  
             }
             if (turn == 1) {
              Serial.println("Incorrect Choice!");
              incorrect_trials +=1;
              correct = 0;
             }
          }
          if (correct_side == 1) {
            last_led_pos = 0;
            led_pos = 0;
            second_state = transition_assignment(turn,100*common_transition_probability,forced); 
              
            if (turn == 0) {
              Serial.println("Incorrect Choice!");
              incorrect_trials +=1;
              correct = 0;
            }
            if (turn == 1) {
            Serial.println("Correct Choice!");
            //correct_trials +=1;
            correct = 1;
            }
          }
          break;
        }
        if((t-t_1)>=timeOut_period) {//timeout
          correct = 2;
          consecutive_timeOut++;
          timeOut_trials +=1;
          strip.clear();
          strip.show();
          myEnc.readAndReset();
          phase = 4; // no reward timeout phase
          t_1 = t;

          Serial.println("Time Out");
          break;
        }
        break;
      case 4: 
        tone(toneA_pin,random(4000,8000),10);
        if ((t-t_1)>=incorrect_time) {
          t_1 = t;   
          phase = 8; 
          strip.clear();
          strip.show();
          myEnc.readAndReset();
        }
        break;
        
      case 5:
        reward_roll = random(0,101);
        
        // if the second state == optimal side, reward_prob is high
        if(second_state == correct_side){
          reward_prob = high_reward_prob;
        }else{
          reward_prob = low_reward_prob;
        }
        
        if ((t-t_1) >= second_step_length) {
            strip.clear();
            strip.show();
            t_1 = t; 
             
        if(reward_roll<reward_prob*100.0){
            phase = 6;
            reward =1;

            Serial.println("Reward!");
         }
        else{
              phase = 8;
              reward = 0;
              Serial.println("No Reward");
            }
            
        lcd.setCursor(0,1);
        lcd.print(String(correct_side) + " " + String(turn) + " " + String(transition) + " " + String(reward) + " " + String(forced));
        }

        break;
      
      case 6: // turn on solonoid
        if((t-t_1)>=reward_delay){
          digitalWrite(solenoid_pin, HIGH);
          phase = 7;
          t_1 = t;
        }
        break;
        
      case 7: // Turn off solonoid
         if ((t-t_1) >= sol_open_time) {
         digitalWrite(solenoid_pin, LOW);
         phase = 8;
         t_1= t;
         correct_trials +=1;
         //Serial.println("ITI start at: " + String(t));
         }
        break;
        
      case 8:
        
        if ((t-t_1) >= ITI) {
          if (behavior_dependent == 0 and trials_completed_in_block>trials_in_block){
            // if trials in block==block_trials, switch
            trials_in_block = random(trials_in_block_low,trials_in_block_high);
            if(correct_side ==0){
              correct_side = 1;
            }else{
              correct_side = 0;
            }
          }
          
          // block switches are dependent on behavior
          if (behavior_dependent == 1){ 
            if (correct != 2) {
              percent_correct = (percent_correct * weight_avg) + (correct * weight_data);
             } 
             Serial.println();
             Serial.println("Perc Correct: " + String(100*percent_correct));
  
            if (trials_since_switch>=switch_criteria_trials and enable_switch==0) {
              if (percent_correct>=switch_criteria_percent_accurate) {
                enable_switch = 1;
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Switch Enabled");
              }
    
              if (forced == 0 and percent_correct <= (forced_threshold_lower)) {
                forced = 1;
              }
    
              if (forced == 1 and percent_correct >= (forced_threshold_upper)) {
                forced = 0;
              }
            }
  
            if (enable_switch == 1) {
              if (extra_trials < plus_trials) { extra_trials++; }
              if (extra_trials >= plus_trials){
                if (correct_side == 0){
                  correct_side = 1;
                }else{
                  correct_side = 0;
                }
                switch_criteria_trials = random(switch_criteria_trials_range_low,switch_criteria_trials_range_high+1);
                Serial.println("New Block Achieved!");
                lcd.clear();
                lcd.print("New Corr. Side ="+String(correct_side));
                trials_since_switch = 0;
                extra_trials = 0;
                enable_switch = 0;
                number_of_switches++;
                percent_correct = 1-percent_correct;
              }
            }
          }
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(String(trial_number) + " " + String(number_of_switches) + " " + String(trials_since_switch) + " " + String(percent_correct) + " " + String(extra_trials));
          
          
          strip.clear();
          strip.show();
          trial_number++;
          trials_since_switch++;
          ITI = random(2000,4000);        
          phase = 1;
          second_state = -1;
          transition = -1;
          correct= -1;
          turn = -1;
          reward = -1;
          solonoid_open = 0;
          t_1 = t;
        }
        break;
    }

    if(save_to_sd ==1){
      if (trial_number >= trial_limit or done == 1) {
        phase = 1000;
        myFile.close();
        lcd.clear();
        lcd.print("Done!");
      }
      if (done == 0) {
        if (first_save == 1) {
          first_save = 0; myFile = SD.open(tlt, FILE_WRITE);
        }
        if (myFile) {
          myFile.println(String(t) + ','+ String(trial_number) +',' + String(phase)+ ','+ String(correct_side)+ ','+ String(turn)+ ',' + String(correct)+ ',' + String(reward) + ',' + String(transition) + ',' + String(second_state) + ',' + String((360.0*enc_val)/resolution) + ',' + String(digitalRead(7)) + ',' + String(forced));
        }
        else {
          phase = 0;
          lcd.clear();
          lcd.print("Not saving right");
        }
      }
    }

}

void setTarget2(int forced, int correct_side){
  if (forced == 0) {
      strip.setPixelColor(114,white);
      strip.setPixelColor(115,white);
      strip.setPixelColor(140,white);
      strip.setPixelColor(141,white);
      strip.setPixelColor(123,white);
      strip.setPixelColor(124,white);
      strip.setPixelColor(131,white);
      strip.setPixelColor(132,white);
  }
  
  if (forced == 1) {
    if (correct_side == 0) {
      strip.setPixelColor(114,white);
      strip.setPixelColor(115,white);
      strip.setPixelColor(140,white);
      strip.setPixelColor(141,white);
    }
    if (correct_side == 1) {
      strip.setPixelColor(123,white);
      strip.setPixelColor(124,white);
      strip.setPixelColor(131,white);
      strip.setPixelColor(132,white);
    }
  }
  strip.show();
}

void verticalLinesOn(int led1){
  for(int i=0;i<5;i++){
    strip.setPixelColor(led1+32*i,white);
    strip.setPixelColor(led1-32*i,white);
    strip.setPixelColor(led1+32*i,white);
    strip.setPixelColor(led1+32*i -(led1%16+(led1-112)+1),white);
    strip.setPixelColor(led1-32*i -(led1%16+(led1-112)+1),white);
  }
}

void verticalLinesOff(int led1){
  for(int i=0;i<5;i++){
    strip.setPixelColor(led1+32*i,0);
    strip.setPixelColor(led1-32*i,0);
    strip.setPixelColor(led1+32*i,0);
    strip.setPixelColor(led1+32*i -(led1%16+(led1-112)+1),0);
    strip.setPixelColor(led1-32*i -(led1%16+(led1-112)+1),0);
  }
}

void fix_encoder(){
  if ((360.0*myEnc.read())/resolution<=-degrees_per_led*pixels+3){
    myEnc.write(((-degrees_per_led*pixels+3)*resolution)/360);
  }else
    //max value read should not be greater than the smallest degrees value needed to reach the last pixel
    // - 3 to give slight leeway on this value in event of overshoot
    if((360.0*myEnc.read())/resolution>=degrees_per_led*pixels-3){
      myEnc.write(((degrees_per_led*pixels-3)*resolution)/360);
    }
}

int get_led_position(double angle,double degrees_per_led,int leds){
  int led_pos = 0;
  // iteratively determining angle boundaries for a given number of leds
  if (angle<0){
    int min_angle = -leds*degrees_per_led;
    for (int i=0;i<leds;i++){
      if (angle<= -i*degrees_per_led and angle>=-degrees_per_led*(i+1)){
        led_pos = -i ;
        }
      }
    if(angle<=min_angle){
      led_pos = -leds;
    }
  }else if(angle>0){
    int max_angle = leds*degrees_per_led;
    for (int i=0;i<leds;i++){
      if (angle>= i*degrees_per_led and angle<=degrees_per_led*(i+1)){
        led_pos = i ;
      }
    }
    if(angle>=max_angle){
      led_pos = leds; 
    }
  }else{
    led_pos = 0;
  }
  return led_pos;
}

int transition_assignment(int turn, int common_transition_probability, int forced){
  int state_value = random(0,101);
  if (state_value <= common_transition_probability) {
    transition = 1;     // common
  }
  else { 
    transition = 2;
    } // uncommon

    
  switch(turn){
    case 0:
        if (transition == 1) {   // If Left --> common = patternA
            //lcd.clear();
            //lcd.print("Common Transition");
            second_state = 0;
            patternA();
            tone(toneA_pin,patternA_tone_freq,second_step_length);
            return(second_state);
            break;
        }
        if (transition == 2) {  // If Left --> uncommon = patternB
            //lcd.print("Uncommon Transition");
            second_state = 1;
            patternB();
            tone(toneB_pin,patternB_tone_freq,second_step_length);
            return(second_state);
            break;
        }
    case 1:
        if (transition == 1) {   // If Right --> common = patternB
            //lcd.print("Common Transition");    
            second_state = 1;
            patternB();
            tone(toneB_pin,patternB_tone_freq,second_step_length);
            return(second_state);
            break;
        }
        if (transition == 2) {   // If Right --> uncommon = patternA
            //lcd.print("Uncommon Transition");
            second_state = 0;
            patternA();
            tone(toneA_pin,patternA_tone_freq,second_step_length);
            return(second_state);
            break;
        }
  }
}

void initializeToBlack() {
  for (int i = 0; i < 256; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void patternA() {
  initializeToBlack();
  int verts[160] = {
    2, 29, 34, 61, 66, 93, 98, 125, 130, 157, 162, 189, 194, 221, 226, 253,
    5, 26, 37, 58, 69, 90, 101, 122, 133, 154, 165, 186, 197, 218, 229, 250,
    8, 23, 40, 55, 72, 87, 104, 119, 136, 151, 168, 183, 200, 215, 232, 247,
    11, 20, 43, 52, 75, 84, 107, 116, 139, 148, 171, 180, 203, 212, 235, 244,
    14, 17, 46, 49, 78, 81, 110, 113, 142, 145, 174, 177, 206, 209, 238, 241};
  for (int j=0;j<160;j++){
    strip.setPixelColor(verts[j],white);
  }
  strip.show();
}

int printDirectory(File dir, int numTabs) {
  int ctt = 0;
  while (true) {
    File entry =  dir.openNextFile();
    ctt++;
    if (! entry) {
        return ctt;
        // no more files
        // return to the first file in the directory
        dir.rewindDirectory();
        break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      //Serial.print('\t');
    }
  }
}

void patternB() {
  initializeToBlack();
  strip.fill(white,16,16);
  strip.fill(white,64,16);
  strip.fill(white,112,16);
  strip.fill(white,160,16);
  strip.fill(white,208,16);
  strip.show();
}

int force_encoder(int forced, int side,int last_enc_val,int current_enc_val){
  if(forced==1){
    if(side==0){//need to turn left, encoder value must decrease
      if(last_enc_val<current_enc_val){//if the last enc value is smaller than current(value is increasing) write last_enc_val to encoder
        myEnc.write(last_enc_val+1);
        return last_enc_val;
      }
      else{
        myEnc.read();
        return current_enc_val;
      }
    }
    if(side==1){//need to turn left, encoder value must decrease
      if(last_enc_val>current_enc_val){//if the last enc value is smaller than current(value is increasing) write last_enc_val to encoder
        myEnc.write(last_enc_val-1);//for some reason, we need to have this +/- 1 so that it doesn't update the encoder value
        return last_enc_val;
      }
      else{
        myEnc.read();
        return current_enc_val;
      }
    }
  }
  else{
    myEnc.read();
    return current_enc_val;
  }
}
