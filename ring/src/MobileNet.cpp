/**
 * @file MobileNet.cpp
 * @brief Detect object type
 * Adapted from MBNet_1000.cpp by Neucrack@sipeed.com
 */

#include "MobileNet.h"
#include "names.h"
#include "stdlib.h"
#include "errno.h"

#include "region_layer.h"


const char *MSG_LOADING = "Loading...";
const char *MSG_NO_MEM = "Memory not enough...";
const char *MSG_NO_SD = "No SD card!";
const char *MSG_UNKNOWN = "Unknown";
const char *MSG_INSTRUCTION = "Show me...";

// region layer constructs
#define ANCHOR_NUM 5
static region_layer_t detect_rl;
float g_anchor[ANCHOR_NUM * 2] = {0.57273, 0.677385, 1.87446, 2.06253, 3.33843, 5.47434, 7.88282, 3.52778, 9.77052, 9.16828};

#define BG_COLOR COLOR_BLACK

MobileNet::MobileNet(KPUClass &kpu, Sipeed_ST7789 &lcd, Sipeed_OV2640 &camera)
    : _kpu(kpu), _lcd(lcd), _camera(camera),
      _modelData(nullptr), _labelCount(0), _kpuResult(nullptr)
{
    labels = mbnet_label_name;
    memset(_statistics, 0, sizeof(_statistics));
}

MobileNet::~MobileNet()
{
    if (_modelData)
        free(_modelData);
}

int MobileNet::begin()
{
    if (!_camera.begin())
        return -1;
    if (!_lcd.begin(15000000, BG_COLOR))
        return -2;
    _camera.run(true);

    _lcd.setTextSize(LABEL_TEXT_SIZE);
    _lcd.setTextColor(COLOR_WHITE);
    _lcd.setRotation(_rotation);


    printCenterOnLCD(_lcd, MSG_LOADING, LABEL_TEXT_SIZE);
    // delay(500); //So, I can see some message, not really needed

    return 0;
}

int MobileNet::beginWithModelData(uint8_t *model_data, float threshold)
{
    _threshold = threshold;
    int ret = begin();
    if (ret != 0) {
        Serial.println("FAILED to init");
        printf("FAILED to init, err: %d\n", ret);
        return ret;
    }

    _modelLoaded = _kpu.begin(model_data);
    char string[64] = {0};
    sprintf(string, "model address = %lx\n", (uintptr_t) model_data);
    Serial.print(string);
    Serial.print("modelLoaded: ");
    Serial.println(_modelLoaded);
    if (_modelLoaded != 0)
    {
        Serial.println("FAILED to load model");
        printf("FAILED to load model, err: %d\n", _modelLoaded);
        return -6;
    }

    detect_rl.anchor_number = ANCHOR_NUM;
    detect_rl.anchor = g_anchor;
    detect_rl.threshold = threshold;
    detect_rl.nms_value = 0.1;
    region_layer_init(&detect_rl, 7, 7, 125, 224, 224);

    return 0;
}

int MobileNet::beginWithModelName(const char *kmodel_name, float threshold)
{
    _threshold = threshold;
    int ret = begin();
    if (ret != 0) {
        return ret;
    }

    if (!SD.begin())
    {
        printCenterOnLCD(_lcd, MSG_NO_SD, LABEL_TEXT_SIZE);
        return -3;
    }

    File myFile = SD.open(kmodel_name);
    if (!myFile)
        return -4;
    uint32_t fSize = myFile.size();

    _modelData = (uint8_t *)malloc(fSize);
    if (!_modelData)
    {
        printCenterOnLCD(_lcd, MSG_NO_MEM, LABEL_TEXT_SIZE);
        return ENOMEM;
    }
    long retSize = myFile.read(_modelData, fSize);
    myFile.close();
    if (retSize != fSize)
    {
        free(_modelData);
        _modelData = nullptr;
        return -5;
    }

    _modelLoaded = _kpu.begin(_modelData);
    if (_modelLoaded != 0)
    {
        printf("FAILED to load model, err: %d\n", _modelLoaded);
        free(_modelData);
        _modelData = nullptr;
        return -6;
    }
    return 0;
}

