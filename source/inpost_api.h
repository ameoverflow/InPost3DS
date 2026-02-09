#ifndef INPOST_H
#define INPOST_H

#include "request.h"

extern ResponseBuffer send_phone_numer;
extern ResponseBuffer send_kod_sms;
extern ResponseBuffer paczkas;
extern ResponseBuffer sent_paczkas;
extern ResponseBuffer returned_paczkas;
extern ResponseBuffer get_refresh_token;
extern ResponseBuffer dane_usera;
extern ResponseBuffer balans_konta;
extern ResponseBuffer validate_paczkomat;
extern ResponseBuffer open_paczkomat;
extern ResponseBuffer terminate_paczka;
extern ResponseBuffer jak_tam_skrytka;
extern ResponseBuffer testreq;

extern const char* refreshToken;
extern const char* authToken;
extern int inpointsy;

extern bool pers_data_done;

#define MAX_PACZKAS 32
#define MAX_EVENTS 16

typedef struct {
    char name[64];
    char date[32];
} PaczkaEvent;

typedef struct {
    char shipmentNumber[32];
    char status[16];
    char statusGroup[16];
    char parcelSize[4];

    char storedDate[32];
    char pickUpDate[32];

    char pickupPointName[128];
    char city[64];
    char street[64];

    float latitude;
    float longitude;

    char phonePrefix[8];
    char phoneNumber[16];
    char opencode[8];
    
    char qrCode[128];
    char imageUrl[256]; 
    
    char latestEvent[64];
    bool courier_paczka;
    bool paczka_openable;
    int eventCount;
    PaczkaEvent events[MAX_EVENTS];
} Paczka;

extern Paczka paczka_list[MAX_PACZKAS];
extern int paczka_count;
void testrequest();
void sendNumerTel(char numertel[60]);
void sendKodSMS(char numertel[60],char kodsms[60]);
void refresh_the_Token(const char* refresh);
void parseRefreshedToken(const char* json);
void getPaczkas(const char* auth);
void parseFakePaczkas();
void parsePaczkas(const char* json);
void getWyslanePaczkas(const char* auth);
void getReturnedPaczkas(const char* auth);
void getPersonalData(const char* auth);
void parsePersonalData(const char* json);
void getInPointsBalance(const char* auth);
void parseInPointsBalance(const char* json);
void getPaczkomatStatus(const char* auth, const char* shipmentNumber, const char* openCode, const char* recieverPhoneNumber, const char* recieverPhonePrefix, float latitude, float longitude);
void openPaczkomat(const char* auth, const char* uuid);
void terminatePaczka(const char* auth, const char* uuid);
void jaktamSkrytka(const char* auth, const char* uuid, bool otwarta);
void getPaczkomatImage(const char* url);


void getEverything(const char* auth);

#endif
