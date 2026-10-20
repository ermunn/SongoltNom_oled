#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// ==================== ДЭЛГЭЦ ====================

constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;

U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);

// ==================== ТЕКСТҮҮД ====================

const char* TITLE1 = "Бодол";
const char* TITLE2 = "Сэтгэл хөдлөл";
const char* TITLE3 = "Бүтэлгүйтвэл";
const char* TITLE4 = "Муу зүйл";

const char* TITLE5_FIRST = "Мэдрэмж";
const char* TITLE5_FULL = "Мэдрэмж биш";

const char* TITLE6_TOP = "Бодол";
const char* TITLE6_MIDDLE = "Төлөвлөгөө";
const char* TITLE6_BOTTOM = "Таамаглал";

const char* TITLE7_FIRST = "Сэтгэл хөдлөл";
const char* TITLE7_SECOND = "Мэдрэмж";

const char* TITLE8_WORDS[] = {
  "Тайлбарлах",
  "Засах",
  "Шүүх",
  "Зайлуулах"
};

const char* TITLE9_TOP = "Энгийн";
const char* TITLE9_BOTTOM = "байдаг";

const char* TITLE10_TOP = "Зүгээр л";
const char* TITLE10_BOTTOM = "МЭДРЭХ";

constexpr size_t TITLE8_COUNT =
  sizeof(TITLE8_WORDS) / sizeof(TITLE8_WORDS[0]);

// ==================== ХУГАЦАА ====================
// Бүх хугацаа миллисекундээр.

// Бүх текст эхлэхийн өмнөх завсар.
constexpr uint32_t START_DELAY_MS = 2000;

// Үндсэн анимейшний хугацаанууд.
constexpr uint32_t TYPE_MS = 110;
constexpr uint32_t PAUSE_MS = 400;
constexpr uint32_t SLIDE_MS = 1200;
constexpr uint32_t FADE_MS = 700;

constexpr uint32_t TWO_LINES_HOLD_MS = 100;
constexpr uint32_t THIRD_HOLD_MS = 100;
constexpr uint32_t FOURTH_HOLD_MS = 500;

// 5-р текст.
constexpr uint32_t FIFTH_GAP_MS = 250;
constexpr uint32_t BISH_DELAY_MS = 500;
constexpr uint32_t FIFTH_HOLD_MS = 1500;

// 6-р текст.
constexpr uint32_t SIXTH_WORD_DELAY_MS = 500;
constexpr uint32_t SIXTH_HOLD_MS = 800;

// 7-р текст бүрэн байрласны дараа тогтох хугацаа.
constexpr uint32_t SEVENTH_HOLD_MS = 800;

// 7-р текст алга болсноос 8-р текст хүртэлх завсар.
constexpr uint32_t BEFORE_EIGHTH_GAP_MS = 400;

// 8-р текстийн үг бүр тогтох хугацаа.
constexpr uint32_t EIGHTH_WORD_HOLD_MS = 800;

// 9-р текст.
constexpr uint32_t BEFORE_NINTH_GAP_MS = 1000;
constexpr uint32_t NINTH_TYPE_MS = 200;
constexpr uint32_t NINTH_HOLD_MS = 150;

// 10-р текст.
constexpr uint32_t BEFORE_TENTH_GAP_MS = 1000;
constexpr uint32_t TENTH_WORD_DELAY_MS = 700;

// Кадрын зорилтот зай.
constexpr uint32_t ANIM_FRAME_MS = 20;

// ==================== ХУВЬСАГЧИД ====================

uint32_t sceneStart = 0;
uint32_t sceneLastFrame = 0;

size_t titleLetters = 0;

// ==================== UTF-8 ====================

// UTF-8 текстийн бүтэн үсгийн тоо.
size_t letterCount(const char* text) {
  size_t count = 0;

  for (size_t i = 0; text[i]; ++i) {
    if ((uint8_t(text[i]) & 0xC0) != 0x80) {
      ++count;
    }
  }

  return count;
}

