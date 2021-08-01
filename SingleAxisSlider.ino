/*Stuff for lcd*/
#define LCD_COL 20
#define LCD_ROW 4

/*Stuff for KY-040 encoder*/
#define EN_DT 2 //for knob input
#define EN_SW 3 //for push button input
#define EN_CLK 4 //reference for encoder signal
char key = ' '; //holds the last user input

/*Stuff for length stepper*/
#include <Stepper.h>
#define LEN_EN 5 //pin for enabling the stepper
#define LEN_STEP 6 //pin for moving the stepper
#define LEN_DIR 7 //pin for stepper direction
int LEN_STEPS = 200; //steps per revolution
int microSteps = 8; //microsteps per step
long LEN_MMtoSteps = 50.79; //for conversions, number of steps to move 1 mm
long LEN_StepstoMM = 0.019; //for conversions, number of mms in one step
long LEN_POS = 0; //holds the current position of the slider
long LEN_Speed = 30; //the sepped of the motor, rpm
Stepper LEN_Stepper = Stepper(LEN_STEPS, LEN_STEP, LEN_DIR); //the stepper object

/*motion parameters*/
//initalize with default parameters
int lenStart = 0; //starting pos [mm]
int lenStop = 935; //stop pos [mm]
int delayTime = 2; //time to move to start pos [s]
int intTime = 2; //time between shots [s]
int expTime = 100; //exposure time [ms]
int n = 5; //number of pictures [#]
int hypeTime = 30; //total time for hyperlapse

/*Stuff for Menus and navigation*/
#include <LcdMenu.h>
extern MenuItem mainMenu[];
extern MenuItem timelapse[];
extern MenuItem hyperlapse[];
extern MenuItem settings[];

//Declare Menu callback commands
void commandBack();
void startTimelapse();
void getStartPosInput();
void getStopPosInput();
void getDelayTimeInput();
void getExpTimeInput();
void getIntervalTimeInput();
void getNumCountInput();
void startHyperlapse();
void getHypeTimeInput();
void toggleBacklight();
void toggleSteppers();

//Initialize the menu structure
MenuItem mainMenu[] = {ItemHeader(),
                       ItemSubMenu("TimeLapse", timelapse),
                       ItemSubMenu("HyperLapse", hyperlapse),
                       ItemSubMenu("Settings", settings),
                       ItemFooter()
                      };
MenuItem timelapse[] = {ItemHeader(mainMenu),
                        ItemCommand("Begin", startTimelapse),
                        ItemInput("Start Pos [mm]", String(lenStart), getStartPosInput),
                        ItemInput("Stop Pos [mm]", String(lenStop), getStopPosInput),
                        ItemInput("Delay [s]", String(delayTime), getDelayTimeInput),
                        ItemInput("Interval [s]", String(intTime), getIntervalTimeInput),
                        ItemInput("Exp Time [ms]", String(expTime), getExpTimeInput),
                        ItemInput("Num [#]", String(n), getNumCountInput),
                        ItemCommand("Back", commandBack),
                        ItemFooter()
                       };
MenuItem hyperlapse[] = {ItemHeader(mainMenu),
                         ItemCommand("Begin", startHyperlapse),
                         ItemInput("Start Pos [mm]", String(lenStart), getStartPosInput),
                         ItemInput("Stop Pos [mm]", String(lenStop), getStopPosInput),
                         ItemInput("Delay [s]", String(delayTime), getDelayTimeInput),
                         ItemInput("Time [s]", String(hypeTime), getHypeTimeInput),
                         ItemCommand("Back", commandBack),
                         ItemFooter()
                        };
MenuItem settings[] = {ItemHeader(mainMenu),
                       ItemToggle("Backlight", toggleBacklight),
                       ItemToggle("Toggl Steppers", "OFF", "ON", toggleSteppers),
                       ItemCommand("Back", commandBack),
                       ItemFooter()
                      };
//Construct the LcdMenu
LcdMenu menu(LCD_ROW, LCD_COL);

/*shutter pin*/
#define Shutter 8

void setup()
{
  //Setup for encoder
  pinMode(EN_SW, INPUT_PULLUP);
  pinMode(EN_DT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(EN_SW), pushButton, LOW);
  attachInterrupt(digitalPinToInterrupt(EN_DT), rotate, LOW);

  //add LcdMenu to the menu object
  menu.setupLcdWithMenu(0x27, mainMenu);

  //Setup for testing tools
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Serial.begin(9600);

  //Setup steppers
  pinMode(LEN_EN, OUTPUT);
  pinMode(LEN_STEP, OUTPUT);
  pinMode(LEN_DIR, OUTPUT);
  digitalWrite(LEN_DIR, LOW);
  digitalWrite(LEN_EN, HIGH);

  //setup for shutter pin
  pinMode(Shutter, OUTPUT);
  digitalWrite(Shutter, HIGH);
}

void loop()
{
  //checks the input, manipulates the menu, clears the input
  switch (key)
  {
    case 'D':
      key = ' ';
      menu.down();
      break;
    case 'U':
      key = ' ';
      menu.up();
      break;
    case 'E':
      key = ' ';
      menu.enter();
      break;
    default:
      key = ' ';
      break;
  }
  delay(10);
}

/*Interupt funtctions for user input*/
void rotate()
{
  //detects rotary input with software debounce
  static unsigned long lastRotCall = 0;
  unsigned long currentRotCall = millis();
  if (currentRotCall - lastRotCall > 10)
  {
    if (digitalRead(EN_CLK))
    {
      key = 'U';
    }
    else
    {
      key = 'D';
    }
  }
  lastRotCall = currentRotCall;
}

