// Checks the protocol parsers against frames captured from real devices
// (quoted in the reference projects listed in docs/PROTOCOLS.md).
// Builds on any OS:  g++ -std=c++17 tests/parsers_test.cpp -o parsers_test
#define MBT_PARSER_TEST 1
#include "../mods/mouse-battery-tray.wh.cpp"

#include <cstdio>
#include <vector>

using namespace mbt;

static int g_failures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            g_failures++;                                              \
        }                                                              \
    } while (0)

static std::vector<uint8_t> Pad(std::vector<uint8_t> v, size_t n) {
    v.resize(n, 0);
    return v;
}

static void TestF83() {
    uint8_t req[kF83Len];
    BuildF83(req);
    // Same bytes as the web driver's getBatPer: data[2]=2, data[3]=2,
    // data[5]=0x83, behind report ID 0.
    CHECK(req[0] == 0 && req[1] == 0 && req[2] == 0 && req[3] == 2 && req[4] == 2 &&
          req[5] == 0 && req[6] == 0x83 && req[7] == 0);

    // Windows layout (report ID first): 00 A1 00 02 02 00 83 <chg> <lvl>
    auto r = Pad({0x00, 0xA1, 0x00, 0x02, 0x02, 0x00, 0x83, 0x01, 0x16}, 65);
    Parsed p = ParseF83(r.data(), (int)r.size());
    CHECK(p.ok && p.level == 22 && p.charging && !p.asleep);

    // Without the report ID, as Lamzu's reference reads it: a1 00 02 02 00 83 00 4b
    auto q = Pad({0xA1, 0x00, 0x02, 0x02, 0x00, 0x83, 0x00, 0x4B}, 64);
    p = ParseF83(q.data(), (int)q.size());
    CHECK(p.ok && p.level == 75 && !p.charging);

    auto sleeping = Pad({0x00, 0xA4, 0x00, 0x02, 0x02, 0x00, 0x83}, 65);
    p = ParseF83(sleeping.data(), (int)sleeping.size());
    CHECK(p.ok && p.asleep && p.level == -1);

    // The request still sitting in the buffer is not an answer.
    p = ParseF83(req, kF83Len);
    CHECK(!p.ok);

    auto bad = Pad({0x00, 0xA1, 0x00, 0x02, 0x02, 0x00, 0x83, 0x00, 0xC8}, 65);
    CHECK(!ParseF83(bad.data(), (int)bad.size()).ok);  // 200 %
}

static void TestCompx() {
    uint8_t req[kCompxLen];
    BuildCompx(req);
    CHECK(req[0] == 0x08 && req[1] == 0x04 && req[16] == 0x49);

    // Captured: 35 %, 3732 mV, on the dongle.
    std::vector<uint8_t> r = {0x08, 0x04, 0x00, 0x00, 0x00, 0x02, 0x23, 0x00, 0x0E,
                              0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82};
    Parsed p = ParseCompx(r.data(), (int)r.size());
    CHECK(p.ok && p.level == 35 && !p.charging);

    // i-Rocks capture with header 0x09: 75 %, on the cable.
    std::vector<uint8_t> c = {0x09, 0x04, 0x00, 0x00, 0x00, 0x02, 0x4B, 0x01, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA};
    p = ParseCompx(c.data(), (int)c.size());
    CHECK(p.ok && p.level == 75 && p.charging);

    r[16] ^= 1;  // broken checksum
    CHECK(!ParseCompx(r.data(), (int)r.size()).ok);
}

static void TestN54() {
    auto r = Pad({0x08, 0x72, 0x00, 0x3A, 0x00, 0x07, 0x01, 0x64, 0x16, 0x11}, 64);
    Parsed p = ParseN54(r.data(), (int)r.size(), true);
    CHECK(p.ok && p.level == 100 && !p.charging);
    r[2] = 0x01;  // stale answer, the mouse did not reply
    p = ParseN54(r.data(), (int)r.size(), false);
    CHECK(p.ok && p.asleep);
}

static void TestBeken() {
    std::vector<uint8_t> x11 = {0x03, 0x55, 0x40, 0x01, 0x1F};
    Parsed p = ParseBeken(x11.data(), 5);
    CHECK(p.ok && p.level == 31 && !p.charging);

    std::vector<uint8_t> r1 = {0x03, 0x10, 0x40, 0x01, 0x09};
    p = ParseBeken(r1.data(), 5);
    CHECK(p.ok && p.level == 90);

    std::vector<uint8_t> x3max = {0x03, 0x4E, 0x40, 0x01, 0x1E};
    p = ParseBeken(x3max.data(), 5);
    CHECK(p.ok && p.level == 30);

    std::vector<uint8_t> x3maxCable = {0x03, 0x4E, 0x40, 0x02, 0x43};
    p = ParseBeken(x3maxCable.data(), 5);
    CHECK(p.ok && p.charging && p.level == -1);

    std::vector<uint8_t> charging = {0x03, 0x55, 0x40, 0x03, 0x00};
    p = ParseBeken(charging.data(), 5);
    CHECK(p.ok && p.charging && p.level == -1);

    std::vector<uint8_t> ack = {0x03, 0x10, 0x50, 0x01, 0x09};  // config ACK
    CHECK(!ParseBeken(ack.data(), 5).ok);
}

