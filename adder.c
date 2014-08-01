#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef uint8_t byte;

inline signed char varint_compare(const byte a[],const byte b[],size_t size){
	return memcmp(a,b,size);
}

inline bool varint_equals(const byte a[],const byte b[],size_t size){
	return varint_compare(a,b,size)==0;
}

inline bool varint_greaterThan(const byte a[],const byte b[],size_t size){
	return varint_compare(a,b,size)>0;
}

inline bool varint_lesserThan(const byte a[],const byte b[],size_t size){
	return varint_compare(a,b,size)<0;
}

inline bool varint_equalsi(const byte a[],int b,size_t size){
	if(size > sizeof(int)){
		size_t i=size-sizeof(int);
		while(i-->0)
			if(a[i]!=0)
				return false;
		return varint_compare(a+size-sizeof(int),(byte*)&b,sizeof(int))==0;
	}else if(size < sizeof(int)){
		int tmp;
		memcpy(&tmp,&b,size);
		if(tmp!=b)
			return false;
		return varint_compare(a,(byte*)&b,size)==0;
	}else
		return *(int*)a==b;
}

/**
 * @return Whether the summing overflowed the limit limited by the size/capacity of the data
 */
bool varint_added(const byte a[],const byte b[],byte out[],size_t size){
	//Base n carry from sum
	bool carry = false;
	size_t i=size;
	
	{//Calculate a big portion using the largest int type (in base BIT_SIZE*sizeof(uintmax_t))
		if(i >= sizeof(uintmax_t)){
			uintmax_t* sum_ptr,
			         * a_ptr,
			         * b_ptr;

			do{
				i-=sizeof(uintmax_t);

				sum_ptr = (uintmax_t*)(&out[size-i-1]);//`size-i-1` or just `i` depends on endian
				a_ptr   = (uintmax_t*)(&a[size-i-1]);
				b_ptr   = (uintmax_t*)(&b[size-i-1]);

				//Sum of a, b and the carry, letting it overflow
				*sum_ptr = *a_ptr + *b_ptr + carry;
				
				///*Debugging*/printf("%u: a=%u , b=%u , carried=%u , sum = a+b+carry = %u\n",i,*a_ptr,*b_ptr,carry,*sum_ptr);
				
				//Check if the summing overflowed
				carry = *a_ptr>*sum_ptr || *b_ptr>*sum_ptr;
			}while(i>0);
		}
	}

	{//Calculate the rest using the smallest int type (in base BIT_SIZE*sizeof(uint8_t))
		if(i >= sizeof(uint8_t)){
			uint8_t* sum_ptr,
			       * a_ptr,
			       * b_ptr;

			do{
				i-=sizeof(uint8_t);

				sum_ptr = (uint8_t*)(&out[size-i-1]);//`size-i-1` or just `i` depends on endian
				a_ptr   = (uint8_t*)(&a[size-i-1]);
				b_ptr   = (uint8_t*)(&b[size-i-1]);

				//Sum of a, b and the carry, letting it overflow
				*sum_ptr = *a_ptr + *b_ptr + carry;
				
				//Check if the summing overflowed
				carry = *a_ptr>*sum_ptr || *b_ptr>*sum_ptr;
			}while(i>0);
		}
	}

	return carry;
}

/**
 * @param size Size of the data (a, b and out) in bytes
 * @return     Whether the subtracting underflowed. It true,, it means that (a<b) and (2^(size*BIT_SIZE)-out = result) in theory
 */
bool varint_subtracted(const byte a[],const byte b[],byte out[],size_t size){
	//Base n borrow from difference
	bool borrow = false;
	size_t i=size;

	{//Calculate the rest using the smallest int type (in base BIT_SIZE*sizeof(uint8_t))
		if(i >= sizeof(uint8_t)){
			uint8_t* diff_ptr,
			       * a_ptr,
			       * b_ptr;

			do{
				i-=sizeof(uint8_t);

				diff_ptr = (uint8_t*)(&out[size-i-1]);//`size-i-1` or just `i` depends on endian
				a_ptr    = (uint8_t*)(&a[size-i-1]);
				b_ptr    = (uint8_t*)(&b[size-i-1]);

				//Difference between a and b counting with the borrow
				if(*a_ptr<*b_ptr || (borrow && *a_ptr==*b_ptr)){//a<b when a-b => underflow
					/* Guarantees:
					 *   0 <= (INTn_MAX-*b_ptr) <= 2^n-1
					 *   (+1) because of UINTn_MAX being one away from the base (UINTn_MAX = 2^n-1)
					 */
					*diff_ptr = (UINT8_MAX-*b_ptr) + 1 + *a_ptr - borrow;
					borrow = true;
				}else{
					/* Guarantees:
					 *   *a_ptr > b_ptr
					 */
					*diff_ptr = (*a_ptr - *b_ptr) - borrow;
					borrow = false;
				}
			}while(i>0);
		}
	}

	return borrow;
}

/**
 * Implemented using Peasant multiplication
 * @param size Size of the data (a, b and out) in bytes
 * @return     Whether the multiplication overflowed.
 */
bool varint_multiplied(const byte a[],const byte b[],byte out[],size_t size){
	//Initial value
	memcpy(out,b,size);

	/*
	while(varint_greaterThani(a,0)){
		varint_shiftRighti(a,1,size);
		varint_shiftLefti(b,1,size);

		//If b+out overflowed
		if(varint_add(out,b,size))
			return true;
	}*/

	return false;
}

int main(int argc,const char* argv[]){
	struct{
		uint16_t a;
		uint16_t b;
		uint16_t result;
	}values = {65500,65316};//TODO: When b is 65316, carry=0 when adding?

	bool carry;

	carry = varint_added((byte*)&values.a,(byte*)&values.b,(byte*)&values.result,sizeof(uint16_t));
	printf("%u + %u = %u (Carry: %u) = %u\n",values.a,values.b,values.result,carry,values.a+values.b);

	carry = varint_subtracted((byte*)&values.a,(byte*)&values.b,(byte*)&values.result,sizeof(uint16_t));
	printf("%u - %u = %u (Borrow: %u) = %u\n",values.a,values.b,values.result,carry,values.a-values.b);

	return 0;
}
