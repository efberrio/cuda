#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void scaleImageOpenCL(__global int *pixels, const int minpix, const int maxpix, const int imageSize)
{   
    int index = get_global_id(0);
    int value;
    /* Avoid accesing data beyond the end of the arrays */
    if (index < imageSize) {
        value = round(((double)(pixels[index] - minpix) / (maxpix - minpix)) * 255);
        pixels[index] = value;
    }
}

__kernel void edgeDetectionOpenCL(__global int *pixels, __global int *tempImage, const int width, const int height, const int imageSize)
{   
    int index = get_global_id(0);
    int x = 0, y = 0;
    int xG = 0, yG = 0;

    /* Avoid accesing data beyond the end of the arrays */
    if (index < imageSize) {
        x = index % width;

        if (index != 0) {
            y = convert_int4_sat((convert_float4(index) / convert_float4(width)));
        }
        
        if (x < (width - 1) && y < (height - 1)
                && (y > 0) && (x > 0)) {

            //index = x + (y * width)
            //Finds the horizontal gradient
            xG = (pixels[(x+1) + ((y-1) * width)]
                         + (2 * pixels[(x+1) + (y * width)])
                         + pixels[(x+1) + ((y+1) * width)]
                                  - pixels[(x-1) + ((y-1) * width)]
                                           - (2 * pixels[(x-1) + (y * width)])
                                           - pixels[(x-1) + ((y+1) * width)]);

            //Finds the vertical gradient
            yG = (pixels[(x-1) + ((y+1) * width)]
                         + (2 * pixels[(x) + ((y + 1) * width)])
                         + pixels[(x+1) + ((y+1) * width)]
                                  - pixels[(x-1) + ((y-1) * width)]
                                           - (2 * pixels[(x) + ((y-1) * width)])
                                           - pixels[(x+1) + ((y-1) * width)]);
            tempImage[index] = convert_int4_rte(sqrt(convert_float4(xG * xG) + convert_float4(yG * yG)));

        } else {

            //Pads out of bound pixels with 0
            tempImage[index] = 0;

        }
    }
}
