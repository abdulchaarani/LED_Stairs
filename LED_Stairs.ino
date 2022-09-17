#include <FastLED.h>

#define DATA_PIN 13
#define TRIG_PIN_UP 11
#define ECHO_PIN_UP 9
#define TRIG_PIN_DOWN 7
#define ECHO_PIN_DOWN 6
#define NUM_LEDS 300

double wall_distance_up;
double wall_distance_down;
// Define the array of leds
CRGB leds[NUM_LEDS];

double getDistance(int trig_pin, int echo_pin) {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  const int duration = pulseIn(echo_pin, HIGH);
  return duration * 0.034 / 2;
}

double getDistanceUP() {
  return getDistance(TRIG_PIN_UP, ECHO_PIN_UP);
}

double getDistanceDOWN() {
  return getDistance(TRIG_PIN_DOWN, ECHO_PIN_DOWN);
}

double getWallDistance(int trig_pin, int echo_pin) {
  double wall_distance = 0;
  static const int n_iterations = 100;
  for (int i = 0; i < n_iterations; ++i)
  {
    wall_distance += getDistance(trig_pin, echo_pin);
  }
  return wall_distance / n_iterations;
}


void setup() {
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  pinMode(TRIG_PIN_UP, OUTPUT);
  pinMode(ECHO_PIN_UP, INPUT);
  pinMode(TRIG_PIN_DOWN, OUTPUT);
  pinMode(ECHO_PIN_DOWN, INPUT);
  Serial.println("Getting UP wall distance");
  wall_distance_up = getWallDistance(TRIG_PIN_UP, ECHO_PIN_UP);
  Serial.println(wall_distance_up);
  Serial.println("Getting DOWN wall distance");
  wall_distance_down = getWallDistance(TRIG_PIN_DOWN, ECHO_PIN_DOWN);
  Serial.println(wall_distance_down);
}

enum class State {
  STANDBY,
  LIGHTSHOW,
  HOLD
};

bool check(double wall_distance, double distance, int &counter)
{
  static const int TRIGGER = 3;
  static const double EPSILON = 20;
  if (distance < wall_distance - EPSILON) {
    ++counter;
  } else {
    counter = 0;
  }
  return counter >= TRIGGER;
}

bool checkUP()
{
  static int counter = 0;
  const double distance = getDistanceUP();
  Serial.print("Distance Up: ");
  Serial.println(distance);
  return check(wall_distance_up, distance, counter);
}

bool checkDOWN()
{
  static int counter = 0;
  const double distance = getDistanceDOWN();
  Serial.print("Distance Down: ");
  Serial.println(distance);
  return check(wall_distance_down, distance, counter);
}

void addGlitter(fract8 chanceOfGlitter)
{
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] = CRGB::White;
  }
}

void loop() {
  static State state = State::STANDBY;
  static double start_time = 0;
  static int it = 0;
  static int i;
  const bool upIsOn = checkUP();
  const bool downIsOn = checkDOWN();

  if (state == State::STANDBY) {
    if (upIsOn) {
      state = State::LIGHTSHOW;
      it = 1;
      i = 0;
      start_time = millis();
    } else if (downIsOn) {
      state = State::LIGHTSHOW;
      it = -1;
      i = NUM_LEDS - 1;
      start_time = millis();
    }
  }

  if (state == State::LIGHTSHOW) {
    static const double DURATION = 2000;
    static int j = 0;
    const double cur_time = millis();
    const double increment = NUM_LEDS * (cur_time - start_time) / DURATION;
    for (; i >= 0 && i < NUM_LEDS && j < increment; i += it, ++j) {
      leds[i] = CRGB::Green;
      fill_rainbow(&leds[i], 1, i, 4);
      addGlitter(80);
    }
    if (cur_time - start_time >= DURATION) {
      start_time = cur_time;
      state = State::HOLD;
      j = 0;
    }
  }
  if (state == State::HOLD) {
    addGlitter(80);
    static const double DURATION = 6000;
    const double cur_time = millis();
    if (cur_time - start_time >= DURATION) {
      for (int j = 0; j < NUM_LEDS; ++j) {
        leds[j] = CRGB::Black;
      }
      state = State::STANDBY;
    }
  }

  FastLED.show();
}
