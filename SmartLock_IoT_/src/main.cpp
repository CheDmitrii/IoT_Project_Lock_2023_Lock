#include "mbed.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"

#define MQTTCLIENT_QOS2 1


//----


//TX-D2
//RX-D8

DigitalOut Rele(A2);
DigitalOut Led(LED2);
DigitalIn ButtonS1(A4);
DigitalIn ButtonS2(A5);
DigitalIn ButtonS3(D2);
DigitalIn ButtonS4(D7);

bool isOpen = false;
bool isOpenByPassword = false;
bool isSendetOnLockTopic = false;
bool isButtonClose = false;
bool passwordChanged = false;


WiFiInterface *wifi;


//--------MQTT setup

std::string botsTopicString = "IotLock/bot";   

char* botsTopic = "IoTLock/bot";
char* lockTopic = "IoTLock/lock";
char* passwordTopic = "IoTLock/password";

char* ipBrocker = "185.213.2.121";
char* hostname = "https://ru.flespi.io/mqtt/";

char* openMessage = "1";
char* closeMessage = "0";

char* paswword = "2222";

int port = 1883;

MQTT::Message messageHandlerOpen;

MQTT::Message messageHandlerPassword;


MQTT::Message messageOpen = {
    .qos = MQTT::QOS1,
    .retained = true,
    .payload = openMessage,
    .payloadlen = strlen(openMessage)
};

MQTT::Message messageClose = {
    .qos = MQTT::QOS1,
    .retained = true,
    .payload = closeMessage,
    .payloadlen = strlen(closeMessage)
};

//--------

DigitalIn button(USER_BUTTON);


bool isEquals(char* first, char* second){
    if(strlen(first) == strlen(second)){
        for(int i = 0; i < strlen(first); i++){
            if (first[i] != second[i]){
                return false;
            }
        }
        return true;
    }
    return false;
}

bool isEqualsPassword(char* handledPassword){
    if(strlen(handledPassword) < 4){
        return false;
    }
    if(strlen(paswword) == 0){
        return false;
    }
    for(int i = 0; i < 4; i++){
        if (paswword[i] != handledPassword[i]){
            return false;
        }
    }
    return true;
}

bool isEqualsPassword(std::string str){
    if(str.size() < 4 || strlen(paswword) < 4){
        return false;
    }
    for(int i = 0; i < 4; i++){
        if(str.at(i) != paswword[i]){
            return false;
        }
    }
    return true;
}

bool isPassword(char* handlerPassword){
    if(strlen(handlerPassword) < 4 || strlen(handlerPassword) == 0){
        return false;
    }
    for(int i = 0; i < 4; i++){
        if(handlerPassword[i] < '0' || handlerPassword[i] > '9'){
            return false;
        }
    }
    return true;
}

const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}

int scan_demo(WiFiInterface *wifi)
{
    WiFiAccessPoint *ap;

    printf("Scan:\n");

    int count = wifi->scan(NULL,0);

    if (count <= 0) {
        printf("scan() failed with return value: %d\n", count);
        return 0;
    }

    /* Limit number of network arbitrary to 15 */
    count = count < 15 ? count : 15;

    ap = new WiFiAccessPoint[count];
    count = wifi->scan(ap, count);

    if (count <= 0) {
        printf("scan() failed with return value: %d\n", count);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\n", ap[i].get_ssid(),
               sec2str(ap[i].get_security()), ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
               ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5], ap[i].get_rssi(), ap[i].get_channel());
    }
    printf("%d networks available.\n", count);

    delete[] ap;
    return count;
}

void init(){
    ButtonS1.mode(PullUp);
    ButtonS2.mode(PullUp);
    ButtonS3.mode(PullUp);
    ButtonS4.mode(PullUp);

    wifi = WiFiInterface::get_default_instance();

    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\n");
        return;
    }
    int count = scan_demo(wifi);
    if (count == 0) {
        printf("No WIFI APs found - can't continue further.\n");
        return;
    }
    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\nConnection error: %d\n", ret);
        return;
    }
    printf("Success\n\n");
    printf("MAC: %s\n", wifi->get_mac_address());
    SocketAddress a;
    wifi->get_ip_address(&a);
    printf("IP: %s\n", a.get_ip_address());
    wifi->get_netmask(&a);
    printf("Netmask: %s\n", a.get_ip_address());
    wifi->get_gateway(&a);
    printf("Gateway: %s\n", a.get_ip_address());
    printf("RSSI: %d\n\n", wifi->get_rssi());
}