static void TestMchose() {
    uint8_t req[21];
    BuildMchoseStatus(req, 21, 0x11);
    CHECK(req[0] == 0x11 && req[1] == 0xF9 && req[2] == 0xFF && req[20] == 0xFF);

    // M7 Ultra: decoded payload 53 52 31 00 02 05 07 00 09 64 00 64 (100 %).
    uint8_t pay[] = {0x53, 0x52, 0x31, 0x00, 0x02, 0x05, 0x07, 0x00, 0x09, 0x64, 0x00, 0x64};
    std::vector<uint8_t> r = {0x11, 0xF9};
    for (uint8_t b : pay) r.push_back(b ^ 0xFF);
    r = Pad(r, 21);
    Parsed p = ParseMchoseStatus(r.data(), (int)r.size());
    CHECK(p.ok && p.level == 100 && !p.charging);

    // Asleep: all-zero payload -> no MCHOSE vid -> not a reading.
    std::vector<uint8_t> z = Pad({0x11, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF}, 21);
    for (size_t i = 2; i < z.size(); i++) z[i] = 0xFF;
    CHECK(!ParseMchoseStatus(z.data(), (int)z.size()).ok);

    // L7 Pro push: 93 %, on the 2.4 GHz link.
    std::vector<uint8_t> push = {0x13, 0x1D, 0xFE, 0xFE, 0xFF, 0xA2, 0xFD, 0xFD};
    p = ParseMchosePush(push.data(), (int)push.size());
    CHECK(p.ok && p.level == 93 && !p.charging);

    // G7: 46 % not charging, then 46 % charging on the cable.
    std::vector<uint8_t> g7 = {0xAA, 0x30, 0xA5, 0x0B, 0x0A, 0x01, 0x01, 0x01, 0x2E, 0x00, 0x00, 0x00};
    p = ParseG7(g7.data(), (int)g7.size());
    CHECK(p.ok && p.level == 46 && !p.charging);
    std::vector<uint8_t> g7c = {0x00, 0xAA, 0x30, 0xA5, 0x3C, 0x0A, 0x01, 0x01, 0x01, 0x2E, 0x01, 0x00};
    p = ParseG7(g7c.data(), (int)g7c.size());
    CHECK(p.ok && p.level == 46 && p.charging);
}

static void TestRazer() {
    uint8_t req[kRazerLen];
    BuildRazer(req, 0x1F, 0x07, 0x80);
    CHECK(req[0] == 0 && req[2] == 0x1F && req[6] == 0x02 && req[7] == 0x07 &&
          req[8] == 0x80);
    uint8_t crc = 0;
    for (int i = 3; i < 89; i++) crc ^= req[i];
    CHECK(req[89] == crc);

    // DeathAdder V4 Pro: 02 1f 00 00 00 02 07 80 00 ab -> 171/255 = 67 %.
    auto r = Pad({0x00, 0x02, 0x1F, 0x00, 0x00, 0x00, 0x02, 0x07, 0x80, 0x00, 0xAB}, 91);
    int value = -1;
    CHECK(ParseRazer(r.data(), (int)r.size(), 0x07, 0x80, &value) == 0x02);
    CHECK(value == 0xAB && (value * 100 + 127) / 255 == 67);

    // Synapse's LED answer on the same channel is not ours.
    auto led = Pad({0x00, 0x02, 0x1F, 0x00, 0x00, 0x00, 0x02, 0x0F, 0x03}, 91);
    CHECK(ParseRazer(led.data(), (int)led.size(), 0x07, 0x80, &value) == -1);
}

static void TestLogitechVoltage() {
    CHECK(LogitechVoltageToPercent(4200) == 100);
    CHECK(LogitechVoltageToPercent(3811) == 50);
    CHECK(LogitechVoltageToPercent(3400) == 0);
    int mid = LogitechVoltageToPercent(3900);
    CHECK(mid > 60 && mid < 70);
}

static void TestCustom() {
    ByteMatch m[] = {{1, 0xA1}, {6, 0x83}};
    auto r = Pad({0x00, 0xA1, 0x00, 0x02, 0x02, 0x00, 0x83, 0x00, 0x40}, 65);
    Parsed p = ParseCustom(r.data(), (int)r.size(), m, 2, 8, 7, 100);
    CHECK(p.ok && p.level == 64 && !p.charging);
    r[1] = 0xA4;
    CHECK(!ParseCustom(r.data(), (int)r.size(), m, 2, 8, 7, 100).ok);
    std::vector<uint8_t> raw = {0x05, 0xFF};
    p = ParseCustom(raw.data(), 2, nullptr, 0, 1, -1, 255);
    CHECK(p.ok && p.level == 100);
}

int main() {
    TestF83();
    TestCompx();
    TestN54();
    TestBeken();
    TestMchose();
    TestRazer();
    TestLogitechVoltage();
    TestCustom();
    if (g_failures) {
        printf("%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("all parser tests passed\n");
    return 0;
}
