//TTN decoder script for observer notification format
//All notifications include an ID (uint16_t), peak value (float/4 bytes), and average value (float/4 bytes). 
function Decoder(bytes, port) {
    // Decode an uplink message from a buffer
    // (array) of bytes to an object of fields.
    var notification = {};
    
  
    rawId = bytes[0] + (bytes[1] << 8);
    notification.id = rawId; //nothing to do here. raw value is already correct. 

    rawPeakValue = bytes[2] + (bytes[3] << 8) + (bytes[4] << 16) + (bytes[5] << 24);
    notification.peakValue = sflt322f(rawPeakValue)

    rawAverageValue = bytes[6] + (bytes[7] << 8) + (bytes[8] << 16) + (bytes[9] << 24);
    notification.averageValue = sflt322f(rawAverageValue)
    

    return notification;S
  }
  
  function sflt322f(rawSflt32)
      {
      //convert raw 32 bit float to float.
      
      // filter required bits only (in case extra bits are passed).
      rawSflt32 &= 0xFFFFFFFF;
    
      // return -0.0 according to IEEE format
      if (rawSflt32 == 0x80000000)
          return -0.0;
  
      // extract sign.
      var sSign = ((rawSflt32 & 0x80000000) !== 0) ? -1 : 1;
      
      // extract exponent
      var exp1 = (rawSflt32 >> 23) & 0xFF;
        
      // extract mantissa
      var mant1 = (rawSflt32 & 0x007FFFFF) / 8388608.0 + 1;
      

      
      var f_unscaled = sSign * mant1 * Math.pow(2, exp1 - 127);
  
      return f_unscaled;
      }