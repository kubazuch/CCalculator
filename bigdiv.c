#include "bigmath.h"

/*
 * Divide two big integers with remainder using Knuth algorithm
 */
void bigdiv(BigInt u, BigInt v, BigInt* quo, BigInt* rem)
{
	short nobits;
	int i, j;
	long m, n, qlen;
	register ul* uptr, * vptr, * qptr;
	ul msw;
	ull remainder, qhat, p;
	long long carry, res;
	BIGUNION qunion, runion;

	// Check if we don't divide by 0
	if (iszero(v))
	{
		fprintf(stderr, "Can't divide by zero!\n");
		longjmp(exception, 1);
	}

	m = u.len;
	n = v.len;
	qlen = m - n + 1; // quotient will be at most u.len - v.len + 1

	if (m < n) // a is definitely smaller, we can set quo = 0, rem = a
	{
		*quo = _zero;
		bigcpy(u, rem);

		return;
	}
	else if (n == 1) // divisor is only one word, we can just divide by it
	{
		// divisor is one, we can just copy dividend to quo
		if (v.vals[0] == 1)
		{
			bigcpy(u, quo);
			*rem = _zero;

			return;
		}

		remainder = 0;
		uptr = u.vals + m;

		quo->len = qlen;			// alloc space for quotient
		quo->vals = alloc(qlen);
		qptr = quo->vals + qlen;
		
		// Perform basic long division
		while (qlen--)
		{
			remainder = remainder << 32 | *--uptr;
			*--qptr = (ul)(remainder / v.vals[0]);	
			remainder = remainder % v.vals[0];
		}

		bigtrim(quo); // Trim leading 0s from result
		if (rem != NULL) // Remainder is requested
		{
			rem->len = 1;
			rem->vals = alloc(1);
			rem->vals[0] = (ul)remainder;
		}
		return;
	}

	/*
	 *	Knuth division
	 */

	// 1. Normalize	(shift divisor and dividend left until MSb of divisor is 1)
	nobits = 1;
	msw = v.vals[n - 1];
	while (msw >>= 1)	// count number of bits of MSW of divisor
		nobits++;

	// normalize divisor
	vptr = alloc(n);
	for (i = n - 1; i > 0; i--)
		vptr[i] = ((ull)v.vals[i] << (32 - nobits)) | ((ull)v.vals[i - 1] >> nobits);	// Shift left whole number, borrowing bits from previous words
	vptr[0] = v.vals[0] << (32 - nobits);	// Just shift LSW

	// normalize dividend
	uptr = alloc(m+1);	// dividend may get bigger by one word
	uptr[m] = (ull)u.vals[m - 1] >> nobits;	// Shift MSW overflow
	for (i = m - 1; i > 0; i--)
		uptr[i] = (u.vals[i] << (32 - nobits)) | ((ull)u.vals[i - 1] >> nobits);	// Shift left whole number, borrowing bits from previous words
	uptr[0] = u.vals[0] << (32 - nobits); // Just shift LSW

	// 2. Initialize
	quo->len = m - n + 1;
	quo->vals = alloc(quo->len);
	qptr = quo->vals;

	for (j = m - n; j >= 0; j--)
	{
		// 3. Estimate q[j], using BIGUNION to avoid unnecessary bit shifts
		qunion.svals.high =   uptr[j + n];
		qunion.svals.low  = uptr[j + n - 1];				// qunion now holds [uptr[j+n] uptr[j+n-1]]
		runion.svals.high  = qunion.value % vptr[n - 1];	// store remainder in runion high word, this will be usefull later
		runion.svals.low = uptr[j + n - 2];					// store next word in runion low
		qunion.value /= vptr[n - 1];						// perform division

		// Correct q[j]
		while (qunion.svals.high || qunion.value * vptr[n - 2] > runion.value)	// Estimated q[j] is too big, we have to correct it
		{
			qunion.value -= 1;									// decrement q[j]
			remainder = (ull) runion.svals.high + vptr[n - 1];	// calculate new remainder
			runion.svals.high = remainder;
			if (remainder > ULONG_MAX)							// remainder is bigger than one word, we definitely have correct q[j] now
				break;
		}
		
		qhat = qunion.value;	// store qunion in normal variable, as we will need to use BIGUNION later

		// 4. Multiply and subtract, we can reuse qunion
		carry = 0;
		for (i = 0; i < n; i++)
		{
			qunion.value = qhat * (ull)vptr[i];				// Multiply estimated q[j] by divisor word
			res = uptr[i + j] - carry - qunion.svals.low;	// Subtract result from dividend with carry (borrow)
			uptr[i + j] = res;								// Update dividend
			carry = qunion.svals.high - (res >> 32);		// Recalculate carry (borrow)
		}
		res = uptr[j + n] - carry;	// Subtract carry (borrow) from dividend
		uptr[j + n] = res;			// Update dividend

		qptr[j] = qhat;
		// 5. Test dividend if we didn't subtract too much
		if (res < 0)
		{
			// 6. Add back, we can resue qunion
			qptr[j]--;
			carry = 0;
			for (i = 0; i < n; i++)
			{
				qunion.value = (ull)uptr[i + j] + (ull)vptr[i] + carry;	// Add dividend word + divisor word + carry
				uptr[i + j] = qunion.svals.low;							// low word of sum is result 
				carry = qunion.svals.high;								// high word of sum is carry 
			}
			uptr[j + n] += carry;	// add carry to last word
		}

		// 7. Loop
	}

	bigtrim(quo); // Trim leading 0s from quotient

	if (rem != NULL) // Remainder is requested
	{
		rem->len = n;
		rem->vals = alloc(n);	// alloc memory for remainder
		// 8. Unnormalize (shift back to normal)
		for (i = 0; i < n - 1; i++)
			rem->vals[i] = (uptr[i] >> (32 - nobits)) | ((ull)uptr[i + 1] << nobits);
		rem->vals[n - 1] = uptr[n - 1] >> (32 - nobits);
		bigtrim(rem); // Trim leading 0s from remainder
	}

	// Free normalized values
	freearr(vptr);
	freearr(uptr);
}

void bigquo(BigInt a, BigInt b, BigInt* res)
{
	bigdiv(a, b, res, NULL); // Quotient can be calculated by just not passing remainder pointer
}

void bigmod(BigInt a, BigInt b, BigInt* res)
{
	BigInt tmp;
	bigdiv(a, b, &tmp, res); // Pass temporary quotionent pointer
	freeval(tmp);			 // Free no longer needed pointer
}