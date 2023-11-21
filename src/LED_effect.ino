void solid()
{
    // 普通の光
    fill_solid(leds, NUM_LEDS, currentColor(gVariation));
}

void round()
{
    // 回転
    static uint8_t r = 0;

    fadeToBlackBy(leds, NUM_LEDS, 255);
    leds[r / 3] = currentColor(gVariation+1);
    leds[r / 3 + 34] = leds[r / 3];
    for (uint8_t i = 0; i < 4; ++i)
    {
        leds[i * 6 + r / 5 + 10] = leds[r / 3];
    }
    r = ((r + 1) % 30);
}

void rainbow()
{
    // LEDが1周6個なので256/6でDeltaHueは43
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
    uint8_t chanceOfFlash = 64;

    fadeToBlackBy(leds, NUM_LEDS, 50);

    if (random8() < chanceOfFlash)
    {
        leds[random8(NUM_LEDS)] = currentColor(gVariation+7);
    }
}
void gradation()
{
    static uint8_t r = 0;
    CRGBPalette16 currentPalette;

    switch (gVariation % 8)
    {
    case 0:
        currentPalette = OceanColors_p;
        break; // blue and white
    case 1:
        currentPalette = ForestColors_p;
        break; // orange, red, black and yellow),
    case 2:
        currentPalette = LavaColors_p;
        break; // blue, cyan and white
    case 3:
        currentPalette = PartyColors_p;
        break; // greens and blues
    case 4:
        currentPalette = RainbowColors_p;
        break; // standard rainbow animation
    case 5:
        currentPalette = RainbowStripeColors_p;
        break; // single colour, black space, next colour and so forth
    case 6:
        currentPalette = CloudColors_p;
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
    if (r < NUM_LEDS)
    {
        leds[r] = currentColor(gVariation);
    }
    else
    {
        leds[NUM_LEDS * 2 - r - 1] = currentColor(gVariation);
    }
    r = (r + 1) % (NUM_LEDS * 2);
}

CRGB currentColor(uint8_t variation)
{
    switch (variation % 8)
    {
    case 0:
        return CRGB::Blue;
    case 1:
        return CRGB::Red;
    case 2:
        return CRGB::Green;
    case 3:
        return CRGB::Yellow;
    case 4:
        return CRGB::Purple;
    case 5:
        return CRGB::Cyan;
    case 6:
        return CRGB::HotPink;
    case 7:
        return CRGB::White;
    }
}

void explosion(void)
{
    static uint16_t sStep = 0;
    CRGBPalette16 currentPalette;
    if (sStep == 0)
    {
        while (!digitalRead(LOWER_SW_PIN))
        {
            gDirection = 1;
            delay(1);
        }
        gDirection = 0;
    }
    else if (sStep < 100)
    {
        fadeToBlackBy(leds, NUM_LEDS, 50);
    }
    else if (sStep < 170)
    {
        uint8_t chanceOfFlash = sStep * 2;

        fadeToBlackBy(leds, NUM_LEDS, 50);

        if (random8() < chanceOfFlash)
        {
            leds[random8(NUM_LEDS)] = CRGB::White;
        }
    }
    else if (sStep < 177)
    {
        uint8_t chanceOfFlash = 200;

        if (digitalRead(UPPER_SW_PIN))
        {
            gDirection = 0;
        }
        else
        {
            gDirection = -1;
        }

        fadeToBlackBy(leds, NUM_LEDS, 10);

        if (random8() < chanceOfFlash)
        {
            leds[random8(NUM_LEDS)] = CRGB::White;
        }
    }
    else if (sStep < 280)
    {
        uint8_t chanceOfFlash = 200;

        gDirection = 0;
        fadeToBlackBy(leds, NUM_LEDS, 5);

        if (random8() < chanceOfFlash)
        {
            leds[random8(NUM_LEDS)] = CRGB::White;
        }
    }
    else if (sStep < 400)
    {
        if (digitalRead(UPPER_SW_PIN))
        {
            gDirection = 0;
        }
        else
        {
            gDirection = -1;
        }
        if (random8(20))
        {
            currentPalette = LavaColors_p;
            for (uint16_t i = 0; i < NUM_LEDS; ++i)
            {
                leds[i] = ColorFromPalette(currentPalette, (uint8_t)((i + sStep + 100) % 256), 255);
            }
        }
        else
        {
            fill_solid(leds, NUM_LEDS, CRGB::Black);
        }
    }
    else if (sStep < 655)
    {
        currentPalette = HeatColors_p;

        for (uint16_t i = 0; i < NUM_LEDS; ++i)
        {
            leds[i] = ColorFromPalette(currentPalette, (uint8_t)((655 - sStep) % 256), 255);
        }
    }
    else if (sStep < 1000)
    {
        uint8_t chanceOfFlash = 200;

        if (random8() < chanceOfFlash)
        {
            leds[random8(NUM_LEDS)] -= CRGB(1, 0, 0);
        }
    }
    else
    {
        sStep = 0;
        gDirection = 0;
        gCurrentPatternNumber = 0;
        return;
    }
    sStep++;
}
