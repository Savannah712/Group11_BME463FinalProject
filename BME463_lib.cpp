#include "BME463_lib.h"
#include "stdio.h"


void shift_right(float *in, int const n){
for (int i= n-1; i > 0; i--){
    in[i] = in[i-1];
    }
}

float filter_IIR(float const a, float const* inx, float const* cx, int
const nx, float const* iny, float const* cy, int const ny){
     float result1;
     float result2;
    for (int j =0; j < nx; j++){
        result1 += inx[j]*cx[j];
    }
    for(int k = 1; k< ny; k++){
        result2  += iny[k]*cy[k];
    }
    return result2 + a*result1;
}

/* DEFINE THIS IN LAB 5:
 * This function implements an FIR filter. It is assumed that the input arrays
 * and the coefficient arrays are of the same size. Returns the output value.
 *
 * a  := The attenuation factor.
 * in := The input x array.
 * c  := The coefficients to dot product with inx.
 * n  := The number of coefficients in cx.
 */
float filter_FIR(float const a, float const* in, float const* c, int const n){
     float result1 = 0;

    for(int j = 0; j < n; j++){
        result1 += in[j]*c[j];
    }
    return a*result1;
}
