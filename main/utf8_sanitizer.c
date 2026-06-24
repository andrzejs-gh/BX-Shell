#include "utf8_sanitizer.h"

void sanitize_utf8(uint8_t* input_buffer, size_t len, uint8_t subst)
{
	uint8_t leading_byte;
	size_t i = 0;
	
	while (i < len)
	{
		leading_byte = input_buffer[i];
		
		if ( (leading_byte >> 7) == 0x00 ) // ASCII
		{
			if (leading_byte <= 31 || leading_byte == 0x7F) 
				input_buffer[i] = subst;
			i++;
		}
		else if ( (leading_byte & 0xE0) == 0xC0 ) // 110x xxxx & 1110 0000 = 1100 0000
		{										  // if it's leading byte of 2-byte seuence
			if ( i + 1 == len )
			{
				input_buffer[i] = subst;
				break;
			}	
			
			uint8_t b1 = input_buffer[i+1];
			
			if ( (b1 & 0xC0) == 0x80 ) // is continuation byte valid
			{
				uint32_t code_point = ((leading_byte & 0x1F) << 6) | (b1 & 0x3F);
				//
				if (  code_point < 0x0080                             ||  // overlongs
					 (code_point >= 0x0080 && code_point <= 0x009F)   ||  // Cc
					  code_point == 0x00AD                            ||  // Cf
					 (code_point >= 0x0300 && code_point <= 0x036F)   ||  // Mn, Cf
					 (code_point >= 0x0600 && code_point <= 0x0605)   ||  // Cf
					  code_point == 0x061C                            ||  // Cf
					  code_point == 0x06DD                            ||  // Cf
					  code_point == 0x070F )                              // Cf
				{
					input_buffer[i] = subst;
					input_buffer[i+1] = subst;
				}
				i += 2;
			}
			else
			{
				input_buffer[i] = subst;
				i++;
			}
				
		}
		else if ( (leading_byte & 0xF0) == 0xE0 ) // 1110 xxxx & 1111 0000 = 1110 0000
		{										  // if it's leading byte of 3-byte seuence
			if ( 2 >= len - i )
			{
				uint8_t overreach = len - i;
				while (overreach--)
					input_buffer[i++] = subst;
				break;
			}
			
			uint8_t b1 = input_buffer[i+1];
			uint8_t b2 = input_buffer[i+2];
			
			if ( (b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 ) // are cont. bytes correct
			{
				input_buffer[i] = subst;
				input_buffer[i+1] = subst;
				input_buffer[i+2] = subst;

				i += 3;
			}
			else // continuation bytes are not correct
			{
				input_buffer[i] = subst;
				i++;
			}
		}
		else if ( (leading_byte  & 0xF8) == 0xF0 ) // 1111 0xxx & 1111 1000 = 1111 0000
		{										   // if it's leading byte of 4-byte seuence
			if ( 3 >= len - i )
			{
				uint8_t overreach = len - i;
				while (overreach--)
					input_buffer[i++] = subst;
				break;
			}
			
			uint8_t b1 = input_buffer[i+1];
			uint8_t b2 = input_buffer[i+2];
			uint8_t b3 = input_buffer[i+3];
			
			if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80)
			{
				input_buffer[i] = subst;
				input_buffer[i+1] = subst;
				input_buffer[i+2] = subst;
				input_buffer[i+3] = subst;

				i += 4;
			}
			else
			{
				input_buffer[i] = subst;
				i++;
			}
		}	
		else // not a UTF8 leading byte
		{
			input_buffer[i] = subst;
			i++;
		}
	}
	
	return;
}
