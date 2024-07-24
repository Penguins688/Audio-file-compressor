#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <sndfile.h>
#include <samplerate.h>

int main() {
    std::string file_path;
    int new_sample_rate;

    std::cout << "Enter file path: ";
    std::cin >> file_path;
    std::cout << "Enter new sample rate: ";
    std::cin >> new_sample_rate;

    SF_INFO sfinfo;
    SNDFILE *infile = sf_open(file_path.c_str(), SFM_READ, &sfinfo);
    if (!infile) {
        std::cerr << "Error: could not open file" << std::endl;
        return 1;
    }

    const int channels = sfinfo.channels;
    const double src_ratio = static_cast<double>(new_sample_rate) / sfinfo.samplerate;

    std::vector<float> input_buffer(sfinfo.frames * channels);
    sf_count_t num_frames_read = sf_readf_float(infile, input_buffer.data(), sfinfo.frames);
    if (num_frames_read != sfinfo.frames) {
        std::cerr << "Error: could not read all frames" << std::endl;
        sf_close(infile);
        return 1;
    }
    sf_close(infile);

    const int output_frames = static_cast<int>(sfinfo.frames * src_ratio);
    std::vector<float> output_buffer(output_frames * channels);

    SRC_DATA src_data;
    src_data.data_in = input_buffer.data();
    src_data.input_frames = sfinfo.frames;
    src_data.data_out = output_buffer.data();
    src_data.output_frames = output_frames;
    src_data.src_ratio = src_ratio;
    src_data.end_of_input = SF_TRUE;

    int error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, channels);
    if (error) {
        std::cerr << "Error: resampling failed: " << src_strerror(error) << std::endl;
        return -1;
    }

    SF_INFO sfinfo_out = sfinfo;
    sfinfo_out.samplerate = new_sample_rate;
    sfinfo_out.frames = src_data.output_frames_gen;

    SNDFILE *outfile = sf_open("output.wav", SFM_WRITE, &sfinfo_out);
    if (!outfile) {
        std::cerr << "Error: failed to open output file" << std::endl;
        return -1;
    }

    sf_writef_float(outfile, output_buffer.data(), src_data.output_frames_gen);
    sf_close(outfile);

    std::cout << "Resampling complete" << std::endl;
    return 0;
}