size_t MobileNet::printCenterOnLCD(Sipeed_ST7789 &lcd_, const char *msg, uint8_t textSize)
{
    lcd_.setCursor((lcd_.width() - (6 * textSize * strlen(msg))) / 2, (lcd_.height() - (8*textSize)) / 2);
    return lcd_.print(msg);
}

int MobileNet::detect()
{
    uint8_t *img = _camera.snapshot();
    if (img == nullptr || img == 0) {
        return -1;
    }

    if (_modelLoaded != KPU_ERROR_NONE) {
        return _modelLoaded;
    }

    uint8_t *img888 = _camera.getRGB888();
    if (_kpu.forward(img888) != 0)
    {
        return -2;
    }

    while (!_kpu.isForwardOk())
        ;

    if (_kpu.getResult((uint8_t **)&_kpuResult, &_labelCount) != 0)
    {
        return -3;
    }

    detect_rl.input = _kpuResult;
    region_layer_run(&detect_rl, NULL);

    return 0;
}

#define BLACK       0x0000
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define GREENYELLOW 0xAFE5
#define PINK        0xF81F
#define USER_COLOR  0xAA55

int highestClassIndex = 0;
float highestProb = 0.0f;

void drawboxes(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t classIndex, float prob)
{
    if (x1 >= 224)
        x1 = 223;
    if (x2 >= 224)
        x2 = 223;
    if (y1 >= 224)
        y1 = 223;
    if (y2 >= 224)
        y2 = 223;

    int16_t origin_x = (x1 < x2) ? x1 : x2;
    int16_t origin_y = (y1 < y2) ? y1 : y2;
    int16_t width = abs(x1 - x2);
    int16_t height = abs(y1 - y2);

    // _lcd.drawRect(origin_x, origin_y, width, height, WHITE);
    Serial.print(mbnet_label_name[classIndex]);
    Serial.print(":");
    Serial.print(prob);
    Serial.print(", ");

    if (prob > highestProb) {
      highestProb = prob;
      highestClassIndex = classIndex;
    }
}

void MobileNet::show()
{
    char predLabel[64];// = "Unknown";
    uint16_t *img;

    int16_t validLabelIdx = -1;
    lastPredictionLabelIndex = -1;

#if USING_STATISTICS
    uint8_t firstIdx = 0, secondIdx = 0;
#endif

    if (_modelLoaded == KPU_ERROR_NONE) {

        region_layer_draw_boxes(&detect_rl, drawboxes);
        Serial.print("HIGHEST:");
        Serial.println(labels[highestClassIndex]);

        // _labelCount /= sizeof(float);
        // _labelCount = LABEL_COUNT;
        // Serial.print("Num Labels: ");
        // Serial.println(_labelCount);
        // label_indices_init();
        // label_sort();

        // for(int x = 0; x < _labelCount; x++) {
        //   Serial.print(labels[_labelIndices[x]]);
        //   Serial.print(":");
        //   Serial.print(_kpuResult[x]);
        //   Serial.print(", ");
        // }Serial.println("");

        float firstProb = highestProb;
        const char *firstName = labels[highestClassIndex];

        // label_get(0, &firstProb, &firstName);


        if (firstProb < _threshold) {
            sprintf(predLabel, "%s", MSG_INSTRUCTION);
        }
        else {
            validLabelIdx = 0;
            lastPredictionLabelIndex = _labelIndices[validLabelIdx];
            sprintf(predLabel, "%s (%.2f%s)", firstName, (firstProb*100), "%");
        }
    }
    else {
        Serial.println("Model Error");
    }

    img = _camera.getRGB565();

    uint16_t imgX = 0, imgY = 0;
    if (_rotation == 3 || _rotation == 1) {
        imgX = (_lcd.width() - _camera.width())/2;
        imgY = 0;

        _lcd.drawImage(imgX, imgY, _camera.width(), _camera.height(), img);
        _lcd.fillRect(0, imgY + _camera.height(), _lcd.width(), (_lcd.height() - _camera.height()), BG_COLOR);
    }
    else {
        imgY = (_lcd.height() - _camera.height());
        imgX = 0;

        _lcd.drawImage(imgX, imgY, _camera.width(), _camera.height(), img);
        _lcd.fillRect(0, 0, _camera.width(), imgY, BG_COLOR);
        _lcd.fillRect(_camera.width(), 0, (_lcd.width() - _camera.width()), _lcd.height(), BG_COLOR);
    }

    if (_rotation == 3 || _rotation == 1) {
        uint16_t textX = (uint16_t)((_lcd.width() - (strlen(predLabel)*1.0f*LABEL_TEXT_SIZE*6))/2);
        // printf("x %d, x1 %d\n", textX, (uint16_t)(strlen(predLabel)*1.0f*LABEL_TEXT_SIZE*8));
        _lcd.setCursor(textX, imgY + _camera.height() + 10);
    }
    else {
        _lcd.setCursor(4, 0);
    }

    //Print first prediction or unknown
    _lcd.print(predLabel);

    highestClassIndex = 0;
    highestProb = 0.0f;
}

