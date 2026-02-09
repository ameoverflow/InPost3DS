#include <3ds.h>
#include <citro2d.h>
#include "inpost_api.h"
#include "scene_manager.h"
#include "main.h"
#include "cwav_shit.h"
#include "sprites.h"
#include "text.h"
#include <math.h>
#include <stdio.h>
#include <jansson.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "request.h"

ResponseBuffer send_phone_numer = {NULL, 0, 0, false};
ResponseBuffer send_kod_sms = {NULL, 0, 0, false};
ResponseBuffer paczkas = {NULL, 0, 0, false};
ResponseBuffer sent_paczkas = {NULL, 0, 0, false};
ResponseBuffer returned_paczkas = {NULL, 0, 0, false};
ResponseBuffer get_refresh_token = {NULL, 0, 0, false};
ResponseBuffer dane_usera = {NULL, 0, 0, false};
ResponseBuffer balans_konta = {NULL, 0, 0, false};
ResponseBuffer validate_paczkomat = {NULL, 0, 0, false};
ResponseBuffer open_paczkomat = {NULL, 0, 0, false};
ResponseBuffer terminate_paczka = {NULL, 0, 0, false};
ResponseBuffer jak_tam_skrytka = {NULL, 0, 0, false};
ResponseBuffer paczkomat_image = {NULL, 0, 0, false};
ResponseBuffer testreq = {NULL, 0, 0, false};

const char* refreshToken = NULL;
const char* authToken = NULL;
int inpointsy = 0;

bool pers_data_done;


Paczka paczka_list[MAX_PACZKAS];
int paczka_count = 0;

static void jstrcpy(char *dst, size_t dstsz, json_t *jstr) {
    if (json_is_string(jstr)) {
        snprintf(dst, dstsz, "%s", json_string_value(jstr));
    } else {
        dst[0] = '\0';
    }
}


void testrequest(){
    queue_request("https://inpost.pl/404", NULL, NULL, &testreq, false);
}

// wyślij prośbe o kod sms na podany numer
void sendNumerTel(char numertel[60]){
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    char request[128]; 
    sprintf(request, "{\"phoneNumber\":{\"prefix\":\"+48\",\"value\":\"%s\"}}", numertel);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/account", request, headers, &send_phone_numer, false);
}

// wyślij kod sms z wcześniej dostanego sms'a
void sendKodSMS(char numertel[60], char kodsms[60]){
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    char request[256]; 
    sprintf(request, "{\"phoneNumber\":{\"prefix\":\"+48\",\"value\":\"%s\"},\"smsCode\":\"%s\",\"devicePlatform\":\"Android\"}", numertel, kodsms);
    queue_request("https://api-inmobile-pl.easypack24.net/v1/account/verification", request, headers, &send_kod_sms, false);
}

// odśwież token używając refresha (tego co dostawałeś przy logowaniu bądz po odświeżeniu)
void refresh_the_Token(const char* refresh) {
    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");

    char request[256]; 
    snprintf(request, sizeof(request), "{\"refreshToken\":\"%s\"}", refresh);

    queue_request("https://api-inmobile-pl.easypack24.net/v1/authenticate", request, headers, &get_refresh_token, false);

    curl_slist_free_all(headers);
}

void parseRefreshedToken(const char* json){
    json_t *root = json_loads(json, 0, NULL);
    json_t *rootenmach = json_load_file("/3ds/in_post_dane.json", 0, NULL);
    json_t *accesstoken = json_object_get(root, "authToken");
    json_object_set_new(rootenmach, "access", json_string(json_string_value(accesstoken)));
    json_dump_file(rootenmach, "/3ds/in_post_dane.json", JSON_INDENT(4));
}

// pobierz paczki
void getPaczkas(const char* auth) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v4/parcels/tracked", NULL, headers, &paczkas, false);
}

