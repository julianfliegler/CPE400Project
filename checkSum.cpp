
// this is for client.cpp //
int Checksum(Client c)
 {
  // variables
     int count;
     int Sum = 0;
    // checks if the count is less than te default buffer 
    // adds the total sum
    //  switches the sums's sign
     for (count = 0; count < sizeof(c.SenderBuffer); count++)
         Sum = Sum + c.SenderBuffer[count];
     Sum = -Sum;
    // returns sum
     return (Sum);
 }

// this for server.cpp //
int Checksum(char buffer[DEFAULT_BUFLEN])
 {
   // variables
     int count;
     int Sum = 0;
    // checks if the count is less than te default buffer 
    // adds the total sum
   //  switches the sums's sign 
     for (count = 0; count < DEFAULT_BUFLEN; count++)
         Sum = Sum + buffer[count];
     Sum = -Sum;
 // returns sum 
     return (Sum );
 }
