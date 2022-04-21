bool verify_checksum(data_slice data)   // data slice is just soething random for placement sake
{
    if (data.size() < checksum_size)  //  checks the size of the data  and if smaller than checksum size returns false
        return false;

    data_slice body(data.begin(), data.end() - checksum_size); 
    auto checksum = (data.end() - checksum_size);  // I am not sure  about this part 
    return checksum(body) == checksum;
} 

