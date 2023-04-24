#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <stdbool.h>
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

int main(int argc, char** argv) {
    cl_uint num_entries = 1;
    cl_platform_id platforms;
    cl_uint num_platforms;

    cl_int ret;

    ret = clGetPlatformIDs(num_entries, &platforms, &num_platforms);

    if(ret != CL_SUCCESS) {
        fprintf(stderr, "num_entries is equal to zero and platforms is not NULL, or if both num_platforms and platforms are NULL\n");
        return (EXIT_FAILURE);
    }

    printf("Number of OpenCL platforms available: %u\n", num_platforms);

    //cl_device_id devices[100];
    cl_device_id device;
    cl_uint num_devices;

    ret = clGetDeviceIDs(platforms, CL_DEVICE_TYPE_DEFAULT, num_entries, &device, &num_devices);

    if(ret != CL_SUCCESS) {
        fprintf(stderr, "error in clGetDeviceIDs\n");
        return (EXIT_FAILURE);
    }

    printf("The number of OpenCL devices available: %u\n", num_devices);

    //cl_device_id device = devices[0];

    char device_version[1000] = {0};
    clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(device_version), device_version, NULL);
    printf("OpenCL version support: %s\n", device_version);

    cl_ulong device_max_mem_alloc_size;
    clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &device_max_mem_alloc_size, NULL);
    printf("Device max mem alloc size: %lu\n", device_max_mem_alloc_size);

    size_t device_max_work_item_sizes[] = {1,1,1};
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t), &device_max_work_item_sizes, NULL);
    printf("Device max work item sizes: %lu\n", device_max_work_item_sizes);

    cl_context context;

    context = clCreateContext(NULL, num_devices, &device, NULL, NULL, &ret);

    if(ret != CL_SUCCESS) {
        fprintf(stderr, "error in clCreateContext\n");
        return (EXIT_FAILURE);
    }

    cl_command_queue command_queue;

    command_queue = clCreateCommandQueue(context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &ret);

    if(ret != CL_SUCCESS) {
        fprintf(stderr, "error in clCreateCommandQueue\n");
        return (EXIT_FAILURE);
    }

    FILE *fp;
    const char file_name[] = "kernel.cl";
    size_t source_size;
    char *source_str;
    int i;

    fp = fopen(file_name, "r");
    if(!fp) {
        fprintf(stderr, "file open problem\n");
        return (EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    source_size = ftell(fp);
    rewind(fp);

    //printf("source_size: %zd", source_size);

    source_str = (char *)malloc(source_size);
    source_size = fread(source_str, 1, source_size, fp);
    fclose(fp);

    //printf("source_str: %s", source_str);

    cl_program program = NULL;
    cl_uint count = 1;

    program = clCreateProgramWithSource(context, count, (const char **)&source_str, (const size_t *)&source_size, &ret);

    if(ret != CL_SUCCESS) {
        fprintf(stderr, "error in clCreateProgramWithSource\n");
        return (EXIT_FAILURE);
    }

    ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    if(ret != CL_SUCCESS) {
        fprintf(stderr, "error in clBuildProgram\n");
        return (EXIT_FAILURE);
    }



    //Прямой расчет
    void A_C_r(float* At,float* Bt,float* Ct,float* Wt,int a_length,int w_length){
        cl_kernel kernel;
        kernel = clCreateKernel(program, "A_C", &ret);

        if(ret != CL_SUCCESS) {
            fprintf(stderr, "error in clCreateKernel\n");
            return (EXIT_FAILURE);
        }

        //Инициализация в оперативке
        cl_float* A = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* B = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* C = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* P = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* W = (cl_float *)malloc(sizeof(cl_float) * w_length);

        float Pt[5] = {a_length,0,0,0,0};

        //printf("--- Before ---\n");
        for(i = 0; i < a_length; i++) {
            A[i] = At[i];
            B[i] = Bt[i];
            C[i] = Ct[i];
            P[i] = Pt[i];
            //printf("A[%d]: %f\n", i, A[i]);
        }
        for(i = 0; i < w_length; i++){
                int j = (int)(i%a_length);
                int in = (int)(i/a_length);
                W[i] = Wt[i];
                //printf("W[%d][%d]: %f\n", in, j, W[i]);
        }

        //Инициализация на видиокарте
        cl_mem P_obj = NULL;
        P_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, P_obj, CL_TRUE, 0, a_length * sizeof(cl_float), P, 0, NULL, NULL);

        cl_mem A_obj = NULL;
        A_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, A_obj, CL_TRUE, 0, a_length * sizeof(cl_float), A, 0, NULL, NULL);

        cl_mem B_obj = NULL;
        B_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, B_obj, CL_TRUE, 0, a_length * sizeof(cl_float), B, 0, NULL, NULL);

        cl_mem C_obj = NULL;
        C_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, C_obj, CL_TRUE, 0, a_length * sizeof(cl_float), C, 0, NULL, NULL);

        cl_mem W_obj = NULL;
        W_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, w_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, W_obj, CL_TRUE, 0, w_length * sizeof(cl_float), W, 0, NULL, NULL);

        //Загрузка данных на видиокарте
        size_t  arg_size = sizeof(cl_mem);
        ret = clSetKernelArg(kernel, 0, arg_size, (void *)&P_obj);
        ret = clSetKernelArg(kernel, 1, arg_size, (void *)&A_obj);
        ret = clSetKernelArg(kernel, 2, arg_size, (void *)&B_obj);
        ret = clSetKernelArg(kernel, 3, arg_size, (void *)&C_obj);
        ret = clSetKernelArg(kernel, 4, arg_size, (void *)&W_obj);

        //Запуск ядер
        size_t global_work_size[1] = { a_length };
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

        //Выгрузка массивов
        ret = clEnqueueReadBuffer(command_queue, P_obj, CL_TRUE, 0, a_length * sizeof(float), P, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, A_obj, CL_TRUE, 0, a_length * sizeof(float), A, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, B_obj, CL_TRUE, 0, a_length * sizeof(float), B, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, C_obj, CL_TRUE, 0, a_length * sizeof(float), C, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, W_obj, CL_TRUE, 0, w_length * sizeof(float), W, 0, NULL, NULL);

        printf("--- After ---\n");
        /*
        for(i = 0; i < a_length; i++) {
            printf("A[%d]: %f\n", i, A[i]);
            printf("B[%d]: %f\n", i, B[i]);
            printf("C[%d]: %f\n", i, C[i]);
            printf("P[%d]: %f\n", i, P[i]);
        }
        for(i = 0; i < w_length; i++){
                int j = (int)(i%a_length);
                int in = (int)(i/a_length);
                printf("W[%d][%d]: %f\n", in, j, W[i]);
        }*/
        for(i = 0; i < a_length; i++) {
            Ct[i] = C[i];
            printf("C[%d]: %f\n", i, C[i]);
        }
        clReleaseMemObject(P_obj);
        clReleaseMemObject(A_obj);
        clReleaseMemObject(B_obj);
        clReleaseMemObject(C_obj);
        clReleaseMemObject(W_obj);
        clReleaseKernel(kernel);
    }

    //Обратный расчет 1
    void So_Sh_r(float* Oat,float* Oit,float* Sht,float* Sot,float* Ht,float* Wt,int a_length,int w_length){
        cl_kernel kernel;
        kernel = clCreateKernel(program, "So_Sh", &ret);

        if(ret != CL_SUCCESS) {
            fprintf(stderr, "error in clCreateKernel\n");
            return (EXIT_FAILURE);
        }

        //Инициализация в оперативке
        //cl_float* A = (cl_float *)malloc(sizeof(cl_float));
        cl_float* Oa_t = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* Oi_t = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* So_t = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* Sh_t = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* H_t = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* P = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* W = (cl_float *)malloc(sizeof(cl_float) * w_length);

        float Pt[5] = {a_length,0,0,0,0};


        for(i = 0; i < a_length; i++) {
            Oa_t[i] = Oat[i];
            Oi_t[i] = Oit[i];
            Sh_t[i] = Sht[i];
            So_t[i] = Sot[i];
            H_t[i] = Ht[i];
            P[i] = Pt[i];
            printf("Oi[%d]: %f\n", i, Oi_t[i]);
        }
        for(i = 0; i < w_length; i++){
                int j = (int)(i%a_length);
                int in = (int)(i/a_length);
                W[i] = Wt[i];
        }

        //Инициализация на видиокарте
        cl_mem P_obj = NULL;
        P_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, P_obj, CL_TRUE, 0, a_length * sizeof(cl_float), P, 0, NULL, NULL);

        cl_mem Oa_t_obj = NULL;
        Oa_t_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, Oa_t_obj, CL_TRUE, 0, a_length * sizeof(cl_float), Oa_t, 0, NULL, NULL);

        cl_mem Oi_t_obj = NULL;
        Oi_t_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, Oi_t_obj, CL_TRUE, 0, a_length * sizeof(cl_float), Oi_t, 0, NULL, NULL);

        cl_mem Sh_t_obj = NULL;
        Sh_t_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, Sh_t_obj, CL_TRUE, 0, a_length * sizeof(cl_float), Sh_t, 0, NULL, NULL);

        cl_mem So_t_obj = NULL;
        So_t_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, So_t_obj, CL_TRUE, 0, a_length * sizeof(cl_float), So_t, 0, NULL, NULL);

        cl_mem H_t_obj = NULL;
        H_t_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, H_t_obj, CL_TRUE, 0, a_length * sizeof(cl_float), H_t, 0, NULL, NULL);

        cl_mem W_obj = NULL;
        W_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, w_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, W_obj, CL_TRUE, 0, w_length * sizeof(cl_float), W, 0, NULL, NULL);

        //Загрузка данных на видиокарте
        size_t  arg_size = sizeof(cl_mem);
        ret = clSetKernelArg(kernel, 0, arg_size, (void *)&P_obj);
        ret = clSetKernelArg(kernel, 1, arg_size, (void *)&Oa_t_obj);
        ret = clSetKernelArg(kernel, 2, arg_size, (void *)&Oi_t_obj);
        ret = clSetKernelArg(kernel, 3, arg_size, (void *)&Sh_t_obj);
        ret = clSetKernelArg(kernel, 4, arg_size, (void *)&So_t_obj);
        ret = clSetKernelArg(kernel, 5, arg_size, (void *)&H_t_obj);
        ret = clSetKernelArg(kernel, 6, arg_size, (void *)&W_obj);

        //Запуск ядер
        size_t global_work_size[1] = { a_length };
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

        //Выгрузка массивов
        ret = clEnqueueReadBuffer(command_queue, P_obj, CL_TRUE, 0, a_length * sizeof(float), P, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, Oa_t_obj, CL_TRUE, 0, a_length * sizeof(float), Oa_t, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, Oi_t_obj, CL_TRUE, 0, a_length * sizeof(float), Oi_t, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, Sh_t_obj, CL_TRUE, 0, a_length * sizeof(float), Sh_t, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, So_t_obj, CL_TRUE, 0, a_length * sizeof(float), So_t, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, H_t_obj, CL_TRUE, 0, a_length * sizeof(float), H_t, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, W_obj, CL_TRUE, 0, w_length * sizeof(float), W, 0, NULL, NULL);

        for(i = 0; i < a_length; i++) {
            Sht[i] = Sh_t[i];
            Sot[i] = So_t[i];
            //printf("Sh[%d]: %f\n", i, Sh_t[i]);
            //printf("So[%d]: %f\n", i, So_t[i]);
        }
        clReleaseMemObject(P_obj);
        clReleaseMemObject(Oa_t_obj);
        clReleaseMemObject(Oi_t_obj);
        clReleaseMemObject(Sh_t_obj);
        clReleaseMemObject(So_t_obj);
        clReleaseMemObject(H_t_obj);
        clReleaseMemObject(W_obj);
        clReleaseKernel(kernel);
    }

    //Обратный расчет 2
    void W_dW_r(float* At,float* Bt,float* dBt,float* St,float* Wt,float* dWt,int a_length,int w_length){
        cl_kernel kernel;
        kernel = clCreateKernel(program, "W_dW", &ret);

        if(ret != CL_SUCCESS) {
            fprintf(stderr, "error in clCreateKernel\n");
            return (EXIT_FAILURE);
        }

        //Инициализация в оперативке
        cl_float* P = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* A = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* B = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* dB = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* S = (cl_float *)malloc(sizeof(cl_float) * a_length);
        cl_float* W = (cl_float *)malloc(sizeof(cl_float) * w_length);
        cl_float* dW = (cl_float *)malloc(sizeof(cl_float) * w_length);

        float Pt[5] = {a_length,0.7,0.3,0,0};

        //printf("--- Before ---\n");
        for(i = 0; i < a_length; i++) {
            A[i] = At[i];
            B[i] = Bt[i];
            dB[i] = dBt[i];
            S[i] = St[i];
            P[i] = Pt[i];
        }
        for(i = 0; i < w_length; i++){
                int j = (int)(i%a_length);
                int in = (int)(i/a_length);
                W[i] = Wt[i];
                dW[i] = dWt[i];
                //printf("W[%d][%d]: %f\n", in, j, W[i]);
        }

        //Инициализация на видиокарте
        cl_mem P_obj = NULL;
        P_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, P_obj, CL_TRUE, 0, a_length * sizeof(cl_float), P, 0, NULL, NULL);

        cl_mem A_obj = NULL;
        A_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, A_obj, CL_TRUE, 0, a_length * sizeof(cl_float), A, 0, NULL, NULL);

        cl_mem B_obj = NULL;
        B_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, B_obj, CL_TRUE, 0, a_length * sizeof(cl_float), B, 0, NULL, NULL);

        cl_mem dB_obj = NULL;
        dB_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, dB_obj, CL_TRUE, 0, a_length * sizeof(cl_float), dB, 0, NULL, NULL);

        cl_mem S_obj = NULL;
        S_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, a_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, S_obj, CL_TRUE, 0, a_length * sizeof(cl_float), S, 0, NULL, NULL);

        cl_mem W_obj = NULL;
        W_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, w_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, W_obj, CL_TRUE, 0, w_length * sizeof(cl_float), W, 0, NULL, NULL);

        cl_mem dW_obj = NULL;
        dW_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, w_length * sizeof(cl_float), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, dW_obj, CL_TRUE, 0, w_length * sizeof(cl_float), dW, 0, NULL, NULL);

        //Загрузка данных на видиокарте
        size_t  arg_size = sizeof(cl_mem);
        ret = clSetKernelArg(kernel, 0, arg_size, (void *)&P_obj);
        ret = clSetKernelArg(kernel, 1, arg_size, (void *)&A_obj);
        ret = clSetKernelArg(kernel, 2, arg_size, (void *)&B_obj);
        ret = clSetKernelArg(kernel, 3, arg_size, (void *)&dB_obj);
        ret = clSetKernelArg(kernel, 4, arg_size, (void *)&S_obj);
        ret = clSetKernelArg(kernel, 5, arg_size, (void *)&W_obj);
        ret = clSetKernelArg(kernel, 6, arg_size, (void *)&dW_obj);

        //Запуск ядер
        size_t global_work_size[1] = { a_length };
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

        //Выгрузка массивов
        ret = clEnqueueReadBuffer(command_queue, P_obj, CL_TRUE, 0, a_length * sizeof(float), P, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, A_obj, CL_TRUE, 0, a_length * sizeof(float), A, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, B_obj, CL_TRUE, 0, a_length * sizeof(float), B, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, dB_obj, CL_TRUE, 0, a_length * sizeof(float), dB, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, S_obj, CL_TRUE, 0, a_length * sizeof(float), S, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, W_obj, CL_TRUE, 0, w_length * sizeof(float), W, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, dW_obj, CL_TRUE, 0, w_length * sizeof(float), dW, 0, NULL, NULL);

        //printf("--- After ---\n");
        for(i = 0; i < a_length; i++) {
            Bt[i] = B[i];
            dBt[i] = dB[i];
            //printf("B[%d]: %f\n", i, B[i]);
        }
        for(i = 0; i < w_length; i++){
                int j = (int)(i%a_length);
                int in = (int)(i/a_length);
                Wt[i] = W[i];
                dWt[i] = dW[i];
                //printf("W[%d][%d]: %f\n", in, j, W[i]);
        }
        clReleaseMemObject(P_obj);
        clReleaseMemObject(A_obj);
        clReleaseMemObject(B_obj);
        clReleaseMemObject(dB_obj);
        clReleaseMemObject(S_obj);
        clReleaseMemObject(W_obj);
        clReleaseMemObject(dW_obj);
        clReleaseKernel(kernel);
    }
    bool isPermutation(float* str1, float* str2, int size)
    {
        for (int i = 0; i < size; ++i)
        {
            if (str1[i] != str2[i])
                return false;
        }

        return true;
    }
    float I[2] = {0,0};

    bool flag = true;
    int a=2;int a2=4;

    float Bh[2] = {0,0};
    float H[2] = {0,0};
    float Wh[4] =   {0.45,0.78,
                     -0.12,0.13};
    float Bo[2] = {0,0};
    float O[2] = {0,0};
    float Wo[4] =    {0.5,0,
                     -0.3,0};
    float So[2] = {0,0};
    float Sh[2] = {0,0};
    float Oi[2] = {1,0};
    float dWo[4] =   {0,0,
                      0,0};
    float dBo[2] = {0,0};
    float dWh[4] =   {0,0,
                      0,0};
    float dBh[2] = {0,0};
    int k = 0;int k123 = 0;
    while(flag){

        int i123 = 0;
        for(i123=0;i123<4;i123++) {
            system("cls");
            char p[16];
            itoa(i123,p,15);
            char si[16];
            strcpy(si,"input/I");
            strcat(si,p);
            strcat(si,".bin");

            //Чтение данных
            FILE* fd = fopen(si, "rb");
            size_t result = fread(I, 1, sizeof(I), fd);
            fclose(fd);
            strcpy(si,"output/O");
            strcat(si,p);
            strcat(si,".bin");
            fd = fopen(si, "rb");
            result = fread(Oi, 1, sizeof(Oi), fd);
            fclose(fd);

            A_C_r(I,Bh,H,Wh,a,a2);
            A_C_r(H,Bo,O,Wo,a,a2);
            bool check = isPermutation(O,Oi,a);
            if(check) {
                if(k==4){
                    flag = false;
                }
                else {
                    k++;
                }
            }
            else{
                k=0;
            }
            So_Sh_r(O,Oi,Sh,So,H,Wo,a,a2);
            W_dW_r(H,Bo,dBo,So,Wo,dWo,a,a2);
            W_dW_r(I,Bh,dBh,Sh,Wh,dWh,a,a2);
            if(k123%100 == 0){
                Sleep(2000);
                k123 = 0;
            }

        }
        k123++;
    }
    Sleep(10000);
    return (EXIT_SUCCESS);
}

