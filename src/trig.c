#include "trig.h"

// factorial LUT
static const float factLUT[] = {
    1.0f,          // 0!
    1.0f,          // 1!
    2.0f,          // 2!
    6.0f,          // 3!
    24.0f,         // 4!
    120.0f,        // 5!
    720.0f,        // 6!
    5040.0f,       // 7!
    40320.0f,      // 8!
    362880.0f,     // 9!
    3628800.0f,    // 10!
    39916800.0f    // 11!
};

// recursive factorial function
int factorial(int a) {
	if(a==0) return 1;
	return a*factorial(a-1);
}

float cosine(float);
 
float sine(float x) {
	if(x > (PI/2) || x < (-PI/2)){
		float d=x/2;
		return cosine(d)*sine(d)*2;
	}
	int i,j;
	float sine=0;
	float power;
	for(i=0;i<6;i++) {	// changed from 10 iterations to 6
		power=x;
		if(i!=0) {
			for(j=0;j<i*2;j++)
				power*=x;
		}
		if(i%2==1)
			power*=-1;
		sine += power / factLUT[2*i + 1];
	}
	return sine;
}

float cosine(float x){
	float c,s;
	if(x > (PI/2) || x < (-PI/2)) {
		c=cosine(x/2);
		s=sine(x/2);
		return c*c-s*s;
	}
	int i,j;
	float cosine=0;
	float power;
	for(i=0;i<6;i++) {	// changed from 10 iterations to 6
		if(i==0) power=1;
		else power=x;
		if(i!=0) {
			for(j=0;j<i*2-1;j++)
				power*=x;
		}
		if(i%2==1)
			power*=-1;
		cosine += power / factLUT[2*i];
	}
	return cosine;	
}