void pushButton()
{
  //detects button input with software debounce
  static unsigned long lastBtnCall = 0;
  unsigned long currentBtnCall = millis();
  if (currentBtnCall - lastBtnCall > 10)
  {
    key = 'E';
  }
  lastBtnCall = currentBtnCall;
}

/*helper for user input*/
int getCountInput(int current)
{
  //until button is pressed, ++ or -- the given count
  //first by 100's, then by 10's, then by 1's
  bool hundreds = true;
  bool tens = true;
  bool ones = true;
  int count = current;
  while (ones)
  {
    //check the input, manipulates the menu, clears the input
    switch (key)
    {
      case 'D':
        key = ' ';
        if (hundreds)
          count = count + 100;
        else if (tens)
          count = count + 10;
        else
          count++;
        menu.clear();
        menu.type(String(count));
        break;
      case 'U':
        key = ' ';
        if (hundreds)
          count = count - 100;
        else if (tens)
          count = count - 10;
        else
          count--;
        menu.clear();
        menu.type(String(count));
        break;
      case 'E':
        key = ' ';
        if (hundreds)
          hundreds = false;
        else if (tens)
          tens = false;
        else
          ones = false;
        break;
      default:
        key = ' ';
        break;
    }
    if (!ones)
      break;
  }
  return count;
}

/*helper functions*/
void enableSteppers()
{
  digitalWrite(13, HIGH);
  digitalWrite(LEN_EN, LOW);
}

void disableSteppers()
{
  digitalWrite(13, LOW);
  digitalWrite(LEN_EN, HIGH);
}

void takePicture()
{
  digitalWrite(Shutter, LOW);
  delay(expTime);
  digitalWrite(Shutter, HIGH);
}

/*movement functions*/
void moveLen(long stepCount)
{
  Serial.print("LenPos:\t");
  Serial.println(LEN_POS);
  //step() takes an int, which has a max value
  //if the function is alled wwith a number greater than that, it moves the other way
  //this handles when input number is too large
  while (abs(stepCount) > 32767)
  {
    if (stepCount > 0)
    {
      LEN_Stepper.step(32767);
      stepCount = stepCount - 32767;
    }
    else
    {
      LEN_Stepper.step(-32767);
      stepCount = stepCount + 32767;
    }
  }
  LEN_Stepper.step(int(stepCount));
}

/*Menu Callback commands*/
void commandBack()
{
  menu.back();
}

void startTimelapse()
{
  enableSteppers();
  //goto start location
  long stepCount = (lenStart - LEN_POS) * LEN_MMtoSteps;
  LEN_Stepper.setSpeed(abs(stepCount / delayTime * LEN_MMtoSteps / 200 * 1.195));
  moveLen(stepCount);
  LEN_POS = lenStart;
  Serial.print("LenPos:\t");
  Serial.println(LEN_POS);
  delay(delayTime);
  takePicture();

  //begin time lapse exectuion
  stepCount = (lenStop - lenStart) / n * LEN_MMtoSteps;
  LEN_Stepper.setSpeed(abs(stepCount / intTime * LEN_MMtoSteps / 200 * 1.195));
  Serial.println("Begining Timelapse");
  for (int i = 0; i < n; i++)
  {
    Serial.print("LenPos:\t");
    Serial.println(LEN_POS);
    moveLen(stepCount);
    LEN_POS = LEN_POS + (lenStop - lenStart) / n;
    takePicture();
    delay(intTime * 1000);
  }
  Serial.print("LenPos:\t");
  Serial.println(LEN_POS);
  Serial.println("End of Timelapse");

  disableSteppers();
}

void getStartPosInput()
{
  lenStart = getCountInput(lenStart);
}

void getStopPosInput()
{
  lenStop = getCountInput(lenStop);
}

void getDelayTimeInput()
{
  delayTime = getCountInput(delayTime);
}

void getIntervalTimeInput()
{
  intTime = getCountInput(intTime);
}

void getExpTimeInput()
{
  expTime = getCountInput(expTime);
}

void getNumCountInput()
{
  n = getCountInput(n);
}

void getHypeTimeInput()
{
  hypeTime = getCountInput(hypeTime);
}

void startHyperlapse()
{
  enableSteppers();

  //goto start location
  long stepCount = (lenStart - LEN_POS) * LEN_MMtoSteps;
  LEN_Stepper.setSpeed(abs(stepCount / delayTime * LEN_MMtoSteps / 200 * 1.195));
  Serial.println("Moving to starting location");
  moveLen(stepCount);
  LEN_POS = lenStart;
  Serial.print("LenPos:\t");
  Serial.println(LEN_POS);
  delay(delayTime*1000);

  stepCount = (lenStop - lenStart) * LEN_MMtoSteps;
  LEN_Stepper.setSpeed(abs(stepCount / hypeTime * 50.7936 / 200 * 1.196));

  Serial.println("Begining Hyperlapse");
  moveLen(stepCount);
  Serial.println("End of Hyperlapse");

  disableSteppers();
}

void toggleBacklight()
{
  menu.lcd->setBacklight(menu.getItemAt(menu.getCursorPosition())->isOn);
}

void toggleSteppers()
{
  digitalWrite(LEN_EN, !digitalRead(LEN_EN));
}