void _releFunc(){
    while (true){
        if(isOpen){
            Rele.write(1);
            thread_sleep_for(10);
        }else{
            Rele.write(0);
            thread_sleep_for(10);
        }
    }
}

bool isClick(DigitalIn button){
  int firstTime = get_ms_count();
  int secondTime = 0;
  while (true){
    if (button == 1){
      secondTime = get_ms_count();
      break;
    }
  }
  if (secondTime - firstTime >= 100){
    return true;
  }else{
    return false;
  }
}

int whatButton(){
    while(true){
        if(ButtonS1 == 0){
            if (isClick(ButtonS1)){
                return 1;
            }
        }
        if(ButtonS2 == 0){
            if (isClick(ButtonS2)){
                return 2;
            }
        }
        if(ButtonS3 == 0){
            if (isClick(ButtonS3)){
                return 3;
            }
        }
        if(ButtonS4 == 0){
            if (isClick(ButtonS4)){
                return 4;
            }
        }
    }
}

void whatButtonVoid(){
    while(isOpen){
        if(ButtonS1 == 0){
            if (isClick(ButtonS1)){
                isButtonClose = true;
                isOpen = false;
                return;
            }
        }
        if(ButtonS2 == 0){
            if (isClick(ButtonS2)){
                isButtonClose = true;
                isOpen = false;
                return;
            }
        }
        if(ButtonS3 == 0){
            if (isClick(ButtonS3)){
                isButtonClose = true;
                isOpen = false;
                return;
            }
        }
        if(ButtonS4 == 0){
            if (isClick(ButtonS4)){
                isButtonClose = true;
                isOpen = false;
                return;
            }
        }
    }
}

void _checkPasswrod(){
    char* pass = new char[4];
    std::string passw;
    while (true){
        passw = "";
        if(isOpen && isOpenByPassword){
            whatButtonVoid();
            isOpenByPassword = false;
            continue;
        }
        if(isOpen){
            continue;
        }
        if(strlen(paswword) < 4){
            whatButtonVoid();
            printf("password isn't valid. Change password in bot\r\n");
            continue;
        }
        for(int  i = 0; i < 4; i++){
            if(!isOpen){
                switch (whatButton()){
                case 1:
                    passw += "1";
                    printf("was pressed 1\r\n");
                    break;
                case 2:
                    passw += "2";
                    printf("was pressed 2\r\n");
                    break;
                case 3:
                    passw += "3";
                    printf("was pressed 3\r\n");
                    break;
                case 4:
                    passw += "4";
                    printf("was pressed 4\r\n");
                    break;
                default:
                    break;
                }
            }else{
                passw = "";
                passwordChanged = false;
                break;
            }
        }
        printf("enter password ====== %s\r\n", passw.c_str());
        if(isEqualsPassword(passw)){
            isOpen = true;
            isOpenByPassword = true;
            isSendetOnLockTopic = false;
        }else{
            printf("password isn't right\r\n");
            if(passwordChanged){
                printf("becouse password is changed\n try again\r\n");
            }
            printf("try again\r\n");
        }
    }
}

void handlerOpen(MQTT::MessageData& md)
{
    messageHandlerOpen = md.message;
    char* data = (char*)messageHandlerOpen.payload;
    printf("Payload %.*s\r\n", messageHandlerOpen.payloadlen, data);
    printf("bots lenth = %d\r\n", strlen(data));
    printf("open message length = %d\r\n", strlen(openMessage));

    if (*data == *openMessage){
        isOpen = true;
    }else{
        isOpen = false;
    }
}

