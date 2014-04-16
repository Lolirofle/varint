#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


#define varint_base_t uint8_t

//////////////////////////////////////////////////////
// Adder
//
struct varint_sum_t{
	/**
	 * The sum
	 */
	varint_base_t sum;

	/**
	 * The 1-bit overflow carried
	 */
	bool carry;
};

void varint_adder(varint_base_t a,varint_base_t b,bool carry,struct varint_sum_t* out){
	//Sum of a, b and the carry, letting it overflow
	out->sum = a+b+carry;

	//Check if the summing overflowed
	out->carry = a>out->sum || b>out->sum;
}

//////////////////////////////////////////////////////
// varint_t
//

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct varint_t{//TODO: Endian differences
	size_t size;
	varint_base_t data[];//TODO: Should be uint8_t, varint_base_t is used for testing purposes at the moment. THat should be used in the calculations.
}varint_t;

varint_t* varint_init(size_t bytes){
	varint_t* out = malloc(sizeof(varint_t) + sizeof(varint_base_t)*bytes);
	if(out)
		out->size = bytes;
	return out;
}

void varint_free(varint_t* varint){
	free(varint);
}

void varint_and(const varint_t* a,const varint_t* b,varint_t* out){
	for(size_t i=MIN(MIN(a->size,b->size),out->size); i>0; --i){
		out->data[i] = a->data[i]&b->data[i];
	}
}

/**
 * @return True if overflowing, false otherwise
 */
bool varint_add(const varint_t* a,const varint_t* b,varint_t* out){
	//For simplicity, guarantees a.size >= b.size
	if(a->size < b->size){
		//Swap a and b
		const varint_t* tmp = a;
		a = b;
		b = tmp;
	}

	struct varint_sum_t result = {0,false};
	
	size_t min_size = MIN(b->size,out->size);

	for(size_t i=0; i<min_size; ++i){
		varint_adder(a->data[i],b->data[i],result.carry,&result);
		out->data[i] = result.sum;
	}

	if(out->size >= a->size){
		if(a->size != b->size)
			memcpy(&out->data[min_size],&a->data[min_size],a->size-b->size);

		if(result.carry){
			out->data[min_size] = 1;
			result.carry = false;
		}
	}

	return result.carry;
}

bool varint_setfromdata_hostendian(varint_t* out,void* data,size_t size){
	memcpy(out->data,data,MIN(out->size,size));
	return size <= out->size;
}

int main(){
	/*
	struct varint_sum_t result;
	varint_adder(1,255,false,&result);

	union{
		struct{
			uint8_t a;
			uint8_t b;
		};

		uint16_t ab;
	}result2 = {
		.a=result.sum,
		.b=result.carry
	};

	printf("%u %u = %u\n",result.carry,result.sum,result2.ab);
	*/

	struct{
		uint8_t a;
		uint16_t b;
	}values = {255,257};

	varint_t* a = varint_init(sizeof(values.a));
	varint_t* b = varint_init(sizeof(values.b));
	varint_t* result = varint_init(2);

	varint_setfromdata_hostendian(a,&values.a,sizeof(values.a));
	varint_setfromdata_hostendian(b,&values.b,sizeof(values.b));

	bool overflow = varint_add(a,b,result);

	printf("%u %u = %u %u = %u (Overflow: %i)\n",a->data[0],b->data[0],result->data[1],result->data[0],((union{varint_base_t a[2];uint16_t b;}){.a={result->data[0],result->data[1]}}).b,overflow);

	varint_free(result);
	varint_free(b);
	varint_free(a);

	return 0;
}
