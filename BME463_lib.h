/* This is the header file for the BME463 library. It contains three functions
 * that are useful for implementing filters to be used in multiple lab 
 * activities! These functions need to be defined in a .cpp file with the same
 * name as this header file. You may implement additional functions if you 
 * wish.
 *
 * To use this library in your main.cpp file you must add the line
 * #include "BME463_lib.h" after #include "mbed.h". 
 */ 


#ifndef _BME463_lib
#define _BME463_lib




/* DEFINE THIS IN LAB 4:
 * Moves all elements in an array of floats to the next position by one. Copies
 * the first element to the second position, the second element to the third 
 * position, and so on.
 * 
 * in := the array to be shifted by one.
 * n  := Number of elements in the array.
 */
void shift_right(float *in, int const n);


/* DEFINE THIS IN LAB 4:
 * This function implements an IIR filter. It is assumed that the input arrays
 * and the coefficient arrays are of the same size. Returns the output value.
 *
 * a  := The attenuation factor.
 * inx := The input numerator array.
 * cx  := The coefficients to dot product with inx.
 * nx  := The number of coefficients in cx.
 * iny := The input denominator array.
 * cy  := The coefficients to dot product with iny.
 * ny  := The number of coefficients in cy.
 */
float filter_IIR(float const a, float const* inx, float const* cx, int const nx, float const* iny, float const* cy, int const ny);


/* DEFINE THIS IN LAB 5:
 * This function implements an FIR filter. It is assumed that the input arrays
 * and the coefficient arrays are of the same size. Returns the output value.
 *
 * a  := The attenuation factor.
 * in := The input x array.
 * c  := The coefficients to dot product with inx.
 * n  := The number of coefficients in cx.
 */
float filter_FIR(float const a, float const* in, float const* c, int const n);


#endif