// (używane w dev buildach) parsuj test dane
void parseFakePaczkas() {
    bool ignore_bo_kurier = false;
    json_error_t error;

    json_t *root = json_load_file("romfs:/test_data.json", 0, &error);
    if (!root) {
        log_to_file("[parsePaczkas] JSON error: %s (line %d)",
                    error.text, error.line);
        return;
    }

    json_t *parcels = json_object_get(root, "parcels");
    if (!json_is_array(parcels)) {
        log_to_file("[parsePaczkas] parcels is not array");
        json_decref(root);
        return;
    }

    paczka_count = 0;

    size_t total_parcels = json_array_size(parcels);

    for (size_t i = total_parcels; i > 0; i--) {
        

        json_t *parcel = json_array_get(parcels, i - 1);
    


        if (paczka_count >= MAX_PACZKAS)
            break;

        Paczka *p = &paczka_list[paczka_count];
        
        if (strstr(json_string_value(json_object_get(parcel, "shipmentType")), "courier") != NULL) {
            p->courier_paczka = true;
            ignore_bo_kurier = true;
        } else {
            p->courier_paczka = false;
            ignore_bo_kurier = false;
        }
       
        jstrcpy(p->shipmentNumber, sizeof(p->shipmentNumber),
                json_object_get(parcel, "shipmentNumber"));
        jstrcpy(p->statusGroup, sizeof(p->statusGroup),
                json_object_get(parcel, "statusGroup"));
        jstrcpy(p->parcelSize, sizeof(p->parcelSize),
                json_object_get(parcel, "parcelSize"));
        if (!ignore_bo_kurier) {
            jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                    json_object_get(parcel, "pickUpDate"));     
            jstrcpy(p->opencode, sizeof(p->opencode),
                    json_object_get(parcel, "openCode"));
            if (json_object_get(parcel, "openCode")) {
                p->paczka_openable = true;
            } else {
                p->paczka_openable = false;
            }
            jstrcpy(p->qrCode, sizeof(p->qrCode),
                    json_object_get(parcel, "qrCode"));
        } 
        jstrcpy(p->status, sizeof(p->status),
                json_object_get(parcel, "status"));
        if (strstr(p->status, "DELIVERED")) {
            p->paczka_openable = false;
        } 
        if (ignore_bo_kurier) {
            json_t *sender = json_object_get(parcel, "sender");
            json_t *sender_name = json_object_get(sender, "name");
            jstrcpy(p->pickupPointName, sizeof(p->pickupPointName), sender_name);
        }
        
        if (!ignore_bo_kurier) {
            json_t *pickup = json_object_get(parcel, "pickUpPoint");
            if (json_is_object(pickup)) {
                jstrcpy(p->pickupPointName, sizeof(p->pickupPointName),
                        json_object_get(pickup, "name"));
                jstrcpy(p->imageUrl, sizeof(p->imageUrl),
                        json_object_get(pickup, "imageUrl"));

                json_t *addr = json_object_get(pickup, "addressDetails");
                if (json_is_object(addr)) {
                    jstrcpy(p->city, sizeof(p->city),
                            json_object_get(addr, "city"));
                    jstrcpy(p->street, sizeof(p->street),
                            json_object_get(addr, "street"));
                }
                json_t *location = json_object_get(pickup, "location");
                if (json_is_object(location)) {
                    json_t *lat = json_object_get(location, "latitude");
                    json_t *lon = json_object_get(location, "longitude");

                    if (json_is_number(lat))
                        p->latitude = (float)json_number_value(lat);
                    if (json_is_number(lon))
                        p->longitude = (float)json_number_value(lon);
                }
            }
        }
        json_t *receiver = json_object_get(parcel, "receiver");
        json_t *receiver_phone = json_object_get(receiver, "phoneNumber");
        if (json_is_object(receiver_phone)) {
            jstrcpy(p->phonePrefix, sizeof(p->phonePrefix),
                    json_object_get(receiver_phone, "prefix"));
            jstrcpy(p->phoneNumber, sizeof(p->phoneNumber),
                    json_object_get(receiver_phone, "value"));
        }

        
        json_t *eventsArray = json_object_get(parcel, "events");
        if (json_is_array(eventsArray) && json_array_size(eventsArray) > 0) {
            json_t *latest = json_array_get(eventsArray, 0); 
            jstrcpy(p->latestEvent, sizeof(p->latestEvent), json_object_get(latest, "eventTitle"));
        } else {
            p->latestEvent[0] = '\0';
        }
        

        
        p->eventCount = 0;
        json_t *eventLog = json_object_get(parcel, "events");

        jstrcpy(p->storedDate, sizeof(p->storedDate),
                json_object_get(json_array_get(eventLog, 0), "date"));
        if (json_is_array(eventLog)) {
            size_t e;
            json_t *event;

            json_array_foreach(eventLog, e, event) {
                if (p->eventCount >= MAX_EVENTS)
                    break;

                PaczkaEvent *ev = &p->events[p->eventCount];

                jstrcpy(ev->name, sizeof(ev->name),
                        json_object_get(event, "eventTitle"));
                jstrcpy(ev->date, sizeof(ev->date),
                        json_object_get(event, "date"));

                p->eventCount++;
            }
        }

        paczka_count++;
    }

    log_to_file("[parsePaczkas] Parsed %d paczkas", paczka_count);

    json_decref(root);
}

