## Notes:

1. Real numbers are infinite and the bits we have to represent them are finite. 
   Any representation scheme is therefore going to fall short on precision. We can
   just store what we can represent not the exact thing. like 2.8 is 2.185, like that.

2. Fixed point arithmetic is basically representing real numbers with fixed bits for 
   number and the fraction, we can encode the decimal number as something with the base of
   2 i.e on the right side of the decimal we have 2^-1, 2^-2 and so on.

3. Problem with fixed point is that we can't represent a lot of numbers, we widen the range
   by using floating point. Kind of analogous to decimal base where any number can be represent
   ed as base(0 -10) * 10^exp, in the same way here it is base(0-2) * 2^exp.

