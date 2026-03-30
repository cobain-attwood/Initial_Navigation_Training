
const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data

boolean newData = false;

void setup() 
{ 
  /*while (Serial.available() > 0) 
  {
    Serial.read();
  }*/
  
    Serial.begin(9600);
    Serial.println("<Arduino is ready>");
}

void loop() 
{
    static byte ndx = 0;
    char endMarker = '\n';
    char rc = 0;
    
    while (Serial.available() > 0) 
    {
        rc = Serial.read();

        receivedChars[ndx] = rc;
        ndx++;

        //buffer overrun protection
        if(ndx >= numChars)
        {
          ndx = 0;
        } 
    }

    if (rc == endMarker) 
        {
          receivedChars[ndx] = '\0'; // terminate the string
          ndx = 0;
          //newData = true;
          Serial.println(receivedChars);
        }
}
//Serial.write(receivedChars);
