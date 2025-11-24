#include <hFramework.h>
#include <DistanceSensor.h>
#include <Lego_Touch.h>

using namespace hModules;
using namespace hSensors;

// ----------------- Konfiguracja -----------------
const int ROBOT_SPEED = 300;                 // moc dla hMot1 (0–1000)
const int CANYON_DIST_THRESHOLD = 15;        // [mm] powyżej tej odległości uznajemy, że czujnik widzi "dziurę"

// hMot2 + hMot3 – most + podnoszenie/opuszczanie robota
const int BRIDGE_DOWN_POWER        = -900;   // kierunek: most w dół + robot w górę (dobierz znak)
const int BRIDGE_UP_POWER          = 900;    // przeciwny kierunek: most w górę + robot w dół

const uint32_t BRIDGE_ALIGN_STABLE_MS       = 500;   // (na razie nieużywane w tym kawałku)
const uint32_t BRIDGE_DOWN_ROBOT_UP_TIME_MS = 3000;  // (do dalszych kroków)
const uint32_t BRIDGE_UP_ROBOT_DOWN_TIME_MS = 2500;  // (do dalszych kroków)

// hMot4 – przejazd po moście
const int H4_ROT_TICKS      = 2000;   // ile ticków po moście – do kalibracji
const int H4_ROT_SPEED      = 400;    // „moc” hMot4 (0–1000)
const uint32_t WAIT_AFTER_H4_MS = 500; // przerwa po przejechaniu po moście

// ile ticków ma zjechać most w dół (relatywnie)
const int BRIDGE_TOUCH_TICKS = 13000;

// ----------------- Stany automatu -----------------
enum class State {
    WAIT_START,             // czekamy na wciśnięcie hBtn1
    SEARCH_CANYON,          // jedziemy do przodu, aż zobaczymy kanion (czujnik 3)
    BRIDGE_EXTEND,          // hMot4 do krańcówki
    BRIDGE_TOUCH,           // most w dół o zadaną wartość rel
    ROBOT_POSITION_CORECTION,
    WAIT_BRIDGE_ALIGN,
    BRIDGE_DOWN_ROBOT_UP,
    MOVE_ROBOT_ACROSS,
    BRIDGE_UP_ROBOT_DOWN,
    MOVE_BRIDGE_AHEAD
};

// ----------------- Funkcje pomocnicze -----------------

bool isCanyon(int dist)
{
    if (dist == 0) {
        // często 0 oznacza: brak echa / bardzo daleko
        return true;
    }

    if (dist > CANYON_DIST_THRESHOLD) {
        return true;
    }

    return false;
}

bool isDesk(int dist)
{
    return !isCanyon(dist);
}

static inline int absInt(int x) { return (x < 0) ? -x : x; }

