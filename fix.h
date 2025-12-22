/**
 * Fixed point math for FFT
 */

#ifndef FIX_H
#define FIX_H

#define FIX_MPY(a, b) ((int)((((long)(a) * (long)(b)) + 0x4000) >> 15))

// Fixed point log2 implementation
template<int RSHIFT_AMOUNT>
int FIX_LOG2(int val) {
    if (val <= 0) return -32767;
    
    int result = 0;
    int temp = val;
    
    // Find the highest bit position
    while (temp > 1) {
        temp >>= 1;
        result++;
    }
    
    return result - RSHIFT_AMOUNT;
}

#endif // FIX_H