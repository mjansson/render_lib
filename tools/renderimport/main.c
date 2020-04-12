/* main.c  -  Render library importer  -  Public Domain  -  2014 Mattias Jansson 
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/render_lib
 *
 * The foundation library source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>
#include <resource/resource.h>
#include <window/window.h>
#include <network/network.h>
#include <render/render.h>

#include "main.h"
#include "errorcodes.h"

typedef struct {
	bool              display_help;
	int               binary;
	string_const_t    source_path;
	string_const_t*   config_files;
	string_const_t*   input_files;
} renderimport_input_t;

static void
renderimport_parse_config(const char* path, size_t path_size,
                          const char* buffer, size_t size,
                          const json_token_t* tokens, size_t numtokens);

static renderimport_input_t
renderimport_parse_command_line(const string_const_t* cmdline);

static void
renderimport_print_usage(void);

int
main_initialize(void) {
	int ret = 0;
	application_t application;
	foundation_config_t foundation_config;
	resource_config_t resource_config;
	window_config_t window_config;
	network_config_t network_config;
	render_config_t render_config;

	memset(&foundation_config, 0, sizeof(foundation_config));
	memset(&resource_config, 0, sizeof(resource_config));
	memset(&window_config, 0, sizeof(window_config));
	memset(&network_config, 0, sizeof(network_config));
	memset(&render_config, 0, sizeof(render_config));

	memset(&application, 0, sizeof(application));
	application.name = string_const(STRING_CONST("renderimport"));
	application.short_name = string_const(STRING_CONST("renderimport"));
	application.company = string_const(STRING_CONST(""));
	application.flags = APPLICATION_UTILITY;
	application.version = render_module_version();

	log_enable_prefix(false);
	log_set_suppress(0, ERRORLEVEL_WARNING);

	resource_config.enable_local_autoimport = true;
	resource_config.enable_local_source = true;
	resource_config.enable_local_cache = true;

	if ((ret = foundation_initialize(memory_system_malloc(), application, foundation_config)) < 0)
		return ret;
	if ((ret = network_module_initialize(network_config)) < 0)
		return ret;
	if ((ret = resource_module_initialize(resource_config)) < 0)
		return ret;
	if ((ret = window_module_initialize(window_config)) < 0)
		return ret;
	if ((ret = render_module_initialize(render_config)) < 0)
		return ret;

	log_set_suppress(HASH_RESOURCE, ERRORLEVEL_DEBUG);

	return 0;
}

int
main_run(void* main_arg) {
	int result = RENDERIMPORT_RESULT_OK;
	renderimport_input_t input = renderimport_parse_command_line(environment_command_line());

	FOUNDATION_UNUSED(main_arg);

	for (size_t cfgfile = 0, fsize = array_size(input.config_files); cfgfile < fsize; ++cfgfile)
		sjson_parse_path(STRING_ARGS(input.config_files[cfgfile]), renderimport_parse_config);

	if (input.source_path.length)
		resource_source_set_path(STRING_ARGS(input.source_path));

	if (!resource_source_path().length) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("No source path given"));
		input.display_help = true;
	}

	if (input.display_help) {
		renderimport_print_usage();
		goto exit;
	}

	resource_import_register(render_import);

	size_t ifile, fsize;
	for (ifile = 0, fsize = array_size(input.input_files); ifile < fsize; ++ifile) {
		if (resource_import(STRING_ARGS(input.input_files[ifile]), uuid_null()))
			log_infof(HASH_RESOURCE, STRING_CONST("Successfully imported: %.*s"),
			          STRING_FORMAT(input.input_files[ifile]));
		else
			log_warnf(HASH_RESOURCE, WARNING_UNSUPPORTED, STRING_CONST("Failed to import: %.*s"),
			          STRING_FORMAT(input.input_files[ifile]));
	}

exit:

	array_deallocate(input.config_files);
	array_deallocate(input.input_files);

	return result;
}

void
main_finalize(void) {
	render_module_finalize();
	window_module_finalize();
	resource_module_finalize();
	foundation_finalize();
}

static void
renderimport_parse_config(const char* path, size_t path_size,
                          const char* buffer, size_t size,
                          const json_token_t* tokens, size_t numtokens) {
	resource_module_parse_config(path, path_size, buffer, size, tokens, numtokens);
	render_module_parse_config(path, path_size, buffer, size, tokens, numtokens);
}

static renderimport_input_t
renderimport_parse_command_line(const string_const_t* cmdline) {
	renderimport_input_t input;
	size_t arg, asize;

	memset(&input, 0, sizeof(input));

	for (arg = 1, asize = array_size(cmdline); arg < asize; ++arg) {
		if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--help")))
			input.display_help = true;
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--source"))) {
			if (arg < asize - 1)
				input.source_path = cmdline[++arg];
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--config"))) {
			if (arg < asize - 1)
				array_push(input.config_files, cmdline[++arg]);
		}
		/*else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--uuid"))) {
			if (arg < asize - 1) {
				++arg;
				input.uuid = string_to_uuid(STRING_ARGS(cmdline[arg]));
				if (uuid_is_null(input.uuid))
					log_warnf(HASH_RESOURCE, WARNING_INVALID_VALUE, STRING_CONST("Invalid UUID: %.*s"),
					          STRING_FORMAT(cmdline[arg]));
			}
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--set"))) {
			if (arg < asize - 2) {
				input.key = cmdline[++arg];
				input.value = cmdline[++arg];
			}
		}*/
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--binary"))) {
			input.binary = 1;
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--ascii"))) {
			input.binary = 0;
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--debug"))) {
			log_set_suppress(0, ERRORLEVEL_NONE);
			log_set_suppress(HASH_RESOURCE, ERRORLEVEL_NONE);
			log_set_suppress(HASH_RENDER, ERRORLEVEL_NONE);
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--")))
			break; //Stop parsing cmdline options
		else {
			array_push(input.input_files, cmdline[arg]);
		}
	}
	error_context_pop();

	if (!array_size(input.input_files)) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("No input files given"));
		input.display_help = true;
	}

	return input;
}

static void
renderimport_print_usage(void) {
	const error_level_t saved_level = log_suppress(0);
	log_set_suppress(0, ERRORLEVEL_DEBUG);
	log_info(0, STRING_CONST(
	             "renderimport usage:\n"
	             "  renderimport [--source <path>] [--config <path> ...] [--ascii] [--binary]\n"
	             "               [--debug] [--help] <file> <file> ... [--]\n"
	             "    Arguments:\n"
	             "      <file> <file> ...            Any number of input files\n"
	             "    Optional arguments:\n"
	             "      --source <path>              Operate on resource file source structure given by <path>\n"
	             "      --config <file>              Read and parse config file given by <path>\n"
	             "                                   Loads all .json/.sjson files in <path> if it is a directory\n"
	             "      --binary                     Write binary files\n"
	             "      --ascii                      Write ASCII files (default)\n"
	             "      --debug                      Enable debug output\n"
	             "      --help                       Display this help message\n"
	             "      --                           Stop processing command line arguments"
	         ));
	log_set_suppress(0, saved_level);
}
