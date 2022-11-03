
void solid()
{
    // 普通の光

    fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
}

void round()
{
    // 回転
    static uint8_t r = 0;

    fadeToBlackBy(leds, NUM_LEDS, 255);
    for (uint8_t i = 0; i < 4; ++i)
    {
        leds[i * 6 + r / 5 + 10] = CHSV(gHue, 255, 255);
    }
    leds[r / 3] = CHSV(gHue, 255, 255);
    leds[r / 3 + 34] = CHSV(gHue, 255, 255);
    r = ((r + 1) % 30);
}

void rainbow()
{
    // 回転
    static uint8_t r = 0;
    fill_rainbow(leds, NUM_LEDS, r, 43);

    r++;
}

void rotateColor()
{
    static uint8_t sHue = 0;
    fill_solid(leds, NUM_LEDS, CHSV(sHue, 255, 255));
    sHue++;
}

void flash()
{
    // チカチカ
    static uint8_t sChanceOfFlash = 64;

    fadeToBlackBy(leds, NUM_LEDS, 50);

    if (random8() < sChanceOfFlash)
    {
        leds[random8(NUM_LEDS)] += CRGB::White;
    }
}
void gradation()
{
    static uint8_t sPattern = 4;
    static uint8_t r = 0;
    //    sPattern = (sPattern + getDeltaY(1) + 8) % 8; // 0~7(Rotateあり）

    CRGBPalette16 currentPalette;
    switch (sPattern)
    {
    case 0:
        currentPalette = CloudColors_p;
        break; // blue and white
    case 1:
        currentPalette = LavaColors_p;
        break; // orange, red, black and yellow),
    case 2:
        currentPalette = OceanColors_p;
        break; // blue, cyan and white
    case 3:
        currentPalette = ForestColors_p;
        break; // greens and blues
    case 4:
        currentPalette = RainbowColors_p;
        break; // standard rainbow animation
    case 5:
        currentPalette = RainbowStripeColors_p;
        break; // single colour, black space, next colour and so forth
    case 6:
        currentPalette = PartyColors_p;
        break; // red, yellow, orange, purple and blue
    case 7:
        currentPalette = HeatColors_p;
        break; // red, orange, yellow and white
    }
    for (uint16_t i = 0; i < NUM_LEDS; ++i)
    {
        leds[i] = ColorFromPalette(currentPalette, i + r, 255);
    }
    r++;
}
void run()
{
    static uint8_t r = 0;
    fadeToBlackBy(leds, NUM_LEDS, 100);
    leds[r/2] = CRGB::White;
    r = (r + 1) % (NUM_LEDS*2);
}