// parsuj paczki do structa "paczka_list"
void parsePaczkas(const char* json) {
    bool ignore_bo_kurier = false;
    json_error_t error;
    json_t *root = json_loads(json, 0, NULL);
    
    if (!root) {
        log_to_file("[parsePaczkas] JSON error: %s (line %d)",
                    error.text, error.line);
        return;
    }

    json_t *parcels = json_object_get(root, "parcels");
    if (!json_is_array(parcels)) {
        log_to_file("[parsePaczkas] parcels is not array");
        json_decref(root);
        return;
    }

    paczka_count = 0;

    size_t total_parcels = json_array_size(parcels);
    
    for (size_t i = total_parcels; i > 0; i--) {
        json_t *parcel = json_array_get(parcels, i - 1);
    

        if (paczka_count >= MAX_PACZKAS)
            break;

        Paczka *p = &paczka_list[paczka_count];
        
        if (strstr(json_string_value(json_object_get(parcel, "shipmentType")), "courier") != NULL) {
            p->courier_paczka = true;
            ignore_bo_kurier = true;
        } else {
            p->courier_paczka = false;
            ignore_bo_kurier = false;
        }
       
        jstrcpy(p->shipmentNumber, sizeof(p->shipmentNumber),
                json_object_get(parcel, "shipmentNumber"));
        jstrcpy(p->statusGroup, sizeof(p->statusGroup),
                json_object_get(parcel, "statusGroup"));
        jstrcpy(p->parcelSize, sizeof(p->parcelSize),
                json_object_get(parcel, "parcelSize"));
        if (!ignore_bo_kurier) {
            jstrcpy(p->pickUpDate, sizeof(p->pickUpDate),
                    json_object_get(parcel, "pickUpDate"));     
            jstrcpy(p->opencode, sizeof(p->opencode),
                    json_object_get(parcel, "openCode"));
            if (json_object_get(parcel, "openCode")) {
                p->paczka_openable = true;
            } else {
                p->paczka_openable = false;
            }
            jstrcpy(p->qrCode, sizeof(p->qrCode),
                    json_object_get(parcel, "qrCode"));
        } 
        jstrcpy(p->status, sizeof(p->status),
                json_object_get(parcel, "status"));
        if (strstr(p->status, "DELIVERED")) {
            p->paczka_openable = false;
        } 
        if (ignore_bo_kurier) {
            json_t *sender = json_object_get(parcel, "sender");
            json_t *sender_name = json_object_get(sender, "name");
            jstrcpy(p->pickupPointName, sizeof(p->pickupPointName), sender_name);
        }
        
        if (!ignore_bo_kurier) {
            json_t *pickup = json_object_get(parcel, "pickUpPoint");
            if (json_is_object(pickup)) {
                jstrcpy(p->pickupPointName, sizeof(p->pickupPointName),
                        json_object_get(pickup, "name"));
                jstrcpy(p->imageUrl, sizeof(p->imageUrl),
                        json_object_get(pickup, "imageUrl"));

                json_t *addr = json_object_get(pickup, "addressDetails");
                if (json_is_object(addr)) {
                    jstrcpy(p->city, sizeof(p->city),
                            json_object_get(addr, "city"));
                    jstrcpy(p->street, sizeof(p->street),
                            json_object_get(addr, "street"));
                }
                json_t *location = json_object_get(pickup, "location");
                if (json_is_object(location)) {
                    json_t *lat = json_object_get(location, "latitude");
                    json_t *lon = json_object_get(location, "longitude");

                    if (json_is_number(lat))
                        p->latitude = (float)json_number_value(lat);
                    if (json_is_number(lon))
                        p->longitude = (float)json_number_value(lon);
                }
            }
        }
        json_t *receiver = json_object_get(parcel, "receiver");
        json_t *receiver_phone = json_object_get(receiver, "phoneNumber");
        if (json_is_object(receiver_phone)) {
            jstrcpy(p->phonePrefix, sizeof(p->phonePrefix),
                    json_object_get(receiver_phone, "prefix"));
            jstrcpy(p->phoneNumber, sizeof(p->phoneNumber),
                    json_object_get(receiver_phone, "value"));
        }

        
        json_t *eventsArray = json_object_get(parcel, "events");
        if (json_is_array(eventsArray) && json_array_size(eventsArray) > 0) {
            json_t *latest = json_array_get(eventsArray, 0); 
            jstrcpy(p->latestEvent, sizeof(p->latestEvent), json_object_get(latest, "eventTitle"));
        } else {
            p->latestEvent[0] = '\0';
        }
        

        
        p->eventCount = 0;
        json_t *eventLog = json_object_get(parcel, "events");

        jstrcpy(p->storedDate, sizeof(p->storedDate),
                json_object_get(json_array_get(eventLog, 0), "date"));
        if (json_is_array(eventLog)) {
            size_t e;
            json_t *event;

            json_array_foreach(eventLog, e, event) {
                if (p->eventCount >= MAX_EVENTS)
                    break;

                PaczkaEvent *ev = &p->events[p->eventCount];

                jstrcpy(ev->name, sizeof(ev->name),
                        json_object_get(event, "eventTitle"));
                jstrcpy(ev->date, sizeof(ev->date),
                        json_object_get(event, "date"));

                p->eventCount++;
            }
        }

        paczka_count++;
    }

    log_to_file("[parsePaczkas] Parsed %d paczkas", paczka_count);

    json_decref(root);
}

void getWyslanePaczkas(const char* auth) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v4/parcels/sent", NULL, headers, &sent_paczkas, false);

}

// pobierz zwrócone paczki
void getReturnedPaczkas(const char* auth) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/v1/returns/tickets", NULL, headers, &returned_paczkas, false);

}

// pobierz dane jak imie i nazwisko, ulubiony paczkomat itp
void getPersonalData(const char* auth) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPersonalData] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/izi/app/shopping/v2/profile", NULL, headers, &dane_usera, false);
}

