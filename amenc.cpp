#include <iostream>
#include "amenc.h"

typedef int (__stdcall *AME_CheckInputAVIFile)( const char* filename, int* width, int* height, int* rate, int* scale, int* frames );
typedef int (__stdcall *AME_CheckInputPNGFile)( const char* filename, int* width, int* height, int* frames );
typedef int (__stdcall *AME_StartEncode)( void** handle, AlphaMovieEnc_EncodeOption* param );
typedef int (__stdcall *AME_EncodeNext)( void* handle, AlphaMovieEnc_EncodeOption* param );
typedef int (__stdcall *AME_Close)( void* handle );

int main(int argc, char *argv[])
{
	// Load amenc.dll
    HINSTANCE hDLL;

    AME_CheckInputAVIFile fpCheckInputAVIFile;
	AME_CheckInputPNGFile fpCheckInputPNGFile;
	AME_StartEncode       fpStartEncode;
	AME_EncodeNext        fpEncodeNext;
	AME_Close             fpClose;

    hDLL = LoadLibrary("amenc");
    if (NULL == hDLL) {
	    std::cerr << "Error loading amenc.dll" << std::endl;
	    return 1;
    }
	fpCheckInputAVIFile = (AME_CheckInputAVIFile)GetProcAddress(hDLL, "AlphaMovieEnc_CheckInputAVIFile");
	fpCheckInputPNGFile = (AME_CheckInputPNGFile)GetProcAddress(hDLL, "AlphaMovieEnc_CheckInputPNGFile");
	fpStartEncode       = (AME_StartEncode)GetProcAddress(hDLL, "AlphaMovieEnc_StartEncode");
	fpEncodeNext        = (AME_EncodeNext)GetProcAddress(hDLL, "AlphaMovieEnc_EncodeNext");
	fpClose             = (AME_Close)GetProcAddress(hDLL, "AlphaMovieEnc_Close");
	if (!fpCheckInputAVIFile || !fpCheckInputPNGFile || !fpStartEncode || !fpEncodeNext || !fpClose) {
		std::cerr << "Error resolving function" << std::endl;
		return 1;
	}

	// Options
	struct AlphaMovieEnc_EncodeOption opt = {
		.is_avi_file = false,
		.input_file_path = 0,
		.output_file_path = 0,
		.input_alpha_type = AME_NORMAL_ALPHA,
		.alpha_compress_type = AME_ALPHA_ZLIB,
		.fps_rate = 30,
		.fps_scale = 1,
		.keep_transparent_block = 1,
		.quality = 95,
		.num_of_frame = 0,
		.currnt_frame = 0,
		.is_finished = 0,
	};

	// Parse arguments
	int argi = 0, help = argc == 1;
	bool force_rate = false, force_scale = false;
	while (++argi < argc) {
		if (strncmp(argv[argi], "--", 2) != 0) {
			if (!opt.input_file_path) {
				opt.input_file_path = argv[argi];
			} else if (!opt.output_file_path) {
				opt.output_file_path = argv[argi];
			} else {
				std::cerr << (std::string("Extra argument: ") + argv[argi]) << std::endl;
				return 1;
			}
		} else if (stricmp(argv[argi], "--png") == 0) {
			opt.is_avi_file = false;
		} else if (stricmp(argv[argi], "--avi") == 0) {
			opt.is_avi_file = true;
		} else if (stricmp(argv[argi], "--no-alpha") == 0) {
			opt.input_alpha_type = AME_NORMAL_ALPHA;
			opt.keep_transparent_block = 0;
		} else if (stricmp(argv[argi], "--force-alpha") == 0) {
			opt.input_alpha_type = AME_ADD_ALPHA;
			opt.keep_transparent_block = 1;
		} else if (stricmp(argv[argi], "--zlib") == 0) {
			opt.alpha_compress_type = AME_ALPHA_ZLIB;
		} else if (stricmp(argv[argi], "--jpeg") == 0) {
			opt.alpha_compress_type = AME_ALPHA_JPEG;
		} else if (stricmp(argv[argi], "--rate") == 0) {
			opt.fps_rate = std::strtoul(argv[++argi], NULL, 0);
			force_rate = true;
		} else if (stricmp(argv[argi], "--scale") == 0) {
			opt.fps_scale = std::strtoul(argv[++argi], NULL, 0);
			force_scale = true;
		} else if (stricmp(argv[argi], "--quality") == 0) {
			opt.quality = std::strtoul(argv[++argi], NULL, 0);
		} else if (stricmp(argv[argi], "--help") == 0) {
			help = 1;
		} else {
			std::cerr << (std::string("Unknown argument: ") + argv[argi]) << std::endl;
			help = 1;
		}
	}

	if (help || !opt.input_file_path || !opt.output_file_path) {
		std::cerr << "Usage:    " << argv[0] << " [--png|--avi] [--no-alpha|--force-alpha] [--zlib|--jpeg]" << std::endl;
		std::cerr << "              [--rate #] [--scale #] [--quality #] input output" << std::endl;
		std::cerr << "Defaults: --png --zlib --rate 30 --scale 1 --quality 95" << std::endl;
		std::cerr << "For AVI file format, it can detect FPS rate and FPS scale automatically" << std::endl;
		return 1;
	}

	// Detect file information
	int width = 0, height = 0, frames = 0, rate = 0, scale = 0;
	int check = AME_SUCCESS;
	if (opt.is_avi_file) {
		check = (*fpCheckInputAVIFile)(opt.input_file_path, &width, &height, &rate, &scale, &frames);
		if (!force_rate)
			opt.fps_rate = rate;
		if (!force_scale)
			opt.fps_scale = scale;
	} else {
		check = (*fpCheckInputPNGFile)(opt.input_file_path, &width, &height, &frames);
	}

	std::cout << "AlphaMovie encoding: " << opt.input_file_path << " => " << opt.output_file_path;
	std::cout << ", " << width << "x" << height << ", " << frames << " frames" << std::endl;

	// Start encoding
	void *handle = 0;
	int enc = AME_SUCCESS;
	bool started = false;
	while (enc == AME_SUCCESS || enc == AME_ENCODE_CONTINUE) {
		if (!started)
			enc = (*fpStartEncode)(&handle, &opt);
		else
			enc = (*fpEncodeNext)(handle, &opt);
		started = true;
		std::cout << opt.input_file_path << " => " << opt.output_file_path << " (" << opt.currnt_frame << "/" << opt.num_of_frame << ")" << std::endl;
	}
	(*fpClose)(handle);
	handle = 0;

	if (enc == AME_ENCODE_FINISH)
		return 0;
	else
		std::cerr << "Error " << enc << std::endl;
	return enc;
}
