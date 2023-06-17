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

bool passwordChanged = false;

//----

WiFiInterface *wifi;


//--------MQTT setup




std::string botsTopicString = "IotLock/bot";   

char* botsTopic = "IoTLock/bot";
char* lockTopic = "IoTLock/lock";
char* passwordTopic = "IoTLock/password";

char* ipBrocker = "185.213.2.121";
char* hostname = "https://ru.flespi.io/mqtt/";

char* openMessage = "1";

char* paswword = "1111";

int passwordCount = 0;

int opencount = 0;

char* mes = "1883";

int port = 1883;

MQTT::Message messageOpen;

MQTT::Message messagePassword;


MQTT::Message message = {
    .qos = MQTT::QOS1,
    .retained = true,
    .payload = openMessage,
    .payloadlen = strlen(openMessage)
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
        false;
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

void _checkPasswrod(){
    char* pass = new char[4];
    std::string passw;
    int count = 0;
    while (true){
        passw = "";
        for(int  i = 0; i < 4; i++){
            if(!isOpen && !passwordChanged){
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
        if(strlen(paswword) < 4){
            continue;
        }
        for(int i = 0; i < 4; i++){
            if (paswword[i] != passw.at(i)){
                passwordChanged = false;
                break;
            }
        }
    }
}

 
void handlerOpen(MQTT::MessageData& md)
{
    messageOpen = md.message;
    char* data = (char*)messageOpen.payload;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", messageOpen.qos, messageOpen.retained, messageOpen.dup, messageOpen.id);
    printf("Payload %.*s\r\n", messageOpen.payloadlen, data);
    printf("bots lenth = %d\r\n", strlen(data));
    printf("open message length = %d\r\n", strlen(openMessage));
    printf("==========\r\n");
    for(int i = 0; i < strlen(data); i++){
        printf("data %d %d\r\n", i, (int)data[i]);
    }
    printf("==========\r\n");
    if (*data == *openMessage){
        isOpen = true;
        printf("lock open\r\n");
    }else{
        isOpen = false;
    }
    ++opencount;
}

void handlerPassword(MQTT::MessageData& md){
    messagePassword = md.message;
    char* data = (char*)messagePassword.payload;
    if (!isEquals(paswword, data)){
        printf("pasword change\n\r");
        passwordChanged = true;
        paswword = data;
    }
    ++passwordCount;
}

void sendLockTopic(NetworkInterface *net){
    MQTTNetwork network(net);
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(network);

    int rc = network.connect(ipBrocker, port);
    printf("rs network = %d\r\n", rc);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;
    data.clientID.cstring = "tester";
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
        if(isOpen && isOpenByPassword){
            rc = client.publish(lockTopic, message);
            if(rc == 0){
                printf("message by open password\r\n");
            }
        }    
    }
}

 
// void mqtt_demo(NetworkInterface *net)
// {
//     float version = 0.6;
//     char* topic = "test1230984";
//
//     MQTTNetwork network(net);
//     MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(network);
// 
//     char* hostname = "193.193.165.41";
//     int port = 1883;
// 
//     printf("Connecting to %s:%d\r\n", hostname, port);
//
//     int rc = network.connect(hostname, port);
// 
//     if (rc != 0)
//         printf("rc from TCP connect is %d\r\n", rc);
//     printf("Connected socket\n\r");
//
//     MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
//     data.MQTTVersion = 3;
//     data.clientID.cstring = "2a617592b917";
//     data.username.cstring = "testuser";
//     data.password.cstring = "testpassword";
//     if ((rc = client.connect(data)) != 0)
//         printf("rc from MQTT connect is %d\r\n", rc);
// 
//     if ((rc = client.subscribe(topic, MQTT::QOS0, handlerOpen)) != 0)    //was QOS2
//         printf("rc from MQTT subscribe is %d\r\n", rc);
//
//     while (arrivedcount < 3)
//         client.yield(100);
//
//     if ((rc = client.unsubscribe(topic)) != 0)
//         printf("rc from unsubscribe was %d\r\n", rc);
//
//     if ((rc = client.disconnect()) != 0)
//         printf("rc from disconnect was %d\r\n", rc);
//
//     network.disconnect();
//
//     printf("Version %.2f: finish %d msgs\r\n", version, arrivedcount);
//
//     return;
// }


void checkOpen(NetworkInterface *net){
    MQTTNetwork network(net);
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(network);

    int rc = network.connect(ipBrocker, port);
    printf("rs network = %d\r\n", rc);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;
    data.clientID.cstring = "tester";
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

    MQTT::Message mess = {
        .qos = MQTT::QOS1,
        .retained = true,
        .payload = mes,
        .payloadlen = strlen(mes)
    };

    rc = client.subscribe(botsTopic, MQTT::QOS1, handlerOpen);
    if(rc == 0){
        printf("supscribe sucsess\r\n");
    } else {
        printf("supscribe error\r\n");
    }
    printf("rs client subscribe = %d\r\n", rc);


//delete 
    rc = client.publish(lockTopic, mess);
    if(rc == 0){
        printf("publish sucsess\r\n");
    }
    printf("rs publish = %d\r\n", rc);
//
    while (true)
        client.yield(100);
}

void checkPassword(NetworkInterface *net){
    MQTTNetwork network(net);
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(network);

    int rc = network.connect(ipBrocker, port);
    printf("rs network = %d\r\n", rc);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;
    data.clientID.cstring = "tester";
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

    rc = client.subscribe(passwordTopic, MQTT::QOS1, handlerPassword);
    if(rc == 0){
        printf("supscribe sucsess\r\n");
    } else {
        printf("supscribe error\r\n");
    }
    printf("rs client subscribe = %d\r\n", rc);
    while (true){
        client.yield(100);
    }
    
}





int main(){

    printf("WiFi example\n");
    init();
    Rele.write(1);
    thread_sleep_for(1000);
    Rele.write(0);

    char* first = "wer";
    char* second = "wer";
    Thread openLock;
    openLock.start(_releFunc);

    printf("========%d\r\n", isEquals(first, second));

    Thread passwordOpen;

    passwordOpen.start(_checkPasswrod);



    checkOpen(wifi);

    

    // MQTT::Client<MQTTNetwork, Countdown> client;
    // client = _getClient(wifi);

// // #ifdef MBED_MAJOR_VERSION
// //     printf("Mbed OS version %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
// // #endif


    wifi->disconnect();

    printf("\nDone\n");
}