void handlerPassword(MQTT::MessageData& md){
    messageHandlerPassword = md.message;
    char* data = (char*)messageHandlerPassword.payload;
    printf("starrt handler\r\n");
    printf("data => %s\r\n", data);
    if (!isEqualsPassword(data) && isPassword(data)){
        printf("pasword change\r\n");
        passwordChanged = true;
        std::string str = {data[0], data[1], data[2], data[3]};
        paswword = new char[str.size() + 1];
        strcpy(paswword, str.c_str());
        printf("new password ==>> %s\r\n", paswword);
    }
}

void sendLockTopic(){
    MQTTNetwork network(wifi);
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(network);

    int rc = network.connect(ipBrocker, port);
    printf("rs network = %d\r\n", rc);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;
    data.clientID.cstring = "nonadmin";
    data.username.cstring = "aOKHDLpdeDKOtCFR37k9GtgJGmyci5mhIVO9u1dzvLbOeGw7DE48Y8Hsk3urWFys";
    data.password.cstring = "123";
    data.cleansession = true;
    rc = client.connect(data);
    if(rc == 0){
        printf("CONECT to user\r\n");
    }else{
        printf("conect to user error\r\n");
    }
    printf("rs client = %d\r\n", rc);

    while(true){
        if(isOpen && isOpenByPassword && !isSendetOnLockTopic){
            printf("start sending\r\n");
            rc = client.publish(lockTopic, messageOpen);
            printf("publish by open state of lock => %d\r\n", rc);
            if(rc == 0){
                printf("message by open password\r\n");
                isSendetOnLockTopic = true;
            }
        }
        if(!isOpen && isButtonClose && isSendetOnLockTopic){
            rc = client.publish(lockTopic, messageClose);
            if (rc == 0){
                printf("message by close password\r\n");
                isButtonClose = false;
                isSendetOnLockTopic = false;
            }
        }
    }
}

void checkOpen(NetworkInterface *net){
    MQTTNetwork network(net);
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(network);

    int rc = network.connect(ipBrocker, port);
    printf("rs network = %d\r\n", rc);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;
    data.clientID.cstring = "testerOpen";
    data.username.cstring = "aOKHDLpdeDKOtCFR37k9GtgJGmyci5mhIVO9u1dzvLbOeGw7DE48Y8Hsk3urWFys";
    data.password.cstring = "123";
    data.cleansession = true;
    rc = client.connect(data);
    if(rc == 0){
        printf("CONECT to user\r\n");
    }else{
        printf("conect to user error\r\n");
    }
    printf("rs client = %d\r\n", rc);


    rc = client.subscribe(botsTopic, MQTT::QOS1, handlerOpen);
    if(rc == 0){
        printf("supscribe sucsess\r\n");
    } else {
        printf("supscribe error\r\n");
    }
    printf("rs client subscribe = %d\r\n", rc);

    while (true)
        client.yield(10);
}

void checkPassword(){
    MQTTNetwork network(wifi);
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(network);

    int rc = network.connect(ipBrocker, port);
    printf("rs network = %d\r\n", rc);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;
    data.clientID.cstring = "notadmin";
    data.username.cstring = "aOKHDLpdeDKOtCFR37k9GtgJGmyci5mhIVO9u1dzvLbOeGw7DE48Y8Hsk3urWFys";
    data.password.cstring = "123";
    data.cleansession = true;
    rc = client.connect(data);
    if(rc == 0){
        printf("check password CONECT to user\r\n");
    }else{
        printf("check password conect to user error\r\n");
    }
    printf("rs check password client = %d\r\n", rc);

    rc = client.subscribe(passwordTopic, MQTT::QOS1, handlerPassword);
    if(rc == 0){
        printf("check password supscribe sucsess\r\n");
    } else {
        printf("check password supscribe error\r\n");
    }
    printf("rs check password client subscribe = %d\r\n", rc);
    while (true){
        client.yield(10);
    }
    
}



int main(){
    printf("WiFi Searching\n");
    init();

    Thread openLock, passwordButton, passwordTopic, sendTopic;

    openLock.start(_releFunc);

    passwordButton.start(_checkPasswrod);

    passwordTopic.start(checkPassword);

    sendTopic.start(sendLockTopic);

    checkOpen(wifi);

    wifi->disconnect();

    printf("\nDone\n");
}