// przeparsuj dane z getPersonalData do in_post_dane.json
void parsePersonalData(const char* json){
    pers_data_done = false;
    json_t *root = json_loads(json, 0, NULL);
    json_t *rootenmach = json_load_file("/3ds/in_post_dane.json", 0, NULL);
    
    json_t *personal = json_object_get(root, "personal");
    json_t *firstname = json_object_get(personal, "firstName");
    json_t *lastname = json_object_get(personal, "lastName");
    json_t *phone = json_object_get(personal, "phoneNumber");
    json_t *phoneprefix = json_object_get(personal, "phoneNumberPrefix");
    json_t *email = json_object_get(personal, "email");

    
    json_t *delivery = json_object_get(root, "delivery");
    json_t *points = json_object_get(delivery, "points");
    json_t *items = json_object_get(points, "items");
    json_t *pierwszy_paczkomat = json_array_get(items, 0);
    json_t *id_paczkomatu = json_object_get(pierwszy_paczkomat, "name");
    json_t *dane_adresowe = json_object_get(pierwszy_paczkomat, "addressLines");

    

    json_object_set_new(rootenmach, "firstname", json_string(json_string_value(firstname)));
    json_object_set_new(rootenmach, "lastname", json_string(json_string_value(lastname)));
    json_object_set_new(rootenmach, "phone_number", json_string(json_string_value(phone)));
    json_object_set_new(rootenmach, "phoneprefix", json_string(json_string_value(phoneprefix)));
    json_object_set_new(rootenmach, "email", json_string(json_string_value(email)));
    json_object_set_new(rootenmach, "id_pref_paczkomatu", json_string(json_string_value(id_paczkomatu)));
    size_t i;
    json_t *linia;

    json_t *adres_array = json_array();
    json_array_foreach(dane_adresowe, i, linia) {
        json_array_append_new(adres_array, json_string(json_string_value(linia)));
    }
    json_object_set_new(rootenmach, "adres_pref_paczkomatu", adres_array);


    json_dump_file(rootenmach, "/3ds/in_post_dane.json", JSON_INDENT(4));
    pers_data_done = true;
}

// pobierz inpointsy (never used lol)
void getInPointsBalance(const char* auth) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPersonalData] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    queue_request("https://api-inmobile-pl.easypack24.net/loyalty/v1/wallet/balance", NULL, headers, &balans_konta, false);
    curl_slist_free_all(headers);
    free(authheader);
}

// parsuj inpointsy (never used lol)
void parseInPointsBalance(const char* json){
    json_t *root = json_loads(json, 0, NULL);
    json_t *balans = json_object_get(root, "currentBalance");
    inpointsy = json_integer_value(balans);
}

// pobierz status paczkomatu który otwierasz
void getPaczkomatStatus(const char* auth, const char* shipmentNumber, const char* openCode, const char* recieverPhoneNumber, const char* recieverPhonePrefix, float latitude, float longitude) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    json_t *data = json_object();

    json_t *parcel = json_object();
    json_object_set_new(parcel, "shipmentNumber",
                        json_string(shipmentNumber));
    json_object_set_new(parcel, "openCode",
                        json_string(openCode));

    json_t *phone = json_object();
    json_object_set_new(phone, "prefix",
                        json_string(recieverPhonePrefix));
    json_object_set_new(phone, "value",
                        json_string(recieverPhoneNumber));

    json_object_set_new(parcel, "recieverPhoneNumber", phone);

    json_t *geo = json_object();
    json_object_set_new(geo, "latitude", json_real(latitude));
    json_object_set_new(geo, "longitude", json_real(longitude));
    json_object_set_new(geo, "accuracy", json_real(13.365));

    json_object_set_new(data, "parcel", parcel);
    json_object_set_new(data, "geoPoint", geo);
    char *data_json = json_dumps(data, JSON_INDENT(2));
    queue_request("https://api-inmobile-pl.easypack24.net/v2/collect/validate", data_json, headers, &validate_paczkomat, false);
    free(data_json);
}

// sezamie otwórz się
void openPaczkomat(const char* auth, const char* uuid) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    json_t *data = json_object();
    json_object_set_new(data, "sessionUuid", json_string(uuid));
    char *data_json = json_dumps(data, JSON_INDENT(2));
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/open", data_json, headers, &open_paczkomat, false);
    free(data_json);
}

