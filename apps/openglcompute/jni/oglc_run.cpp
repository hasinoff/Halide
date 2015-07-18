#include <iostream>
#include <iomanip>
#include "avg_filter.h"

const int CHANNELS = 4;

buffer_t make_image(int width, int depth, float host[]) {
    buffer_t bt_input = { 0 };
    bt_input.host = (uint8_t*)&host[0];
    bt_input.stride[0] = depth;
    bt_input.extent[0] = width;
    bt_input.stride[1] = 1;
    bt_input.extent[1] = depth;
    bt_input.elem_size = sizeof(float);
    return bt_input;
}

void print(buffer_t bt) {
    for (int i = 0; i < std::min(bt.extent[0], 10); i++) {
        std::cout << "[";
        for (int j = 0; j < std::min(bt.extent[1], 10); j++) {
            std::cout << std::fixed << std::setprecision(1);
            std::cout << std::setw(4);
            std::cout << +((float*)bt.host)[i * bt.stride[0] +
                                              j * bt.stride[1]];
        }
        std::cout << " ]" << std::endl;
    }
}


extern "C" int halide_copy_to_host(void *, buffer_t *);

int main(int argc, char** argv) {
    int width = 10;
    int depth = CHANNELS;
    float *input = (float*)malloc(sizeof(float) * width * depth);
    float *output = (float*)malloc(sizeof(float) * width * depth);

    buffer_t input_buft = make_image(width, depth, input);
    for (int i = 0; i < std::min(input_buft.extent[0], width); i++) {
        for (int j = 0; j < std::min(input_buft.extent[1], depth); j++) {
            input[i * input_buft.stride[0] +
                  j * input_buft.stride[1]] = i; //((i + j) % 2) * 6;
        }
    }

    std::cout << "Input :\n";
    print(input_buft);

    buffer_t output_buft = make_image(width, depth, output);

    input_buft.host_dirty = true;
    avg_filter(&input_buft, &output_buft);
    halide_copy_to_host(NULL, &output_buft);

    std::cout << "Output :\n";
    print(output_buft);

    bool test_pass = true;
    for (int i = 0; i < std::min(input_buft.extent[0], width); i++) {
        for (int j = 0; j < std::min(input_buft.extent[1], depth); j++) {
            float input_value = input[i * input_buft.stride[0] + j * input_buft.stride[1]];
            float output_value = output[i * input_buft.stride[0] + j * input_buft.stride[1]];
            if (input_value != output_value) {
                std::cout << "Mismatch at (" << i << ", " << j << "): " <<
                    " input=" << (+input_value) << " output=" << (+output_value) << "\n";
                test_pass = false;
            }
        }
    }

    std::cout << (test_pass? "Test passed.\n": "Test failed.\n");
}