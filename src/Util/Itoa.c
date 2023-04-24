#include <Util/Itoa.h>

void CkUtoaHex( size_t n, char* buffer )
{
	char tmp[16]; // Temp buffer
	
	size_t v;     // Digit value
	size_t i = 0; // Index in tmp
	size_t j = 0; // Index in buffer

	// Handling zero
	if ( n == 0 ) {
		buffer[0] = '0';
		buffer[1] = 0;
		return;
	}

	while ( n ) {
		// Getting the last digit
		v = n % 0x10;
		n >>= 4;

		tmp[i++] = v < 10 ? v + '0' : v + 'A' - 10;
	}

	while ( i ) buffer[j++] = tmp[i--];
	buffer[j] = 0;
}