// .........:!!^..:77~:......^??:...........:.:~~:.....::::^^^~~~~~~~~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!77777~.....::::..:!!!!!!!!!!~~~~~~~~~~~~~~~~~!!!!77777777?????7!:...
// ...............:7JJ7!......^:...........^~^........:::::^^^~~~~~~~~~!!!!!!!!7??7!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!77777!!!!777777777!......::^:...^!!!!!!!!~~~~~~~~~~~~~~~~~!!!7777777?????????7:...
// .................:!Y5~..................:::.....:::.::::^^^~~~~~~~!!77!!!!7775P57!!!!!!!!77!!!!!!!!!!!!!!!!!!!!!!!!!!!7!!!!!!!!777777777!....^^:^~!:..:!!!!!!!!~~~~~~~~~~~~~~~~!!!77777777??????????^...
// ...................^~.........................::7J7:::::^^^^~~~~~!!?YYJ777777J5Y7!77!!!!7?J7!!!!!!!7!!!!!!!!!!!!!!!!!!!!!!!!!77777777777!...:77!7?!....!!!!!!!!!~~~~~~~~~~~~~~!!!!77777777?????????7^...
// .....................^^:.....................:::^~^::::::^^^^^~~~~!7Y5Y7777777777777!!!!7777!!!77777!!~~~~~~~~~~~!!!!!!!!!!!!!7777777777!:...!?7777:...~!!!!!!!!~~~~~~~~~~~~~!!!!777777777?????????~....
// .........:::^:......:YP?.......................::~~~~!^::::^^^^~~~!!77!~~!7777777777777777!!!!!7777!!~~~~~~~~~~~~~~!!!!!!~~!!!!777777777?^...^^..:~...:7!!!!!!!!~~~~~~~~~~~!!!!!!777777777?????????^....
// :::^^^^^~77777~~~~~^^?J7^:....................::^~~~~^^::::^^^^~~~!7~....:^!7777777777???7!!!!77!!!!!~~~~~~~~~~~~~~~~~~~~~~~~~!!!7777777?^:..:........^!!!!!!!~~~~~~~~~~~~!!!!!!!!77777777????????7:....
// ~!!!77777????????7!!~~~~^^::.............:~~:::^^~!!^:::^^^:^7??!!77:......:^!777777777777!!!!!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!!!77777777^::.......:~!!!!!~~~~~~~~~~~~~~!!!77777!77777777?????????^....
// !777????JJJJJJ???7!!~~~~~~^^:::::........:^~^^^^~7Y5?^^~~~^^^?YY7!77^:::::...:~7??77777777777!!~~~~~~~!!!~~^^^~~~~~~~~~~~~~~~~!!!!777777777!~:::...!!7!!!!~~~~~~~~~~~~~~!!!!77777777777777?????????^....
// !???JJJJJJJJJJJ??777!!!!!~~~~~777::.......::^^~~~~?J?!~77?~^~!?7!!!77~^::::::.:~7?77777777777!!~~~~~~~!!~~^^^^^^~~~^^^~~~~~~~~!!!!!77777777?J:::...777!!!!~~~~~~~~~~~~~~!!!!!7777777777777?????????~....
// 7J?JYYYYYJJJJJJ??????77777!!!!7J?^:::::::::^^^~~~~~~~~~!!!~~~~!!!!!777~^^:::::::~??77777777!!!~~~~^^^~~~~~~~~^~~~~~~~~~~~~~~~~!!!!777777777?J.:^...!?7!!!!~~~~~~~~~~~~~~!!!!!7777777777777?????????~::..
// 7JJJJYYYYYYYJJJ???JJJJ?????777!!!~~^^^^^^^^^^^~~~~~~~~~~~~~!!!!!!777?7~:^^:::::::~7777777!!!!~~~~^^^^~~~~~~~~~~~~~~~~!?7!~~~~~!!!!777777777?7.:^...~77!!!!~~~~~~~~~~~~~~~~!!!!!77777777777?????????7~::.
// 7JJJJJJJYYYYJJJJJJYY5YJJJJJJ?777!!!!~~~^^^^^~~~~~~~~~~~~~~~!!!77777??7^::::::::::^!777777!!!~~~~~~~~~~~~~~~~^^~~~~~~^75P?~~~~~!!!!77777777~:....:::::~7!!!!~~~~~~~~^~~~~~~!!!!!!!!!7777777?????????7^:^:
// !?JYYJJJJJJYYJJJJJJYYYJJJJJJJ??7777!!!!~~~~~~~~~~~~~~~~~~~!!!77777???7^^::::::::::~!!!!!!!!!!~^:::::::^~~~~~~~~~~~~~~~?55!~~~!!!777777777~......^^^:..^!!!!~~~~~~^^^~~~~~~!!!!!!!!7777777??????????!:.::
// !?JYYJJJJJJYYYYYYYYYYYJJJJJJJ???777777!!!!!!!!!!~~~~~~~~!!!7777777???!^^^:::::^^::^!!!!!~^^^:...........:^~~~~~~~~~~~~~!7!~~!!!!777777777:....:::^^::.:!!!!~~~~~~^^~~~~~!!!!!!!!!!7777777??????????~....
// !???JJJJYYYYYYYYYYYYYJJJJJJJJJ???777777777!!!!!!!!!!!!!!!777777777??7^^^^^:::^~~^::^^^^::..................~~~~~~~~~~~~~!!!!!!!777777777?^...~!~~!?~...!!!!~~~~~~^~~~~~!!!!!!!!!!7777777???????????~....
// 7JJJYYYYYYYYYYYYYYYYYYJJJJJJJJ???777777777777777777!!!!!7777777777?7!~!!~^^::^~~~^:::::....................~^^^^^^~~~!!!777!!77777777777!:...!77777:...~!!!~~~~~~~~~~~!!!!!!!!!!!77777777??????????~:...
// J5555555555YYYYYYYYYYYYJJJJJJJJ???77777777777???7777!!!!!777777??????77!~~^^^^~!77!!!!~::..................::^^~~~~~7YY?!!!!!!!!!!!!!!!!!^...^~^^~!:...~!~~~~~^^^^~~~~~~~~!~~~!!!777777777????????7~:...
// J55555555555YYYYYYYYJJJJJJJJJJ?????7777777777777!!!!!!~~~!!7777????JJ?!~~^^^^~!7JJYYJJ7!~:....:...............^~~~~~!Y5J~~!!!!!!!!!!!~!~~:...^:...:...:~~~~^^^^::^^^~~~~~~~~~~~~!!!!777!!77!!!777!!!:...
// JPPPP55555YYYYJJJJJJJJJJJJ??????7777777!!!!!!!!!!~~~~~~~~~~!777???JJJJ7!~~~~~!!7JY5555YJJ7^:.:~~...............:^~~~~!!!~~~~~~~~~~!!!!~~~~::.........^~~^^^^^:::::^^~~~~~~~^^^~~~~~~~~~~~!!!!!!77!!!:...
// JYYYYYYYYYJJJJJ??????????77777!!!!!!!!!!!!~~~~~~~~~~~~^^~~~!!7???JJJJJJ?7!~~~~!77?JY55YYY?~::::~^..............::^^^^^^^^^^^~~~~~~~~~~~~~~~^::.....:^~~^^^:::::::::^^~~~~~~~~~~~~~~~~~~~~!!!!!!!!!!^....
// !?7777???????7777777777!!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!7?JJJJJJ?????!~^^~~!!~!????JJJ?!^:::^~............:!?^:^^^^^^^^^~~~~~~~~~~~~~~~~^::...:^^^^^::::::::::::^^~~~~~~~~~~~~~~~~~~~!!!!!!!!!!^....
// !7777!7777777777!!!!!!!!!~~~~~~^~~~~~~~~~~~~~^~~~~~~~~~~~!!7??JJJJJ?77??7!~^~~~^^^~!~~^^~~~~!~^^:^^...........::::^^^^^^^^^^^^^~~~~~~~~~~~^^:.....:^^::::::::...::::^^^~~~~~~~~~~~~~~~~~~!!!!!!!!!~:....
// !7777!7!!77!7!!7!!!!!!!!!!~~~~~~^^~~~~~~~~~~~^~~~^^^^~~!!7?JJJJJ???77777!~~~~~^^:^~~^^:::::^^~~~~^:.........:::..^^~~^^^^^^^^^^~~~~~~~~~~~^^:..::::^^^^:::::::::::::^^^^^^~~~~~~~~~~~~~~~~~~!!!!!!~:....
// !7777777777!!!!!!!!!!!!!!!~~~~~~~~~~~~~~~~~~^^~~~^^^~~!!7?Y55Y????????77!~!~~~~~^^^^^^:::::^^^~~~^^::::^:....::::^~~~~~~~~~~~~~~~~~~~~~~~~^::..::::^^^^^^^:::::::::^^^^^^^^^^^^^~~~~~~~~~~~~~~~~~~^:....
// !7777777777777777777!!!!!!!~~~~~~~~~~~~~~~~~^^^^^^~~!7?J???JJ?7777777777!^^^^~!!!~^^:::::::::::::^^^^:^~::...:~^:~~~~~~~~~~~~~~~~~~~~~~~~^:....:^^^^.:^^^^^::::::^^^^^^^^^^^^^^^^^~~~~~~~!!!!!!!!~~^::..
// !77777777777777777777!!!!!!!!!!!~~~~~~~~~~~~~~~~^~~!?JYJ?!!!!~~~!!!7777!~^:^^^~!~~^::::::::::::::^^^^:::::...::^~~~!!!!!!!!!~~~~~~~~~~~~~:.....::^^::.^~^^^^^:::^^^^^^^^^^^^^^^^^^^~~~~~!~!!!!!!!!~~^:..
// !7777777777777777777!!!!!!!!!!!~~~~~~~~~!!!!!!~~~~~!7777!~~~~~~~!!!7777!~^::^^~!7~^:::::^^:::::::::::::::::::::~77!!!!!!!!!!!!!!!!!!~~~~~:..:^^:^~^::.:^^^^^^^^^^^^^^^^^^^^^^^^^^^^~~~~~~!!!!!!!!!!~^^^:
// !77777777777777777!!!!!!!!!!~!!!!!!!!!!!!!!!!!!!!!!!!!!!~~~!!!!!!7777777~^:^^^~??7!^^:^~~^::::::^::::^^^^^^^^^^~!!!!!7777!!!!!!!!!!!!!!!~::.:~!~~~^::.:^^^^^^^^^^^^^^^^^^^^^^^^^^^~~~~~~!!!!!!!!!!!^:^^:
// !?7777777777777777!!!!!!7!!!!!!!7777777777777777777!!!!!!!!!!777777??JJ?^:^^^^~7JYJ?!~!!~^::::^!^:^^^^^^~~~~~~~~!!!!7777777!!!!!!!!!!!!!~::::^!~~~~:.:^~^^^^^^^^^^^^^^^^~~~~~~~~~~~~~~~~!!!!!!!!!!!::...
// 7????????7777777777!!!!77777777777777777???7777777777777777777777???777~^~!!~~~?Y555YJ7~~~^^^^~~^^^^~~~~~!!!!!!!7777777777777!!!!!!7777!7^::::::::^:.:^~^^^^^^^^^^^^^^~~~~~~~~~~~~~~~~~~!!!!!!!!!7!::..:
// 7JJ?JJJJ??????????777777??????????????????????????????77777777???!~~^^!7JYYYJ?YPPP555J7~~~~^^^^^~~~~!!!!!!7!!7??????????????777777777777?!::::..::...^!!~~~~~~~!!!!!!!!!!777777??????????????77???7~:...
// 7JJJJJJJJJJJJJJJJJJJ??????JJJ???????JJJJJJJJJ???JJJ???????????JJ?~^^^~7JY5555GBBGPPPPY7~~~^^^^~~~~!!!!!!7777JYYYJJJJYJJJJJJJJJJJJ????????J!~^:::::::~7?77??????????????JJJJJJJJJJJJJJJJJJJJJJJ??J??~::..
// ?YYJJYYYYYYYYYYYYYYYYYYYJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJYYYYJ~:^~7?Y5PGB&&#P55PP5J7777!~^^~~~~!!!!777JY555YYYY55YYYYYYYJJJJJJJJJJJYYYYJ!^^::::7JJJJJJJJJJJJJJJJJYYYYYYYYYYYYYYYYYYJJJJJJJJJJJJ7^:::
// ?YYYYYYY5555555555YYYYYYYYYYYYYYYYYYYYYYYJJJJJJJJJJJJJJJJJJJYY55555J7!!7J5P#&&@#555PPPP555YY?7!~~~~~!7JY5555555555555555555YYYYYYYYYYYYYYYYY7^^^^::!YYYYYYYYYYYYYYYYY555555555555YYYYYYYYYYYYYYYYYYJ^:::
// J555555555555555555555555555555555YYYYYYYYYYYYYYYYYYYYYYYYYYYY55555P57~7J5G&@@@BP55PPPPGPP55YJ?7!!!77Y55555555555555555555555555555555555555?::^^^^!5YYYYYYYYYY55555555555555555555555555555555Y55Y!::::
// J55555555555555555555555555555555555555555YYYYYYYYYYYYYYYYYYY555PPPPY??Y5P#&@@&GPPPPGGGGPP5YJJJ?7!!JPPP5555555555555555555555555555555555555!:::^^^~?555555555555555555555555555555555555555555555Y^.:::
// Y55555555P55555555555555555555555555555555555555YY55YYYY5YY5555PPPPPPPPP5B&&&&BGPPGGGGGGPP55YYYJ7!7?J7?Y5PPPP555555555555555PPPPPPPPPPP555?^::::^^~^:~Y55555555555555PPPPPPPPPPP555555555555555555J^:::~
// YPPP55PPPPPPPPPPPPP5555PPPPP555555555555555555555555555555555555PPPPPPGPP#&&&BPPGGGGGGGGGPPPP5J7!!!!!!!!!?Y5PPPPPPPPPPPPPPPPPPPPPPPPPPPPPJ^:^^:^^~~~^:~5PP55555PPPPPPPPPPPPPPPPPPPPPPPPP5555555555?^:::!
// YPPPPPPPPPPPPPPPPPPPPPPPPPPPP55P555555555555555555555555555555555PPP5YYY5B&&&PY5PGGGGGGGGPPP5Y?!!!!!7!!!!~~~!7J55555PPPPPPPPPPPPPPPPPPPPP7^^^~!^^!7!^^^YPPPP55PPPPPPPPPPPPPPP5?777?JJJJJJYYYY55555J^:::~
// YPPPPPPPPPPPPPPPPPPPPPPPPPP555PPP5555555555555555555555555555555555J7777?Y###Y?YPPPPGGGPPP555?7!!~~!7!!!!~~^^^^~J55PPPPPPPPPPPPPPPPPPPPPP7^^^7PYJPP!^^~YPPPPPPPPPPPPPPPPPPPP57^^^^^^^^^^^^^^~!?YYY?~^^::
// YPPPPPPPPPPPPPPPPPPPPPPPPPP555555PP55555555555555555555555555555555J7!!!!7G#B??5PPPPGGGP5YJYJ7!!~~~!!!!!~~~^^^!J5PPPPPPPPPPPPPPPPPPPPPPPP?^~^!PPPPP!^~~YPPPPPPPPPPPPPPPPPPP55?~~~~~~~~~^^^^^^^^^^^^~~^^:
// YPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP5P55555555555555555555555555555555Y?77!!!P#B??5PPPPGPP5J?77!!~~~~~~~!!~~^^^~75PPPPPPPPPPPPPPPPPPPPPPPPPPJ~~~~!~^!Y!^~~5PPPPPPPPPPPPPPPPPPP55Y!~~~~~~~~~~~~^^^^^^^^~~~~^
// 5PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP55555555555PPPPP5555555555PPPP5?77!!7PBBY7JPPPPPPYJ?!!!~~~~~~~~!!!!~~~!J5PPPPPPPPPPPPPPPPPPPPPPPPPPPY~!~~^^^^^^^~?PPPPPPPPPPPPPPPPPPPP555J?7~~~~~~~~~~^^^^^^^^~~!~~
// 5PPPPPPGGGGGPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP5555PPPPPPPPPPPPPP5555PPP57!7777PBBPYJ5PPPP5YJ7~~~~~~~!!~!7777777JPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPJ!~~~^^^~~!JPPPPPPPPPPPPPPPPPPPPPPPPPP5?7!~~~~~~~~^^^^^^^~~~~~
// 5PPPPPPPGPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP555Y7!777?5GP55?Y555YYJ?!!!~~~~~!!!!7777777J55PPPPPPPPPPPPPPPPPPPPPPPPPPPPP5?~~~~~~75PPPPPPPPPPPPPPPPPPPPPPPPPPP55Y7~~~~~~~~^^^^^^~~~~~~
// 5GGPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP5PPPPPPP55?777777PG5J5?JYYJJJJ?!~~~~~~~~!!!777777!?PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPGY!!!!!!JGPGPPPPPPPPPPPPPPPPPPPPPPPPPPP557!~~^^^^^^^^^^~~^^^^
void terminatePaczka(const char* auth, const char* uuid) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    json_t *data = json_object();
    json_object_set_new(data, "sessionUuid", json_string(uuid));
    char *data_json = json_dumps(data, JSON_INDENT(2));
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/terminate", data_json, headers, &terminate_paczka, false);
    free(data_json);
}

