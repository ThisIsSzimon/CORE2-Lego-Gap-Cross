#include <hFramework.h>
#include <DistanceSensor.h>

using namespace hModules;

void hMain()
{
    hExt.serial.init(115200);
    char c;
    while (1)
    {
        if (hExt.serial.available() > 0) { // checking Serial availability
            c = hExt.serial.getch();
            if(c == 'a') {
                hLED1.on();
                hLED2.off();
                hLED3.off();
                hMot1.setPower(500);
            }
            if(c == 's') {
                hLED1.off();
                hLED2.on();
                hLED3.off();
                hMot1.setPower(0);
            }
            if(c == 'd') {
                hLED1.off();
                hLED2.off();
                hLED3.on();
                hMot1.setPower(-500);
            }
            if(c == 'z'){
                hMot2.setPower(1000);
                hMot3.setPower(1000);

            }
            if(c == 'x'){
                hMot2.setPower(0);
                hMot3.setPower(0);
            }
            if(c == 'c'){
                hMot2.setPower(-1000);
                hMot3.setPower(-1000);
            }
        }
    }
}