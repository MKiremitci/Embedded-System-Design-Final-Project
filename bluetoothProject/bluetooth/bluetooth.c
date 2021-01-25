/* XDCtools Header files */
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/std.h>


/* TI-RTOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Idle.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/drivers/GPIO.h>
#include <ti/net/http/httpcli.h>
#include <ti/drivers/UART.h>

/* Example/Board Header file */
#include "Board.h"
#include <sys/socket.h>
#include <sys/stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define TASKSTACKSIZE     4096
#define SOCKET_IP         "192.168.1.38" // local IP adress
#define SOCKET_PORT       6011           // TCP Server port
#define NTP_IP            "132.163.96.5" // NTP Server IP adress
                          // 132.163.96.3 // some available NTP server IP address
                          // 132.163.97.5
                          // 128.138.140.44

extern Mailbox_Handle mailbox0;
extern Swi_Handle swi0;
extern Mailbox_Handle mailbox1;
extern Event_Handle event0;
extern Event_Handle event1;

// variables for taking timestamp and acquire datatimes from NTP server
char receivedSecond[4];                              // holds the taken values
unsigned long int allSeconds;           // holds the number of second since 1900
struct tm * ts;                                 // holds the sub-elements of taken values
char dummy[100];                                // holds sended values from recvUART
int year, month, day, hour, minute, second;
bool printTime = false;
char input[100];                                 // holds the recieved data from terminete
const char ledOpen[] = "Led opened!";


/*
 * Hardwire Timer Interrept
 * It create a signal in every second to post SWI to make it update time in seconds
 */
Void timerHWI(UArg arg1){
    Swi_post(swi0);
}

/*
 * Software Interrupt
 * Update the time that taken from NTP Server by recvNTP task
 * In order to make easy, I didn't add the month recovery where we should check day is 30 or not
 * If it is 30 make it 0, when incremented 23 to 24.
 */
Void updateTime(UArg arg1, UArg arg2){
    if (second != 59){              // if second is 59 it means that we should update the minute
            second++;
    } else {
        if(minute != 59) {          // if minute is 59 it means that we should update the hour
            second = 0;
            minute++;
        } else {
            if(hour != 23){         // if hour is not 23, increment the hour
                minute = 0;
                second = 0;
                hour++;
            }
        }
    }

    // In order to control print of date, we use printTime
    if(printTime){
        System_printf("SWI-> hour: %d minute: %d second: %d \n", hour,  minute,second);
        System_flush();
    }
}

/*
 *  Task name recvUART
 *  Parameters arg0 and arg1, where they are unnecessary in this situation
 *  1. Takes the transmitted data from bluetooth
 *  2. Take the updated time which they are global variable
 *  3. Concatenate the taken datas
 *  4. Sends it to the sendData task with mailbox
 */
Void recvUART(UArg arg0, UArg arg1)
{
    // wait until event1 is posted
    Event_pend(event1, Event_Id_00, Event_Id_NONE, BIOS_WAIT_FOREVER);

    System_printf("recvUART\n"); // give information to the console
    System_flush();

    char temporal[100] = ""; // used for concatenate the datas

    // Initializing the UART
    UART_Handle uart;
    UART_Params uartParams;

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    //  Data is not processed
    uartParams.writeDataMode = UART_DATA_BINARY;
    // Data is processed according to readReturnMode
    // In order to take line by line
    uartParams.readDataMode = UART_DATA_TEXT;
    // Returns when buffer is full or newline was read
    uartParams.readReturnMode = UART_RETURN_NEWLINE;
    //  Data is not echoed
    uartParams.readEcho = UART_ECHO_OFF;
    // Baud rate for UART
    uartParams.baudRate = 9600;
    // Function to initialize the UART_Params struct to its defaults
    uart = UART_open(Board_UART0, &uartParams);

    // check the initializing is successful or not
    if (uart == NULL) {
        System_abort("Error opening the UART");
    }

    /* Loop forever */
    while (1) {
        // Function that reads data from a UART without interrupts.
        // readedByte -> total number of byte that readed
        int readedByte = UART_read(uart, &input, 100);
        if (readedByte > 1) { // if data is received, do concatenation
            char message[100] = ""; // holds the concatenated data

            /*
             *  1. Put taken data from UART (input) to the datatime
             *  2. Put year + 1900 to end of the message (year is beginning from 1900)
             *  3. Put month+1 to end of the message (month is beginning from January,
             *     normally it returns 0, but I prefer to name January as 1)
             *  4. Put week day to end of the message (weekday is beginning from Sunday,
             *     normally it returns 0, but I prefer to name Sunday as 1)
             *  5. Put hour to end of the message
             *  6. Put minute to end of the message
             *  7. Put second to end of the message
             */
            strcat(message, input);
            sprintf(temporal, "(Y:%d-", year+1900);
            strcat(message, temporal);
            sprintf(temporal, "Mo:%d-", month+1);
            strcat(message, temporal);
            sprintf(temporal, "WD:%d-", day+1); //since sunday
            strcat(message, temporal);
            sprintf(temporal, "H:%d-", hour);
            strcat(message, temporal);
            sprintf(temporal, "Mi:%d-", minute);
            strcat(message, temporal);
            sprintf(temporal, "S:%d)\n", second);
            strcat(message, temporal);

            //System_printf("%s", message);
            //System_flush();

            //GPIO_toggle(Board_LED1);

            // In order to pass data between tasks, I used mailbox concept
            // This takes the message register adress to the pended location
            Mailbox_post(mailbox0, &message, BIOS_NO_WAIT);
        }

        readedByte = 0; // clear the readedByte for new read of UART
        bzero(input, sizeof(input)); // clear the input for new read of UART
    }
}

