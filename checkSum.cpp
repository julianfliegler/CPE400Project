
// this is for client.cpp //
int Checksum(Client c)
 {
     int count;
     int Sum = 0;
     
     for (count = 0; count < sizeof(c.SenderBuffer); count++)
         Sum = Sum + c.SenderBuffer[count];
     Sum = -Sum;
     return (Sum & 0xFF);
 }

// this for server.cpp //
int Checksum(char buffer[DEFAULT_BUFLEN])
 {
     int count;
     int Sum = 0;
     
     for (count = 0; count < DEFAULT_BUFLEN; count++)
         Sum = Sum + buffer[count];
     Sum = -Sum;
     return (Sum & 0xFF);
 }
