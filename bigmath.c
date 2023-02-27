#include "bigmath.h"

/*
 * Constants
 */
ul _zero_val[1] = { 0 };
ul _one_val[1] = { 1 };

BigInt _zero = { _zero_val, 1 };
BigInt _one = { _one_val, 1 };

jmp_buf exception;

/*
 *	Allocate memory for `value` of given size
 */
ul* alloc(long len)
{
	ul* ptr = (ul*)malloc(((size_t) len + 1) * sizeof(ul));
	if (ptr == 0)
	{
		fprintf(stderr, "Not enough memory to alloc BigInt of size %ud", len);
		longjmp(exception, 1);
	}

	return ptr;
}

/*
 *	Copy BigInt 
 */
void bigcpy(BigInt from, BigInt* to)
{
	to->len = from.len;
	if (isleqone(from)) // 0 or 1, assign constant
	{
		to->vals = (from.vals[0] ? _one_val : _zero_val);
		return;
	}

	to->vals = alloc(from.len);
	copyval(from, *to);
}

/*
 *	Add two BigInts
 */
void bigadd(BigInt a, BigInt b, BigInt* res)
{
	BigInt dest;
	register ul* a_ptr, * b_ptr, * dest_ptr;
	long len;
	ull carry;
	BIGUNION bigunion;

	if (b.len > a.len) // We want a to be bigger so swap (using dest_ptr as tmp) if needed
	{
		dest_ptr = a.vals;
		a.vals = b.vals;
		b.vals = dest_ptr;
		len = a.len;
		a.len = b.len;
		b.len = len;
	}

	// Result will be at most 1 longer than a
	dest.len = a.len + 1;
	dest.vals = alloc(dest.len);

	// Helper variables
	carry = 0;
	dest_ptr = dest.vals;
	a_ptr = a.vals;
	b_ptr = b.vals;

	len = b.len;
	while (len--) // We basically do column addition
	{
		bigunion.value = ((ull)*a_ptr++) + ((ull)*b_ptr++) + carry;	// Add `a` word + `b` word + carry
		*dest_ptr++ = bigunion.svals.low;							// low word of sum is result 
		carry = bigunion.svals.high;								// high word of sum is carry
	}

	// b ended, we now only add carry and a
	len = a.len - b.len;
	while (len--)
	{
		bigunion.value = ((ull)*a_ptr++) + carry;	// Add `a` word + `b` word + carry
		*dest_ptr++ = bigunion.svals.low;			// low word of sum is result 
		carry = bigunion.svals.high;				// high word of sum is carry
	}
	*dest_ptr = (ul)carry;	// put carry in MSW of result
	bigtrim(&dest);			// Trim leading 0s from result
	*res = dest;
}

/*
 *	Trim leading zeros of number 
 */
void bigtrim(BigInt* big)
{
	register ul* ptr;
	long len;

	ptr = big->vals + big->len - 1;
	len = big->len;
	while (*ptr == 0 && len > 1)  // Traverse `vals` from MSW to LSW and decrease `len` until nonzero word hit, effectively shrinking the `vals` array
	{
		--ptr;
		--len;
	}

	big->len = len; // set new length
}


/*
 *	Read BigInt from string in given base
 */
void stobig(char* str, ul base, BigInt* res)
{
	BigInt dest, digit, b, tmp;
	ul digit_value;

	digit.vals = &digit_value;	// Make 1 word BigInt to sore digit in it
	digit.len = 1;

	b.vals = &base;	// Make 1 word BigInt to sore base in it
	b.len = 1;

	dest = _zero; // Init dest as 0
	// Convert string to number using Horner's method
	while (*str)
	{
		digit_value = *str++; // Get char from input string

		// Translate char to its numerical value
		if ((digit_value >= '0') && (digit_value <= '9'))
			digit_value -= 48;
		else if ((digit_value >= 'a') && (digit_value <= 'f'))
			digit_value -= 87;
		else if ((digit_value >= 'A') && (digit_value <= 'F'))
			digit_value -= 55;
		else // Invalid character passed as digit
		{
			fprintf(stderr, "Character '%c' is not a digit!\n", digit_value);
			longjmp(exception, 1);
		}

		if (digit_value >= base) // Check whether digit fits in given base
		{
			fprintf(stderr, "Digit '%d' is to big for base (%d)!\n", digit_value, base);
			longjmp(exception, 1);
		}

		// Multiply by base and add digit
		bigmul(dest, b, &tmp);
		freeval(dest);
		bigadd(tmp, digit, &dest);
		freeval(tmp);
	}

	// Trim leading 0s from result
	bigtrim(&dest);
	*res = dest;
}

size_t n;
size_t new_lineptr_len;
char* num, * cur_pos, * new_num;

/*
 *	Print BigInt to given file
 */
void bigprint(BigInt big, ul basev, FILE* result)
{
	long len;
	char c;
	BigInt a, base, quo, rem;

	base.vals = &basev;	// Make 1 word BigInt to sore base in it
	base.len = 1;

	len = big.len;
	if ((len == 1) && (*big.vals < basev)) // Check whether the number is already a digit
	{
		c = digitc(*big.vals);	// Get digit character
		fputc(c, result);		// Print digit
		fputc('\n', result);
		return;
	}

	bigcpy(big, &a);	// Make local copy of number as it will be modified

	if (num == NULL)	// Allocate output buffer is needed, we need it because we find digits from right to left but print from left to right
	{
		n = 128;		// Make it 128 characters long and try to alloc
		if ((num = (char*)malloc(n)) == NULL)
		{
			fprintf(stderr, "Could not allocate memory for number!");
			longjmp(exception, 1);
		}
	}

	cur_pos = num;
	while (a.len > 1 || a.vals[0])
	{
		if ((num + n - cur_pos) < 2)	// The output buffer is to small to contain another digit
		{
			if (MAXSSIZE_T / 2 < n)		// Hard limit for max buffer size = MAXSSIZE_T
			{	
				fprintf(stderr, "The number is too big to print!");
				longjmp(exception, 1);
			}

			new_lineptr_len = n * 2;	// Double the length and try reallocing
			if ((new_num = (char*)realloc(num, new_lineptr_len)) == NULL)
			{
				fprintf(stderr, "Could not allocate memory for number!");
				longjmp(exception, 1);
			}

			cur_pos = new_num + (cur_pos - num);	// Recalculate pointer
			num = new_num;
			n = new_lineptr_len;
		}

		bigdiv(a, base, &quo, &rem);	// Divide number by base, the remainder is digit
		c = digitc(rem.vals[0]);		// Get digit char
		*cur_pos++ = c;					// Add the char at the end
		freeval(a);		// We can dispose of a
		freeval(rem);	// We can dispose of remainder
		a = quo;		// Quotient is our new a
	}

	// Number must be reversed so print from the end
	while (cur_pos > num)
	{
		fputc(*--cur_pos, result);
	}
	fputc('\n', result);
	freeval(a);
}

/*
 *	Free number buffer 
 */
void cleanup()
{
	free(num);
}