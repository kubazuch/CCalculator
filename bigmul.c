#include "bigmath.h"

/*
 *	Multiply two BigInts
 */
void bigmul(BigInt a, BigInt b, BigInt* res)
{
	BigInt dest;
	register ul* a_ptr, * b_ptr, * dest_ptr;
	long len;
	ull carry;
	BIGUNION bigunion;

	if (iszero(a) || iszero(b))	// Either is zero, return zero
	{
		*res = _zero;
		return;
	}
	else if (isone(a))	// a is one, so we copy b to result
	{
		bigcpy(b, res);
		return;
	}
	else if (isone(b))	// b is one, so we copy a to result
	{
		bigcpy(a, res);
		return;
	}

	dest.len = a.len + b.len;		// Result will be at most a.len + b.len long
	dest.vals = alloc(dest.len);

	carry = 0;
	dest_ptr = dest.vals;
	a_ptr = a.vals;
	b_ptr = b.vals;

	// Multiply every word of b by LSW of a
	len = b.len;
	while (len--)
	{
		bigunion.value = ((ull)*b_ptr++) * ((ull)*a_ptr) + carry;	// Add `a` word * `b` word + carry
		*dest_ptr++ = bigunion.svals.low;							// low word of sum is result 
		carry = bigunion.svals.high;								// high word of sum is carry
	}
	*dest_ptr = (ul)carry;	// put carry in MSW of result

	// Now for the rest of b using column multiplication
	for (long i = 1; i < a.len; i++)
	{
		carry = 0;
		dest_ptr = dest.vals + i; // start at i-th word of result
		b_ptr = b.vals;
		
		// Multiply every word of b by i-th word of a and add to result
		len = b.len;
		while (len--)
		{
			bigunion.value = ((ull)*b_ptr++) * ((ull) * (a_ptr + i)) + ((ull)*dest_ptr) + carry;	// Add `a` word * `b` word + `dest` word + carry
			*dest_ptr++ = bigunion.svals.low;														// low word of sum is result 
			carry = bigunion.svals.high;															// high word of sum is carry
		}
		*dest_ptr = (ul)carry;	// put carry in MSW of result
	}
	bigtrim(&dest);	// Trim leading 0s from result
	*res = dest;
}

/*
 *	Square BigInt
 */
void bigsqr(BigInt big, BigInt* res)
{
	bigmul(big, big, res); // Multiply number by itself
}

/*
 * Exponentiate BigInt
 */
void bigpow(BigInt a, BigInt b, BigInt* res)
{
	ul exponent;
	BigInt dest, tmp, tmp2;

	// Constant results
	if (iszero(a))		// 0^k
	{
		if (iszero(b))	// 0^0 - undefined
		{
			fprintf(stderr, "Zero to zeroth power is undefined!\n");
			longjmp(exception, 1);
		}

		*res = _zero;
		return;
	}
	else if (iszero(b) || isone(a))	// k^0 = 1 or 1^k = 1
	{
		*res = _one;
		return;
	}

	if (b.len > 1)	// Only allow for one-word exponents
	{
		fprintf(stderr, "Exponent is to big!\n");
		longjmp(exception, 1);
	}

	exponent = b.vals[0];	// Store exponent locally
	if (exponent <= 4)  // Calculate small powers
	{
		switch (exponent) {
		case 1:					// k^1 = k
			bigcpy(a, res);			// Copy value to res
			return;
		case 2:					// k^2
			bigsqr(a, res);			// Square a
			return;
		case 3:					// k^2 = (k^2)*k
			bigsqr(a, &tmp);		// Square a
			bigmul(a, tmp, res);	// Multiply tmp by a
			freeval(tmp);			// Free tmp
			return;
		case 4:					// k^4 = (k^2)^2
			bigsqr(a, &tmp);		// Square a
			bigsqr(tmp, res);		// Square tmp
			freeval(tmp);			// Free tmp
			return;
		}
	}

	// Exponentiation by squaring
	bigcpy(a, &tmp2);
	dest = _one;

	while (exponent)
	{
		if (exponent % 2)	// Exponent is odd - multiply dest by tmp2
		{
			bigmul(dest, tmp2, &tmp);
			freeval(dest);
			dest = tmp;
		}

		// No mater exponent - square tmp2 and halve exponent
		bigsqr(tmp2, &tmp);
		freeval(tmp2);
		tmp2 = tmp;

		exponent /= 2;
	}

	freeval(tmp);

	bigtrim(&dest);	// Trim leading 0s from result
	*res = dest;
}