#if USING_STATISTICS
void MobileNet::label_stats(uint8_t *firstIdxOut, uint8_t *secondIdxOut) {

    uint8_t i, j;
    float prob;
    const char *name;

    // Initialize
    for (j = 0; j < STATISTICS_NUM; ++j) {
        _statistics[j].updated = false;
    }

    for (i = 0; i < STATISTICS_NUM; i++)
    {
        //Query prob, label for an index
        label_get(i, &prob, &name);
        for (j = 0; j < STATISTICS_NUM; ++j)
        {
            if (_statistics[j].name == NULL) //Initial
            {
                _statistics[j].name = name;
                _statistics[j].sum = prob;
                _statistics[j].prob = prob;
                _statistics[j].origIndex = i;
                _statistics[j].updated = true;
                break;
            }
            else if (_statistics[j].name == name) //If occured again, add
            {
                _statistics[j].sum += prob;
                _statistics[j].prob = prob;
                _statistics[j].origIndex = i;
                _statistics[j].updated = true;
                break;
            }
            else
            {
            }
        }
        if (j == STATISTICS_NUM)
        {
            float min = _statistics[0].sum;
            j = 0;
            for (i = 1; i < STATISTICS_NUM; ++i)
            {
                if (_statistics[i].name)
                {
                    if (_statistics[i].sum <= min)
                    {
                        min = _statistics[i].sum;
                        j = i;
                    }
                }
            }
            _statistics[j].name = name;
            _statistics[j].sum = prob;
            _statistics[j].updated = true;
        }
    }
    float firstMax = _statistics[0].sum;
    float secondMax = 0;

    for (i = 0; i < STATISTICS_NUM; ++i)
    {
        if (_statistics[i].name)
        {
            if (_statistics[i].sum > firstMax)
            {
                firstMax = _statistics[i].sum;
                *firstIdxOut = i;
            }
            else if (_statistics[i].sum > secondMax && _statistics[i].sum < firstMax)
            {
                *secondIdxOut = i;
            }
        }

        if (!_statistics[i].updated)
        {
            float tmp = _statistics[i].sum - _statistics[i].sum * 2 / STATISTICS_NUM;
            if (tmp < 0)
                tmp = 0;
            _statistics[i].sum = tmp;
        }
    }
}
#endif

void MobileNet::label_indices_init()
{
    int i;
    for (i = 0; i < _labelCount; i++) {
        _labelIndices[i] = i;
    }
}

void MobileNet::label_sort()
{
    int i,j;
    float tmp_prob;
    uint16_t tmp_index;
    for(j = 0; j < _labelCount; j++) {
        for(i = 0; i < _labelCount-1-j; i++) {
            if (_kpuResult[i] < _kpuResult[i+1])
            {
                tmp_prob = _kpuResult[i];
                _kpuResult[i] = _kpuResult[i+1];
                _kpuResult[i+1] = tmp_prob;

                tmp_index = _labelIndices[i];
                _labelIndices[i] = _labelIndices[i+1];
                _labelIndices[i+1] = tmp_index;
            }
        }
    }
}

void MobileNet::label_get(uint16_t index, float *prob, const char **label)
{
    *prob = _kpuResult[index];
    *label = labels[_labelIndices[index]];
}
