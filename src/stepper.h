// STEPPER_STEP has to be 16.
#define STEPPER_STEP 16
#define STEPPER_DIR 15
#define STEPPER_RST 5
#define STEPPER_FAULT 12

#define STEPPER_MAX_SPEED 500
#define STEPPER_MIN_START_SPEED 30000
#define STEPPER_MIN_SPEED 2500000

#define StepperMode_OFF 0
#define StepperMode_Const 1
#define StepperMode_Step 2
#define StepperMode_Wipe 3

volatile IRAM_ATTR uint32_t startingSteps = 1600;
volatile IRAM_ATTR int32_t pendingSteps = 1600;
volatile IRAM_ATTR uint32_t stepperMode = StepperMode_OFF;
volatile IRAM_ATTR uint32_t startingTicks = 30000;
volatile IRAM_ATTR uint32_t pendingTicks = 30000;
volatile IRAM_ATTR uint32_t realTicks = 30000;
volatile IRAM_ATTR uint32_t faultStatus = 0;

void IRAM_ATTR stepperEnable() {
    GPOS = (1 << STEPPER_RST);  // digitalWrite(STEPPER_SLPRST, 1);
    GP16O &= ~1;                // clear GPIO16
    webSocket.broadcastTXT("e1");
}
void IRAM_ATTR stepperDisable() {
    GPOC = (1 << STEPPER_RST);  // digitalWrite(STEPPER_SLPRST, 0);
    GP16O &= ~1;                // clear GPIO16
    webSocket.broadcastTXT("e0");
}
void IRAM_ATTR stepperTimerStart() {
    if (!(TEIE & TEIE1)) {
        TEIE |= TEIE1;  // timer1 edge int enable
        GP16O &= ~1;    // clear GPIO16
        webSocket.broadcastTXT("t1");
    }
}
void IRAM_ATTR stepperTimerStop() {
    if ((TEIE & TEIE1)) {
        TEIE &= ~TEIE1;  // timer1 edge int disable
        GP16O &= ~1;     // clear GPIO16
        webSocket.broadcastTXT("t0");
    }
}

// immediate stop
void stepperStop() {
    stepperTimerStop();  // timer1 edge int disable
    stepperMode = StepperMode_OFF;
    realTicks = STEPPER_MIN_START_SPEED;
    pendingSteps = startingSteps;  // refresh steps
}

void IRAM_ATTR stepperSetDir(uint32_t dir) {
    if (dir) {
        GPOS = (1 << STEPPER_DIR);  // digitalWrite(STEPPER_DIR, 1);
        webSocket.broadcastTXT("d1");
    } else {
        GPOC = (1 << STEPPER_DIR);  // digitalWrite(STEPPER_DIR, 0);
        webSocket.broadcastTXT("d0");
    }
}
void IRAM_ATTR onTimer1ISR(void *para, void *frame) {
    (void)para;
    (void)frame;
    uint32_t savedPS = xt_rsil(15);  // stop other interrupts
    T1I = 0;                         // timer1 clear interrupt
    if (GP16I & 1) {                 // GPIO16 is high
        GP16O &= ~1;                 // clear GPIO16

        // ramp up
        if (realTicks > pendingTicks) {
            realTicks *= (double)0.995;
            // accelSteps++;
            if (realTicks < pendingTicks) realTicks = pendingTicks;  // correct overshoot
        }
        uint8_t setTimer = 1;
        if ((stepperMode & 2) == 2) {  // stepperMode 2 or 3, Step or Wipe
            pendingSteps--;
            if (pendingSteps <= 0) {  // steps has expired
                if (stepperMode == StepperMode_Step) {
                    stepperStop();
                    setTimer = 0;
                } else {                           // StepperMode_Wipe
                    pendingSteps = startingSteps;  // refresh steps
                    pendingTicks = startingTicks;  // set target speed

                    if (pendingTicks < STEPPER_MIN_START_SPEED) {
                        realTicks = STEPPER_MIN_START_SPEED;
                    } else {
                        realTicks = pendingTicks;
                    }
                    setTimer = 0;                        // dirty little hack for wipe rebound
                    T1L = STEPPER_MIN_SPEED & 0x7FFFFF;  // timer1 set time
                    stepperSetDir(!GPIP(STEPPER_DIR));   // reverse direction
                }
            }
        }
        if (setTimer)
            T1L = realTicks & 0x7FFFFF;  // timer1 set time
    } else {
        GP16O |= 1;                  // set GPIO16
        T1L = realTicks & 0x7FFFFF;  // timer1 set time
    }

    xt_wsr_ps(savedPS);
}

void IRAM_ATTR stepperFaultISR() {
    faultStatus = 1;
    stepperTimerStop();
    stepperDisable();
}
uint8_t rssiCount = 0;
void stepperCheckFault() {
    if (faultStatus) {
        if (GPIP(STEPPER_FAULT)) {
            webSocket.broadcastTXT("f1");
        } else {
            webSocket.broadcastTXT("f0");
        }
        faultStatus = 0;
    }
    if (++rssiCount == 80) {
        rssiCount = 0;
        String payload = "-" + String(WiFi.RSSI());
        webSocket.broadcastTXT(payload);
    }
}
void stepperMove(uint32_t mode = StepperMode_Const) {
    stepperMode = mode;
    T1L = realTicks & 0x7FFFFF;  // timer1 set time
    stepperTimerStart();         // timer1 edge int enable
}

void stepper_h_init() {
    ETS_FRC_TIMER1_INTR_ATTACH(onTimer1ISR, NULL);
    T1C = (1 << TCTE) |  // Timer Enable
          (0 << TCPD) |  // Prescale Divider (2bit) 0:1(12.5ns/tick), 1:16(0.2us/tick), 2/3:256(3.2us/tick)
          (0 << TCIT) |  // Interrupt Type 0:edge, 1:level
          (1 << TCAR);   // AutoReload (restart timer when condition is reached)
    T1I = 0;
    ets_isr_unmask(1 << ETS_FRC_TIMER1_INUM);

    pinMode(16, OUTPUT);
    pinMode(STEPPER_DIR, OUTPUT);
    pinMode(STEPPER_RST, OUTPUT);
    pinMode(STEPPER_FAULT, INPUT);

    GP16O &= ~1;                // clear GPIO 16
    GPOC = (1 << STEPPER_RST);  // digitalWrite(STEPPER_RST, 0);

    attachInterrupt(digitalPinToInterrupt(STEPPER_FAULT), stepperFaultISR, CHANGE);
}