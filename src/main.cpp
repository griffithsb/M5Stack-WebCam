#include <M5Unified.h>
#include "WebCam.h"

Webcam webcam;

void setup()
{
    webcam.begin();
}

void loop()
{
    webcam.update();
}