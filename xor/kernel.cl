__kernel void A_C(__global float* P,
		   __global float* A,
		   __global float* B,
		   __global float* C,
		   __global float* W) {
    int gid = get_global_id(0);
    int i = 0;
    float x = B[gid];
    int index;
    for(i = 0; i < P[0]; i++) {
        index = P[0]*i + gid;
        x += A[i]*W[index];
    }
    float b_ch = 1;
    float b_zn = 1;
    float Sum = 1;
    for(i = 1; i < 8; i++) {
        b_ch *= x;
		b_zn *= i;
        Sum += b_ch/b_zn;
    }	
    C[gid] = 1/(1+1/Sum);
}

__kernel void So_Sh(__global float* P,
		    __global float* Oa,
		    __global float* Oi,
		    __global float* Sh,
		    __global float* So,
		    __global float* H,
		    __global float* W) {
    int gid = get_global_id(0);
    int i = 0;
    int index;
    if(gid == 0) {
        for(i = 0; i < P[0]; i++) {
            So[i] = (Oi[i] - Oa[i])*(1-Oa[i])*Oa[i];
        }
    }
    float Sum = 0;
    for(i = 0; i < P[0]; i++) {
        index = P[0]*gid + i;
		Sum += W[index]*So[i];
    }   
	Sh[gid] = Sum*(1-H[gid])*H[gid];
}

__kernel void W_dW(__global float* P,
		    __global float* A,
		    __global float* B,
		    __global float* dB,
		    __global float* S,
		    __global float* W,
		    __global float* dW) {
    int gid = get_global_id(0);
    int i = 0;
    int index;
    float GRAD;
    dB[gid] = P[1]*S[gid] + P[2]*dB[gid];
    B[gid] = B[gid] + dB[gid];
    for(i = 0; i < P[0]; i++) {
        index = P[0]*gid + i;
		GRAD = A[gid]*S[i];
        dW[index] = P[1]*GRAD + P[2]*dW[index];
		W[index] = W[index] + dW[index];
    }   
}