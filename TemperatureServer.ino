/************************************************************************/
/*                                                                      */
/*  TemperatureServer                                                   */
/*                                                                      */
/*  This program use the a MX7 developement board,                      */
/*  a TC1 temperature sensor and SSD to create a                        */
/*  Digigal thermometer.  It then displays the reading                  */    
/*  of the thermometer on a html webpage that can be                    */
/*  seen by any computer on the internal network.  The                  */
/*  ip of the server is set to 10.0.0.97                                */
/************************************************************************/
/*  Author:     Steven Soranno                                          */
/************************************************************************/
/************************************************************************/
/*                          Hardware:                                   */
/*                                                                      */
/*  MX7                                                                 */
/*  TC1 Sensor                                                          */
/*  Seven Segment Display                                               */
/************************************************************************/

//********************* Ethernet and Network Configuration *******************************

#include <IM8720PHY.h>                      // This is for the the Internal MAC and SMSC 8720 PHY
#include <DEIPcK.h>                         // TCPIP stack library

IPv4 ipServer = {192,168,1,97};                // static ip
unsigned short portServer = 80;             // Server port  
   
//****************************************************************************************
// Declare variables and import libraries
#include <TC1.h>  // Temperature Sensor Lib
#include "SSD.h"  // Seven Segment display header file
#include <WProgram.h>
#include <SPI.h>  // SPI library

int chipSelectPin = SS;  // Set the pin for reading temperature sensor data

SSD mySSD;  // Declare hardware objects
TC1 myTMP;

typedef enum  // Declare States for server
{
    NONE = 0,
    LISTEN,
    ISLISTENING,
    WAITISLISTENING,
    AVAILABLECLIENT,
    ACCEPTCLIENT,
    READ,
    WRITE,
    CLOSE,
    EXIT,
    DONE
} STATE;

STATE state = LISTEN;  // Set initial state to listen

unsigned tStart = 0;  // Start and wait variables determine how long a connection cn e open
unsigned tWait = 5000;  // in this case 5 seconds

TCPServer tcpServer;  // Declare TCPServer object
#define cTcpClients 10  // Allow 10 tcp clients at a time
TCPSocket rgTcpClient[cTcpClients];  // Create a socket for each client

TCPSocket * ptcpClient = NULL;

IPSTATUS status;

byte rgbRead[1024]; // byte array containing the http request made by the client
int cbRead = 0;
int count = 0;

// byte Array containing the http responce with the html webpage
byte webpage[2048] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<!DOCTYPE html>\r\n<html><head><style>body{display:inline;text-align:center;vertical-align:middle;width:50%;color:white;background-color:blue;}</style><title>TempServer</title>\r\n<body><center><font size=\"10\"><h1>Temperature: ";
int faren = 0;  // Temperature variables
double celsius = 0;

/***    void setup()
 *
 *    Parameters:
 *          None
 *              
 *    Return Values:
 *          None
 *
 *    Description: 
 *    
 *      Arduino setup function.
 *      
 *      Initialize the Serial Monitor, and initializes the
 *      the IP stack for a static IP of ipServer.
 *      Also inializes the temperature sensor and SSD.
 *      
 * ------------------------------------------------------------ */
void setup() {

    Serial.begin(9600);
    Serial.println("TemperatureServer");
    Serial.println("");
    
    // intialize the stack with a static IP
    deIPcK.begin(ipServer);
    // Inialize the temperature sensor by passing the correct pin for input
    myTMP.begin(chipSelectPin);
    // Inialize the SSD by passing the correct pin for output
    mySSD.begin(4,5,6,7,12,13,14,15);
    pinMode(chipSelectPin, OUTPUT); 

    // initialize sensor  
    digitalWrite(chipSelectPin, HIGH);  
    delay(100); // wait 100ms
}

/***    void loop()
 *
 *    Parameters:
 *          None
 *              
 *    Return Values:
 *          None
 *
 *    Description: 
 *    
 *      Arduino loop function.
 *      
 *      We are using the default timeout values for the DNETck and TcpServer class
 *      which usually is enough time for the Tcp functions to complete on their first call.
 *      
 *      Get the value of the temperature from the temperature sensor and place that value into 
 *      the html webpage to be sent to the client.
 *
 *      This code listens for an HTTP GET client request, then sends a html webpage in return.
 *      After about 5 seconds of inactivity, it will drop the connection.
 * 
 *      
 * ------------------------------------------------------------ */