// Эхний n үсгийг UTF-8 үсэг таслахгүйгээр хуулна.
void makePrefix(
  const char* text,
  size_t n,
  char* output,
  size_t capacity
) {
  if (capacity == 0) return;

  size_t inputPos = 0;
  size_t outputPos = 0;
  size_t letters = 0;

  while (text[inputPos] && letters < n) {
    size_t charBytes = 1;

    while (
      text[inputPos + charBytes] &&
      (uint8_t(text[inputPos + charBytes]) & 0xC0) == 0x80
    ) {
      ++charBytes;
    }

    if (outputPos + charBytes >= capacity) break;

    for (size_t j = 0; j < charBytes; ++j) {
      output[outputPos++] = text[inputPos++];
    }

    ++letters;
  }

  output[outputPos] = '\0';
}

// ==================== ХӨДӨЛГӨӨН ====================

// Эхлэх, зогсох үед зөөлөн хөдөлгөөн.
float smootherStep(float t) {
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

int interpolateY(int start, int finish, float progress) {
  float position = start + (finish - start) * progress;
  return int(position + 0.5f);
}

// ==================== ТЕКСТ ЗУРАХ ====================

void drawCenteredText(const char* text, int baseline) {
  int width = u8g2.getUTF8Width(text);
  int x = (128 - width) / 2;

  u8g2.drawUTF8(x, baseline, text);
}

void drawCenteredTyping(
  const char* text,
  size_t shown,
  int baseline
) {
  char prefix[80];

  makePrefix(text, shown, prefix, sizeof(prefix));

  // Үсэг нэмэгдэхэд текстийн байрлал өөрчлөгдөхгүй.
  int width = u8g2.getUTF8Width(text);
  int x = (128 - width) / 2;

  u8g2.drawUTF8(x, baseline, prefix);
}

// Пикселүүдийг шатлалтай цөөрүүлж бүдгэрүүлнэ.
void fadeTextPixels(float progress) {
  static const uint8_t pattern[4][4] = {
    { 0,  8,  2, 10 },
    {12,  4, 14,  6 },
    { 3, 11,  1,  9 },
    {15,  7, 13,  5 }
  };

  int removed = int(progress * 16.0f);

  if (removed < 0) removed = 0;
  if (removed > 16) removed = 16;

  u8g2.setDrawColor(0);

  for (int y = 5; y < 59; ++y) {
    for (int x = 12; x < 116; ++x) {
      if (pattern[y & 3][x & 3] < removed) {
        u8g2.drawPixel(x, y);
      }
    }
  }

  u8g2.setDrawColor(1);
}

// ==================== ЧИМЭГЛЭЛ ====================

void drawSparkle(int x, int y, int radius) {
  u8g2.drawHLine(x - radius, y, radius * 2 + 1);
  u8g2.drawVLine(x, y - radius, radius * 2 + 1);

  if (radius >= 3) {
    u8g2.drawPixel(x - 1, y - 1);
    u8g2.drawPixel(x + 1, y - 1);
    u8g2.drawPixel(x - 1, y + 1);
    u8g2.drawPixel(x + 1, y + 1);
  }
}

void drawDecorations(uint32_t now) {
  const int edge = 3;
  const int length = 8;

  // Зүүн дээд булан.
  u8g2.drawHLine(edge, edge, length);
  u8g2.drawVLine(edge, edge, length);

  // Баруун дээд булан.
  u8g2.drawHLine(117, edge, length);
  u8g2.drawVLine(124, edge, length);

  // Зүүн доод булан.
  u8g2.drawHLine(edge, 60, length);
  u8g2.drawVLine(edge, 53, length);

  // Баруун доод булан.
  u8g2.drawHLine(117, 60, length);
  u8g2.drawVLine(124, 53, length);

  bool phase = (now / 700) % 2;

  drawSparkle(14, 10, phase ? 3 : 1);
  drawSparkle(113, 52, phase ? 1 : 3);
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  uint8_t address = 0;

  // OLED-ийн түгээмэл хоёр хаягийг шалгана.
  for (uint8_t candidate = 0x3C; candidate <= 0x3D; ++candidate) {
    Wire.beginTransmission(candidate);

    if (Wire.endTransmission() == 0) {
      address = candidate;
      break;
    }
  }

  if (address == 0) {
    Serial.println("OLED олдсонгүй. Тэжээл, SDA/SCL-ийг шалга.");

    while (true) {
      delay(1000);
    }
  }

  Serial.printf("OLED I2C address: 0x%02X\n", address);

  u8g2.setI2CAddress(address << 1);
  u8g2.setBusClock(400000);
  u8g2.begin();

  u8g2.setFont(u8g2_font_cu12_t_cyrillic);

  titleLetters = letterCount(TITLE1);
  sceneStart = millis();
}

// ==================== LOOP ====================

void loop() {
  uint32_t now = millis();

  if (now - sceneLastFrame < ANIM_FRAME_MS) return;
  sceneLastFrame = now;

  uint32_t sinceStart = now - sceneStart;

  // ---------- Бүх текстийн өмнөх завсар ----------

  if (sinceStart < START_DELAY_MS) {
    u8g2.clearBuffer();
    u8g2.setMaxClipWindow();

    drawDecorations(now);
    u8g2.sendBuffer();
    return;
  }

  // elapsed зөвхөн нэг удаа зарлагдана.
  // Эхний завсрын дараа хугацааг 0-ээс тоолно.
  uint32_t elapsed = sinceStart - START_DELAY_MS;

  size_t thirdLetters = letterCount(TITLE3);
  size_t fourthLetters = letterCount(TITLE4);
  size_t seventhLetters = letterCount(TITLE7_FIRST);

  // ---------- Үе бүрийн эхлэх хугацаа ----------

  uint32_t typingDuration =
    titleLetters * TYPE_MS;

  uint32_t slideStart =
    typingDuration + PAUSE_MS;

  uint32_t fadeStart =
    slideStart + SLIDE_MS + TWO_LINES_HOLD_MS;

  uint32_t thirdStart =
    fadeStart + FADE_MS;

  uint32_t thirdFadeStart =
    thirdStart + thirdLetters * TYPE_MS + THIRD_HOLD_MS;

  uint32_t fourthStart =
    thirdFadeStart + FADE_MS;

  uint32_t fourthFadeStart =
    fourthStart + fourthLetters * TYPE_MS + FOURTH_HOLD_MS;

  uint32_t fourthFadeEnd =
    fourthFadeStart + FADE_MS;

  uint32_t fifthStart =
    fourthFadeEnd + FIFTH_GAP_MS;

  uint32_t bishStart =
    fifthStart + BISH_DELAY_MS;

  uint32_t sixthStart =
    bishStart + FIFTH_HOLD_MS;

  uint32_t sixthFadeStart =
    sixthStart + 2 * SIXTH_WORD_DELAY_MS + SIXTH_HOLD_MS;

  uint32_t seventhStart =
    sixthFadeStart + FADE_MS;

  uint32_t seventhTypingEnd =
    seventhStart + seventhLetters * TYPE_MS;

  uint32_t seventhSlideStart =
    seventhTypingEnd + PAUSE_MS;

  uint32_t seventhEnd =
    seventhSlideStart + SLIDE_MS + SEVENTH_HOLD_MS;

  uint32_t eighthStart =
    seventhEnd + BEFORE_EIGHTH_GAP_MS;

  uint32_t eighthEnd =
    eighthStart + TITLE8_COUNT * EIGHTH_WORD_HOLD_MS;

  uint32_t ninthStart =
    eighthEnd + BEFORE_NINTH_GAP_MS;

  size_t ninthLetters =
    letterCount(TITLE9_TOP) + letterCount(TITLE9_BOTTOM);

  uint32_t ninthEnd =
    ninthStart + ninthLetters * NINTH_TYPE_MS + NINTH_HOLD_MS;

  uint32_t tenthStart =
    ninthEnd + BEFORE_TENTH_GAP_MS;

  // ---------- Шинэ кадр ----------

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_cu12_t_cyrillic);

  int ascent = u8g2.getAscent();
  int descent = u8g2.getDescent();

  int centerBaseline = (64 + ascent + descent) / 2;

  int finalY1 = centerBaseline - 11;
  int finalY2 = centerBaseline + 11;

  constexpr int ROW_SPACING = 16;

  u8g2.setClipWindow(12, 5, 116, 59);

  // ---------- 1. Бодол ----------

  if (elapsed < typingDuration) {
    size_t shown = elapsed / TYPE_MS;

    drawCenteredTyping(TITLE1, shown, centerBaseline);

  } else if (elapsed < slideStart) {
    drawCenteredText(TITLE1, centerBaseline);

  // ---------- 2. Сэтгэл хөдлөл ----------

  } else if (elapsed < fadeStart) {
    float t = float(elapsed - slideStart) / float(SLIDE_MS);
    if (t > 1.0f) t = 1.0f;

    float progress = smootherStep(t);

    int y1 = interpolateY(
      centerBaseline,
      finalY1,
      progress
    );

    int y2 = interpolateY(
      64 + ascent,
      finalY2,
      progress
    );

    drawCenteredText(TITLE1, y1);
    drawCenteredText(TITLE2, y2);

  } else if (elapsed < thirdStart) {
    drawCenteredText(TITLE1, finalY1);
    drawCenteredText(TITLE2, finalY2);

    fadeTextPixels(
      float(elapsed - fadeStart) / float(FADE_MS)
    );

  // ---------- 3. Бүтэлгүйтвэл ----------

  } else if (elapsed < thirdFadeStart) {
    size_t shown = (elapsed - thirdStart) / TYPE_MS;

    if (shown > thirdLetters) {
      shown = thirdLetters;
    }

    drawCenteredTyping(TITLE3, shown, centerBaseline);

  } else if (elapsed < fourthStart) {
    drawCenteredText(TITLE3, centerBaseline);

    fadeTextPixels(
      float(elapsed - thirdFadeStart) / float(FADE_MS)
    );

  // ---------- 4. Муу зүйл ----------

  } else if (elapsed < fourthFadeStart) {
    size_t shown = (elapsed - fourthStart) / TYPE_MS;

    if (shown > fourthLetters) {
      shown = fourthLetters;
    }

    drawCenteredTyping(TITLE4, shown, centerBaseline);

  } else if (elapsed < fourthFadeEnd) {
    drawCenteredText(TITLE4, centerBaseline);

    fadeTextPixels(
      float(elapsed - fourthFadeStart) / float(FADE_MS)
    );

  } else if (elapsed < fifthStart) {
    // Текстгүй завсар.

  // ---------- 5. Мэдрэмж биш ----------

  } else if (elapsed < sixthStart) {
    int fullWidth = u8g2.getUTF8Width(TITLE5_FULL);
    int x = (128 - fullWidth) / 2;

    const char* text =
      elapsed < bishStart ? TITLE5_FIRST : TITLE5_FULL;

    u8g2.drawUTF8(x, centerBaseline, text);

  // ---------- 6. Гурван мөр ----------

  } else if (elapsed < sixthFadeStart) {
    uint32_t sixthElapsed = elapsed - sixthStart;

    drawCenteredText(
      TITLE6_TOP,
      centerBaseline - ROW_SPACING
    );

    if (sixthElapsed >= SIXTH_WORD_DELAY_MS) {
      drawCenteredText(
        TITLE6_MIDDLE,
        centerBaseline
      );
    }

    if (sixthElapsed >= 2 * SIXTH_WORD_DELAY_MS) {
      drawCenteredText(
        TITLE6_BOTTOM,
        centerBaseline + ROW_SPACING
      );
    }

  } else if (elapsed < seventhStart) {
    drawCenteredText(
      TITLE6_TOP,
      centerBaseline - ROW_SPACING
    );

    drawCenteredText(
      TITLE6_MIDDLE,
      centerBaseline
    );

    drawCenteredText(
      TITLE6_BOTTOM,
      centerBaseline + ROW_SPACING
    );

    fadeTextPixels(
      float(elapsed - sixthFadeStart) / float(FADE_MS)
    );

  // ---------- 7. Сэтгэл хөдлөл / Мэдрэмж ----------

  } else if (elapsed < seventhTypingEnd) {
    size_t shown = (elapsed - seventhStart) / TYPE_MS;

    drawCenteredTyping(
      TITLE7_FIRST,
      shown,
      centerBaseline
    );

  } else if (elapsed < seventhSlideStart) {
    drawCenteredText(TITLE7_FIRST, centerBaseline);

  } else if (elapsed < seventhEnd) {
    float t =
      float(elapsed - seventhSlideStart) / float(SLIDE_MS);

    if (t > 1.0f) t = 1.0f;

    float progress = smootherStep(t);

    int y1 = interpolateY(
      centerBaseline,
      finalY1,
      progress
    );

    int y2 = interpolateY(
      64 + ascent,
      finalY2,
      progress
    );

    drawCenteredText(TITLE7_FIRST, y1);
    drawCenteredText(TITLE7_SECOND, y2);

  } else if (elapsed < eighthStart) {
    // 7-р текст алга болсон.
    // Зөвхөн хүрээ, од харагдана.

  // ---------- 8. Үгс нэг нэгээрээ ----------

  } else if (elapsed < eighthEnd) {
    uint32_t eighthElapsed = elapsed - eighthStart;

    size_t wordIndex =
      eighthElapsed / EIGHTH_WORD_HOLD_MS;

    drawCenteredText(
      TITLE8_WORDS[wordIndex],
      centerBaseline
    );

  } else if (elapsed < ninthStart) {
    // 9-р текстийн өмнөх завсар.

  // ---------- 9. Энгийн / байдаг ----------

  } else if (elapsed < ninthEnd) {
    uint32_t ninthElapsed = elapsed - ninthStart;
    size_t shown = ninthElapsed / NINTH_TYPE_MS;

    size_t topLetters = letterCount(TITLE9_TOP);
    size_t bottomLetters = letterCount(TITLE9_BOTTOM);

    size_t shownTop =
      shown < topLetters ? shown : topLetters;

    size_t shownBottom =
      shown > topLetters ? shown - topLetters : 0;

    if (shownBottom > bottomLetters) {
      shownBottom = bottomLetters;
    }

    drawCenteredTyping(
      TITLE9_TOP,
      shownTop,
      finalY1
    );

    drawCenteredTyping(
      TITLE9_BOTTOM,
      shownBottom,
      finalY2
    );

  } else if (elapsed < tenthStart) {
    // 9-р текст алга болсон.
    // 10-р текстийн өмнөх завсар.

  // ---------- 10. Зүгээр л / МЭДРЭХ ----------

  } else {
    uint32_t tenthElapsed = elapsed - tenthStart;

    drawCenteredText(
      TITLE10_TOP,
      finalY1
    );

    if (tenthElapsed >= TENTH_WORD_DELAY_MS) {
      drawCenteredText(
        TITLE10_BOTTOM,
        finalY2
      );
    }
  }

  // Чимэглэл үргэлж тод хэвээр үлдэнэ.
  u8g2.setMaxClipWindow();
  drawDecorations(now);

  u8g2.sendBuffer();
}