#include <Joystick.h>
#define FILTER_SIZE 6

// Set this to 1 to show the raw values for calibration
// Set this to 0 for normal use
#define USE_SERIAL 0

// Digital Filter
int xBuffer[FILTER_SIZE], yBuffer[FILTER_SIZE], rBuffer[FILTER_SIZE];
int xIndex = 0, yIndex = 0, rIndex = 0;

int smoothAnalog(int newValue, int *buffer, int &index)
{
  buffer[index] = newValue;
  index = (index + 1) % FILTER_SIZE;
  long sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++)
  {
    sum += buffer[i];
  }
  return sum / FILTER_SIZE;
}
int started_ok = 0;

// X/Y/R: provide start (zero reference) and full range (span).
long X_START = 250;
long X_RANGE = 320;

long Y_START = 80;
long Y_RANGE = 320;

long R_CENTER = 500;
long R_RANGE = 250;
// --- end calibration parameters ---

// Startup calibration readings
int Startup = 10;

Joystick_ Joystick(0x04, JOYSTICK_TYPE_JOYSTICK, 0, 0, true, true, false, false, false, false, true, false, false, false, false);

void setup()
{

#if USE_SERIAL
  Serial.begin(115200);
  while (!Serial)
    delay(10);
#endif

  // --- Startup calibration: sample each analog axis 'Startup' times and
  // set the X_START, Y_START and R_CENTER to the averaged readings.
  long sumX = 0;
  long sumY = 0;
  long sumR = 0;
  for (int i = 0; i < Startup; i++)
  {
    sumX += analogRead(A0);
    sumY += analogRead(A1);
    sumR += analogRead(A3);
    delay(10);
  }
  long avgX = sumX / Startup;
  long avgY = sumY / Startup;
  long avgR = sumR / Startup;

  long HEADROOM = 10;
  X_START = avgX + HEADROOM - X_RANGE;
  Y_START = avgY - HEADROOM;
  R_CENTER = avgR;

  // Initialize the digital filter buffers to the startup readings to avoid
  // artifacts from uninitialized buffer entries.
  for (int i = 0; i < FILTER_SIZE; i++)
  {
    xBuffer[i] = avgX;
    yBuffer[i] = avgY;
    rBuffer[i] = avgR;
  }
  xIndex = yIndex = rIndex = 0;

#if USE_SERIAL
  Serial.print("Startup calib X_START=");
  Serial.print(X_START);
  Serial.print(" Y_START=");
  Serial.print(Y_START);
  Serial.print(" R_CENTER=");
  Serial.println(R_CENTER);
#endif

  Joystick.setXAxisRange(-512, 512);
  Joystick.setYAxisRange(-512, 512);
  Joystick.setRudderRange(-512, 512);
  Joystick.begin();
}

void loop()
{

  int x = smoothAnalog(analogRead(A0), xBuffer, xIndex);

  // Adjust X calibration if reading falls outside configured start/range
  if (x < X_START)
  {
    long delta = X_START - x;
    X_START = x;
    X_RANGE += delta;
  }
  else if (x > X_START + X_RANGE)
  {
    X_RANGE = x - X_START;
  }

  int xAxis_ = map(x, X_START, X_START + X_RANGE, 512, -512);
  Joystick.setXAxis(xAxis_);

  int y = smoothAnalog(analogRead(A1), yBuffer, yIndex);

  // Adjust Y calibration if reading falls outside configured start/range
  if (y < Y_START)
  {
    long delta = Y_START - y;
    Y_START = y;
    Y_RANGE += delta;
  }
  else if (y > Y_START + Y_RANGE)
  {
    Y_RANGE = y - Y_START;
  }

  int yAxis_ = map(y, Y_START, Y_START + Y_RANGE, -512, 512) * -1;
  Joystick.setYAxis(yAxis_);

  int r = smoothAnalog(analogRead(A3), rBuffer, rIndex);

  // Adjust R (rudder) calibration so center/range include the new reading
  long rMin = R_CENTER - R_RANGE;
  long rMax = R_CENTER + R_RANGE;
  if (r < rMin)
  {
    // expand lower side
    R_RANGE = R_CENTER - r;
  }
  else if (r > rMax)
  {
    // expand upper side
    R_RANGE = r - R_CENTER;
  }

  int rAxis_ = map(r, R_CENTER - R_RANGE, R_CENTER + R_RANGE, -512, 512);
  Joystick.setRudder(rAxis_);

#if USE_SERIAL && 0
  Serial.print("x= ");
  Serial.print(x); // find X max min
  Serial.print(" ; y= ");
  Serial.print(y); // find Y max min
  Serial.print(" ; r= ");
  Serial.print(r); // find Rx max min
  Serial.println("");
#endif

  delay(5);
}
