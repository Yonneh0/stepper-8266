

void timer0_set(uint32_t newValue) {
}

void IRAM_ATTR onTimer0ISR(void *para, void *frame) {
    (void)para;
    (void)frame;
    uint32_t savedPS = xt_rsil(15);  // stop other interrupts

    xt_wsr_ps(savedPS);
}
void timer0_init() {
    ETS_CCOMPARE0_INTR_ATTACH(onTimer0ISR, NULL);
}