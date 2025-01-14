
// name of the file to store wifi credentials
// The path to read and write files needs to start with "/"
#define WIFIFILENAME "/wifi.txt"

#define SD_FILENAMELENGTH 13
#define DEFAULTFILENAME "/2010-01.csv"

// length of wifi credentials
#define WIFICREDENTIALLENGTH 33

// how often shall sd states be checked?
#define SDwaitMS 2100
// how often shall be looked if a sd card is inserted?
#define NOSDwaitMS 5000

// how often shall data be saved?
#define SD_SAVE_INTERVALL_MS 6 * 60 * 1000

#if SD_SAVE_INTERVALL_MS <= 10000
#error "Data is saved to often to SD card"
#endif

#define CSV_HEADER F("Date;Temperature T_i;Temperature T_o;Humidity H_i;Humidity H_o;Dew point DP_i;Dew point DP_o;validCnt_i;validCnt_o;Fan;Mode;On_s;Off_s")

// print debug?
//define DEBUGSDHANDLING

enum SDHelperStates
{
    SDINIT,
    GETCREDENTIALS,
    SDREADY,
    NOSD
};

/// @brief SDHelper class to handle the sd card. Write sensor data to it and read wifi credentials.
class SDHelper
{
public:
    boolean getWifiCredentials(char *ssid, char *pw);

    boolean init();

    boolean loop();

    SDHelper(uint8_t sdCSpin) : csPin(sdCSpin),
                                sdPresent(false),
                                sdState(SDINIT),
                                credentialsValid(false),
                                fileName(DEFAULTFILENAME)
    {
    }
    void saveDataNow();
    void setFileName(char fn[SD_FILENAMELENGTH]);
    boolean writeCSVHeader();
    boolean writeData(char *dateStr, char *tempStr, char *controlStr);
    boolean isSDinserted();

private:
    boolean getWifiCredentialsFromSD();

    uint8_t csPin;
    boolean sdPresent;
    SDHelperStates sdState;
    unsigned long lastSDTime;
    unsigned long lastSDSaveTime;
    char _ssid[WIFICREDENTIALLENGTH];
    char _pw[WIFICREDENTIALLENGTH];
    boolean credentialsValid;
    boolean checkSDPresence();
    char fileName[SD_FILENAMELENGTH]; // file name for the datalogger
};