void loop() {
    // get the temperature value from the sensor in celsius.
    celsius = myTMP.getTemp(); 
    faren = (int)myTMP.celToFar(celsius);  // convert the temperature to fahrenheit
    mySSD.DisplayNumber(faren); // Display that value on the SSD

    // Convert the faren integer to a char and store it in the webpage Array
    char x[1];
    char y[1];
    itoa(((faren/10)%10),x,10);
    itoa((faren%10),y,10);
    webpage[293] = x[0];
    webpage[294] = y[0];

    byte temp[39] = "</h1></font></body></center></html>\r\n";
    int i =0;
    for(i = 295; i<39; i++){
        webpage[i] = temp[i-295];
    }
    //delay(10);
    
    switch(state)
    {

    // say to listen on the port
    case LISTEN:
        if(deIPcK.tcpStartListening(portServer, tcpServer))
        {
            for(int i = 0; i < cTcpClients; i++)
            {
                tcpServer.addSocket(rgTcpClient[i]);
            }
        }
        state = ISLISTENING;
        break;

    case ISLISTENING:
        count = tcpServer.isListening();
        Serial.print(count, DEC);
        Serial.print(" sockets listening on port: ");
        Serial.println(portServer, DEC);

        if(count > 0)
        {
            state = AVAILABLECLIENT;
        }
        else
        {
            state = WAITISLISTENING;
        }
        break;

    case WAITISLISTENING:
        if(tcpServer.isListening() > 0)
        {
            state = ISLISTENING;
        }
        break;

    // wait for a connection
    case AVAILABLECLIENT:
        if((count = tcpServer.availableClients()) > 0)
        {
            Serial.print("Got ");
            Serial.print(count, DEC);
            Serial.println(" clients pending");
            state = ACCEPTCLIENT;
        }
        break;

    // accept the connection
    case ACCEPTCLIENT:
        
        // accept the client
        if((ptcpClient = tcpServer.acceptClient()) != NULL && ptcpClient->isConnected())
        {
            Serial.println("Got a Connection");
            state = READ;
            tStart = (unsigned) millis();
        }

        // this probably won't happen unless the connection is dropped
        // if it is, just release our socket and go back to listening
        else
        {
            state = CLOSE;
        }
        break;

    // wait fot the read, but if too much time elapses (5 seconds)
    // we will just close the tcpClient and go back to listening
    case READ:
        // see if we got anything to read
        if((cbRead = ptcpClient->available()) > 0)
        {
            cbRead = cbRead < sizeof(rgbRead) ? cbRead : sizeof(rgbRead);
            cbRead = ptcpClient->readStream(rgbRead, cbRead);

            Serial.print("Got ");
            Serial.print(cbRead, DEC);
            Serial.println(" bytes");
            
            if(rgbRead[0] == 'G' && rgbRead[4] == '/' && rgbRead[5] == ' '){   // if the server receives a http get request jump to the WRITE STATE  
                state = WRITE;
            } else{
                state = CLOSE;
            }
        } else if( (((unsigned) millis()) - tStart) > tWait )
        {
            state = CLOSE;
        }
        break;

    // Send the html webpage back to the client and close the connection.
    case WRITE:
        if(ptcpClient->isConnected())
        {               
            Serial.println("Writing:");
            for(int i=0; i < cbRead; i++) 
            {
                Serial.print((char) rgbRead[i]);
            }
            Serial.println(""); 
            ptcpClient->writeStream(webpage, 306);
            tStart = (unsigned) millis();
            state = CLOSE;
        }

        // the connection was closed on us, go back to listening
        else
        {
            Serial.println("Unable to write back.");  
            state = CLOSE;
        }
        break;
        
    // close our tcpClient and go back to listening
    case CLOSE:
        ptcpClient->close();
        tcpServer.addSocket(*ptcpClient);
        Serial.println("");
        state = ISLISTENING;
        break;

    // something bad happen, just exit out of the program
    case EXIT:
        tcpServer.close();
        Serial.println("Something went wrong, sketch is done.");  
        state = DONE;
        break;

    // do nothing in the loop
    case DONE:
    default:
        break;
    }

    // every pass through loop(), keep the stack alive
    DEIPcK::periodicTasks();
}