void hMain()
{
    // Czujniki odległości:
    DistanceSensor sens1(hSens1); // 1: koniec mostu (nad biurkiem 2)
    DistanceSensor sens2(hSens2); // 2: koniec robota
    DistanceSensor sens3(hSens3); // 3: początek robota (wykrywanie początku kanionu)
    DistanceSensor sens4(hSens4); // 4: początek mostu (nad biurkiem 1)

    // Krańcówki na hSens5 i hSens6
    hLegoSensor_simple ls5(hSens5);
    hLegoSensor_simple ls6(hSens6);
    Lego_Touch limit5(ls5);
    Lego_Touch limit6(ls6);

    // Polaryzacja silników mostu
    hMot2.setEncoderPolarity(Polarity::Reversed);
    hMot2.setMotorPolarity(Polarity::Normal);

    hMot3.setEncoderPolarity(Polarity::Reversed);
    hMot3.setMotorPolarity(Polarity::Normal);

    hMot1.setEncoderPolarity(Polarity::Reversed);
    hMot1.setMotorPolarity(Polarity::Normal);

    // Start – zatrzymujemy wszystko
    hMot1.setPower(0);
    hMot2.setPower(0);
    hMot3.setPower(0);
    hMot4.setPower(0);

    State state = State::WAIT_START;

    uint64_t stateEntryTime = sys.getRefTime();
    bool hBtn1_flag = false;

    Serial.printf("START: automat kanionu.\r\n");
    Serial.printf("Wcisnij hBtn1 aby wystartowac sekwencje.\r\n");

    for (;;)
    {
        // --- Odczyt czujników odległości ---
        int d1 = sens1.getDistance();
        int d2 = sens2.getDistance();
        int d3 = sens3.getDistance();
        int d4 = sens4.getDistance();

        // --- Odczyt enkoderów wszystkich silników ---
        int e1 = hMot1.getEncoderCnt();
        int e2 = hMot2.getEncoderCnt();
        int e3 = hMot3.getEncoderCnt();
        int e4 = hMot4.getEncoderCnt();

        uint64_t now = sys.getRefTime();

        // Jeden wspólny log: czujniki + stan + enkodery
        Serial.printf(
            "d1=%d d2=%d d3=%d d4=%d  state=%d  enc1=%d enc2=%d enc3=%d enc4=%d\r\n",
            d1, d2, d3, d4, (int)state,
            e1, e2, e3, e4
        );

        switch (state)
        {
        case State::WAIT_START:
        {
            // wszystko stoi, czekamy na przycisk
            hMot1.setPower(0);
            hMot2.setPower(0);
            hMot3.setPower(0);
            hMot4.setPower(0);

            if (hBtn1.isPressed()){
                hBtn1_flag = true;
            }

            if (hBtn1_flag) {
                Serial.printf("hBtn1 pressed -> START automatu (SEARCH_CANYON)\r\n");
                bool pressed5 = limit5.isPressed();
                if (!pressed5) {
                    Serial.printf("Ustawienei początkowe mostu do krańcówki\r\n");
                    hMot4.setPower(-H4_ROT_SPEED);
                }
                else if (pressed5) {
                    hMot4.setPower(0);
                    hBtn1_flag = false;
                    state = State::SEARCH_CANYON;
                }
            }
            break;
        }

        case State::SEARCH_CANYON:
            Serial.printf("START SEARCH KANION\r\n");

            // --- KROK 1: jedziemy do przodu, aż czujnik 3 zobaczy kanion ---
            hMot1.setPower(ROBOT_SPEED);   // jeśli jedzie w złą stronę -> zmień znak

            if (isCanyon(d3)) {
                hMot1.setPower(0);
                Serial.printf("KROK 1: WYKRYTO KANION -> STOP ROBOT\r\n");

                // przejście do BRIDGE_EXTEND
                state = State::BRIDGE_EXTEND;
                stateEntryTime = now;
            }
            sys.delay(200); // mała pauza po wykryciu
            break;

        case State::BRIDGE_EXTEND:
        {
            // hMot4 wysuwa most, aż krańcówka na hSens6 zadziała
            hMot4.setPower(H4_ROT_SPEED);
            Serial.printf("BRIDGE_EXTEND: start, hMot4 power=%d\r\n", H4_ROT_SPEED);

            bool pressed6 = limit6.isPressed();

            Serial.printf("BRIDGE_EXTEND: limit6=%d\r\n", (int)pressed6);

            if (pressed6)
            {
                // Krańcówka zadziałała -> zatrzymujemy hMot4
                hMot4.setPower(0);

                Serial.printf("BRIDGE_EXTEND: limit hit -> BRIDGE_TOUCH\r\n");

                state = State::BRIDGE_TOUCH;
                stateEntryTime = now;
            }

            break;
        }

        case State::BRIDGE_TOUCH:
            // --- MOST W DÓŁ DO MOMENTU KIEDY LEKKO DOTKNIE BIUREK - relatywnie ---
            Serial.printf("BRIDGE_TOUCH:\r\n");

            hMot2.rotRel(BRIDGE_TOUCH_TICKS,  900,  false, INFINITE);
            hMot3.rotRel(BRIDGE_TOUCH_TICKS, 1000,  true, INFINITE);

            Serial.printf("BRIDGE_TOUCH: osiagnieto zadana pozycje -> ROBOT_POSITION_CORECTION\r\n");

            state = State::ROBOT_POSITION_CORECTION;
            stateEntryTime = now;
            break;

        case State::ROBOT_POSITION_CORECTION:
            // korekta pozycji robota głównego
            hMot1.rotRel(800, 350, true, INFINITE);
            Serial.printf("Pozycja robota głownego została poprawiona\r\n");
            state = State::BRIDGE_DOWN_ROBOT_UP;
            break;

        case State::BRIDGE_DOWN_ROBOT_UP:
            // most w dół + robot w górę
            hMot2.rotRel(13500,  900, false, INFINITE);
            hMot3.rotRel(13500, 1000, true, INFINITE);
            Serial.printf("Robot główny został podniesiony maksymalnie do góry\r\n");
            state = State::MOVE_ROBOT_ACROSS;
            break;

        case State::MOVE_ROBOT_ACROSS:
        {
            // robot jedzie po moście do krańcówki na hSens5
            bool pressed5 = limit5.isPressed();
            hMot4.setPower(-H4_ROT_SPEED);

            Serial.printf("MOVE_ROBOT_ACROSS: limit5=%d\r\n", (int)pressed5);

            if (pressed5) {
                hMot4.setPower(0);
                state = State::BRIDGE_UP_ROBOT_DOWN;
                stateEntryTime = now;
                Serial.printf("MOVE_ROBOT_ACROSS: limit hit -> BRIDGE_UP_ROBOT_DOWN\r\n");
            }
            break;
        }

        case State::BRIDGE_UP_ROBOT_DOWN:
            // most w górę + robot w dół
            hMot2.rotRel(-25400, 1000, false, INFINITE);
            hMot3.rotRel(-25400, 1000, true, INFINITE);
            state = State::MOVE_BRIDGE_AHEAD;
            stateEntryTime = now;
            Serial.printf("BRIDGE_UP_ROBOT_DOWN: koniec cyklu -> WAIT_START\r\n");
            break;

        case State::MOVE_BRIDGE_AHEAD:
        {
            bool pressed6 = limit6.isPressed();
            hMot4.setPower(H4_ROT_SPEED);
            if(pressed6) {
                hMot4.setPower(0);
                state = State::WAIT_START;
            }
            break;
        }
        }

        sys.delay(20);
    }
}