/*
 *  Prints the error, when tasks are not handled succesfully
 *  and make BIOS exit.
 */
void printError(char *errString, int code)
{
    System_printf("Error! code = %d, desc = %s\n", code, errString);
    System_flush();
    BIOS_exit(code);
}


/*
 *  This function takes timestamp from NTP server
 *   serverIP -> the ip adress of NTP server
 *   serverPort -> the port of the ip adresses that related to timestamp
 */
void recvTimeStamptFromNTP(char *serverIP, int serverPort)
{

    System_printf("recvTimeStamptFromNTP start\n");
    System_flush();

    // initializing the socket for recieving timestampt value from NTP by using TCP
    int sockfd, connStat;
    char buf[80];
    struct sockaddr_in serverAddr;

    // TCP socket is created
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) { // if socket is not created successfully make BIOS exit.
        System_printf("Socket not created");
        BIOS_exit(-1);
    }


    memset(&serverAddr, 0, sizeof(serverAddr));  // clear serverAddr structure
    serverAddr.sin_family = AF_INET;     // declare the socket family
    serverAddr.sin_port = htons(37);     // convert port number to network order
    inet_pton(AF_INET, serverIP , &(serverAddr.sin_addr));

    // connect to the port of the server adress
    connStat = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    // check connection is done or not
    if(connStat < 0) {
        System_printf("sendData2Server::Error while connecting to server\n");
        if(sockfd>0) close(sockfd);
        BIOS_exit(-1);
    }

    // take the response from connected NTP server
    // fill the receivedSecond with the taken data
    recv(sockfd, receivedSecond, sizeof(receivedSecond), 0);
    allSeconds = receivedSecond[0]*16777216 +
                         receivedSecond[1]*65536 +
                         receivedSecond[2]*256 +
                         receivedSecond[3];

    // declare a calendar variable that used for converting seconds to calendar time
    time_t rawTime = allSeconds // number of second data that recieved from NTP
                     + 3*60*60;         // it equals to he 10800 seconds. Why I used that
                                        // I am living in Turkey and I couldn't find any running
                                        // NTP server in Turkey. There are some, but they did not
                                        // give response to me. Thus I used the NTP server where are
                                        // in NIST, Boulder, Colorado. This means that the taken second data
                                        // are three hour back with respect to the hour of Turkey
                                        // i.e. in Turkey time is 16:22 and NTP server gives me
                                        // 13:22. In order to make equal with my time I added to 10800 seconds

    ts = localtime(&rawTime);           // converts the second data to calendar date
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    System_printf("time: %s\n", buf);
    System_flush();

    // In order to prevent connection problems between NTP server and launcpad
    // I decided to take second data one time and control it in the Swi
    // Therefore, I take the structuring elements to the global elements
    year = ts->tm_year;
    month = ts->tm_mon;
    day = ts->tm_wday;
    hour = ts->tm_hour;
    minute = ts->tm_min;
    second = ts->tm_sec;

    if(sockfd>0) {
        close(sockfd);
    }

    printTime = true; // controls the print function of SWI
    System_printf("recvTimeStamptFromNTP end\n");
    System_flush();
}

/*
 * The task that responsible for recieving data from NTP server
 */
Void recvNTP(UArg arg0, UArg arg1)
{
    while(1) {
        // Event is pended. Task cannot continue until event posted by other tasks.
        Event_pend(event0, Event_Id_00, Event_Id_NONE, BIOS_WAIT_FOREVER);

        System_printf("recvNTP start\n");
        System_flush();

        // it shows that second data is taking by using recvTimeStamptFromNTP
        GPIO_write(Board_LED0, 1); // turn on the LED

        // calling function to take timestamp
        recvTimeStamptFromNTP(NTP_IP, 37);

        // it shows that second data is took by using recvTimeStamptFromNTP
        GPIO_write(Board_LED0, 0);  // turn off the LED
        Task_sleep(5000);
        System_printf("recvNTP end\n");
        System_flush();

        // Event is posted. It means that the process of recieving timestamp is done.
        Event_post(event1, Event_Id_00);
    }
}