// sprawdź czy skrytka została actually zamknięta
void jaktamSkrytka(const char* auth, const char* uuid, bool otwarta) {
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[openPaczkomat] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    headers = curl_slist_append(headers, authheader);

    json_t *data = json_object();
    json_object_set_new(data, "sessionUuid", json_string(uuid));
    if (otwarta) {
        json_object_set_new(data, "expectedStatus", json_string("OPENED"));
    } else {
        json_object_set_new(data, "expectedStatus", json_string("CLOSED"));
    }

    char *data_json = json_dumps(data, JSON_INDENT(2));
    queue_request("https://api-inmobile-pl.easypack24.net/v1/collect/compartment/status", data_json, headers, &jak_tam_skrytka, false);
    free(data_json);
}

void getPaczkomatImage(const char* url) {
    if (!url || strlen(url) == 0) return;

    
    obrazekdone = false;

    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");

    
    
    queue_request(url, NULL, headers, &paczkomat_image, true);
}



void getEverything(const char* auth) {
    doing_debug_logs = true;
    log_to_file("LOGI ZBIORCZE (DEBUG PURPOSES ONLY)\n");
    struct curl_slist *headers = NULL;

    size_t auth_len = strlen("Authorization: ") + strlen(auth) + 1;
    char *authheader = malloc(auth_len);
    if (!authheader) {
        log_to_file("[getPaczkas] ERROR: malloc failed");
        return;
    }
    snprintf(authheader, auth_len, "Authorization: %s", auth);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authheader);
    headers = curl_slist_append(headers, "User-Agent: InPost-Mobile/3.46.0(34600200) (Horizon 11.17.0-50U; AW715988204; Nintendo 3DS; pl)");
    log_to_file("[API] Paczki do ciebie:\n");
    refresh_data("https://api-inmobile-pl.easypack24.net/v4/parcels/tracked", NULL, headers, &paczkas);
    log_to_file("[RESPONSE] %s\n", paczkas.data);
    log_to_file("[API] Paczki od ciebie:\n");
    refresh_data("https://api-inmobile-pl.easypack24.net/v4/parcels/sent", NULL, headers, &sent_paczkas);
    log_to_file("[RESPONSE] %s\n", sent_paczkas.data);
    log_to_file("[API] Paczki zwrócone:\n");
    refresh_data("https://api-inmobile-pl.easypack24.net/v1/returns/tickets", NULL, headers, &returned_paczkas);
    log_to_file("[RESPONSE] %s\n", returned_paczkas.data);
    log_to_file("[API] Dane użytkownika:\n");
    refresh_data("https://api-inmobile-pl.easypack24.net/izi/app/shopping/v2/profile", NULL, headers, &dane_usera);
    log_to_file("[RESPONSE] %s\n", dane_usera.data);
    doing_debug_logs = false;
}