/*
 *  This functin is used for sending message to the server
 *  serverIP -> server ip
 *  serverPort -> server port
 *  data -> the data to the send
 *  size -> size of the data to send
 */
void sendData2Server(char *serverIP, int serverPort, char *data, int size)
{
    System_printf("sendData2Server start\n");
    System_flush();

    int sockfd;
    struct sockaddr_in serverAddr;

    // initializing the socket for sending message from launcpad to server

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // TCP socket is creating
    if (sockfd == -1) {  // if socket is not created successfully make BIOS exit.
        System_printf("Socket not created");
        BIOS_exit(-1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));  // clear serverAddr structure
    serverAddr.sin_family = AF_INET;             // declare the socket family
    serverAddr.sin_port = htons(serverPort);     // convert port number to network order
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr));

    // connect to the port of the server adress
    int connStat = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    // check connection is done or not
    if(connStat < 0) {
        System_printf("Error while connecting to server\n");
        if (sockfd > 0)
            close(sockfd);
        BIOS_exit(-1);
    }

    // send the data to the TCP server
    // fill the receivedSecond with the taken data
    int numSend = send(sockfd, data, size, 0);

    // if the data is not send, there is a problem. Make BIOS exit.
    if(numSend < 0) {
        System_printf("Error while sending data to server\n");
        if (sockfd > 0) close(sockfd);
        BIOS_exit(-1);
    }
    if (sockfd > 0) close(sockfd);

    System_printf("sendData2Server end\n");
    System_flush();
}

/*
 * The task that responsible for sending data to the TCP server
 */
Void socketSend(UArg arg0, UArg arg1)
{
    while(1) {
        // In order to pass data between tasks, I used mailbox concept
        // This takes the message register adress from the posted location
        // and fills the dummy
        Mailbox_pend(mailbox0, &dummy, BIOS_WAIT_FOREVER);

        System_printf("socketSend start\n");
        System_flush();

        // it shows that message is sending by using sendData2Server
        GPIO_write(Board_LED0, 1); // turn on the LED

        // calling function to send message
        sendData2Server(SOCKET_IP, SOCKET_PORT, dummy, strlen(dummy)); //socket bizim IP, 5011 port,

        // it shows that message is sended by using sendData2Server
        GPIO_write(Board_LED0, 0);  // turn off the LED

        Task_sleep(5000);
        System_printf("socketSend end\n");
        System_flush();
   }
}

/*
 *  This is a hook that used for capturing a IP adresss
 *  This function is called when IP Addr is added/deleted
 */
void netIPAddrHook(unsigned int IPAddr, unsigned int IfIdx, unsigned int fAdd)
{
       System_printf("netIPAddrHook start\n");
       System_flush();

       // Here I declared three task and gave them same priority
       static Task_Handle taskHandle1, taskHandle2, taskHandle3;
       Task_Params taskParams;
       Error_Block eb;
       if (fAdd && !taskHandle1 && !taskHandle2 &&!taskHandle3) {
           Error_init(&eb);
       }

       Task_Params_init(&taskParams);
       taskParams.stackSize = TASKSTACKSIZE;
       taskParams.priority = 1;
       taskHandle1 = Task_create((Task_FuncPtr)socketSend, &taskParams, &eb);

       Task_Params_init(&taskParams);
       taskParams.stackSize = TASKSTACKSIZE;
       taskParams.priority = 1;
       taskHandle2 = Task_create((Task_FuncPtr)recvNTP, &taskParams, &eb);

       Task_Params_init(&taskParams);
       taskParams.stackSize = TASKSTACKSIZE;
       taskParams.priority = 1;
       taskHandle3 = Task_create((Task_FuncPtr)recvUART, &taskParams, &eb);

       // if there is problem about initializing of tasks, print error
       if (taskHandle1 == NULL && taskHandle2 == NULL && taskHandle3 == NULL) {
           printError("netIPAddrHook: Failed to create tasks\n", -1);
       }
       System_printf("netIPAddrHook end\n");
       System_flush();
}

/*
 *  ======== main ========
 */
int main(void)

{
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO(); // initialize the GPIO pins
    Board_initEMAC(); // initialize the Ethernet
    Board_initUART(); // initialize the UART

    GPIO_write(Board_LED0, Board_LED_ON); // Led is opened.

    System_printf("Starting the Bluetooth Project\nSystem provider is set to "
            "SysMin. Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    // event0 is posted where event0 pended in recvNTP function
    Event_post(event0, Event_Id_00);

    // BIOS START
    BIOS_start();

    return (0);
}
