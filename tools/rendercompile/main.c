/* main.c  -  Render library compiler  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>
#include <resource/resource.h>
#include <window/window.h>
#include <render/render.h>

#include "errorcodes.h"

typedef struct {
	bool              display_help;
	int               binary;
	string_const_t    source_path;
	string_const_t*   input_files;
} rendercompile_input_t;

static rendercompile_input_t
rendercompile_parse_command_line(const string_const_t* cmdline);

static void
rendercompile_print_usage(void);

int
main_initialize(void) {
	int ret = 0;
	application_t application;
	foundation_config_t foundation_config;
	resource_config_t resource_config;
	window_config_t window_config;
	render_config_t render_config;

	memset(&foundation_config, 0, sizeof(foundation_config));
	memset(&resource_config, 0, sizeof(resource_config));
	memset(&window_config, 0, sizeof(window_config));
	memset(&render_config, 0, sizeof(render_config));

	memset(&application, 0, sizeof(application));
	application.name = string_const(STRING_CONST("rendercompile"));
	application.short_name = string_const(STRING_CONST("rendercompile"));
	application.config_dir = string_const(STRING_CONST("rendercompile"));
	application.flags = APPLICATION_UTILITY;

	log_enable_prefix(false);
	log_set_suppress(0, ERRORLEVEL_WARNING);

	resource_config.enable_local_source = true;
	resource_config.enable_local_cache = true;
	resource_config.enable_remote_source = true;

	if ((ret = foundation_initialize(memory_system_malloc(), application, foundation_config)) < 0)
		return ret;
	if ((ret = resource_module_initialize(resource_config)) < 0)
		return ret;
	if ((ret = window_module_initialize(window_config)) < 0)
		return ret;
	if ((ret = render_module_initialize(render_config)) < 0)
		return ret;

	log_set_suppress(HASH_RESOURCE, ERRORLEVEL_DEBUG);
	log_set_suppress(HASH_RENDER, ERRORLEVEL_DEBUG);

	return 0;
}

int
main_run(void* main_arg) {
	int result = RENDERCOMPILE_RESULT_OK;
	rendercompile_input_t input = rendercompile_parse_command_line(environment_command_line());

	FOUNDATION_UNUSED(main_arg);

	if (input.display_help) {
		rendercompile_print_usage();
		goto exit;
	}

	resource_source_set_path(STRING_ARGS(input.source_path));
	resource_compile_register(render_compile);

	size_t ifile, fsize;
	for (ifile = 0, fsize = array_size(input.input_files); ifile < fsize; ++ifile) {
		uuid_t uuid = string_to_uuid(STRING_ARGS(input.input_files[ifile]));
		if (uuid_is_null(uuid)) {
			char buffer[BUILD_MAX_PATHLEN];
			string_t pathstr = string_copy(buffer, sizeof(buffer), STRING_ARGS(input.input_files[ifile]));
			pathstr = path_clean(STRING_ARGS(pathstr), sizeof(buffer));
			pathstr = path_absolute(STRING_ARGS(pathstr), sizeof(buffer));
			uuid = resource_import_map_lookup(STRING_ARGS(pathstr));
		}
		if (uuid_is_null(uuid)) {
			log_warnf(HASH_RESOURCE, WARNING_INVALID_VALUE, STRING_CONST("Failed to lookup: %.*s"), STRING_FORMAT(input.input_files[ifile]));
			result = RENDERCOMPILE_RESULT_INVALID_INPUT;
			break;
		}

		string_const_t uuidstr = string_from_uuid_static(uuid);
		if (resource_compile(uuid, RESOURCE_PLATFORM_ALL))
			log_infof(HASH_RESOURCE, STRING_CONST("Successfully compiled: %.*s (%.*s)"),
			          STRING_FORMAT(uuidstr), STRING_FORMAT(input.input_files[ifile]));
		else
			log_warnf(HASH_RESOURCE, WARNING_UNSUPPORTED, STRING_CONST("Failed to compile: %.*s (%.*s)"),
			          STRING_FORMAT(uuidstr), STRING_FORMAT(input.input_files[ifile]));
	}

exit:

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

rendercompile_input_t
rendercompile_parse_command_line(const string_const_t* cmdline) {
	rendercompile_input_t input;
	size_t arg, asize;

	memset(&input, 0, sizeof(input));

	for (arg = 1, asize = array_size(cmdline); arg < asize; ++arg) {
		if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--help")))
			input.display_help = true;
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--source"))) {
			if (arg < asize - 1)
				input.source_path = cmdline[++arg];
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

	bool already_help = input.display_help;
	if (!input.source_path.length)
		input.source_path = resource_source_path();
	if (!already_help && !input.source_path.length) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("No source path given"));
		input.display_help = true;
	}
	if (!already_help && !array_size(input.input_files)) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("No input files given"));
		input.display_help = true;
	}

	return input;
}

static void
rendercompile_print_usage(void) {
	const error_level_t saved_level = log_suppress(0);
	log_set_suppress(0, ERRORLEVEL_DEBUG);
	log_info(0, STRING_CONST(
	             "rendercompile usage:\n"
	             "  rendercompile [--source <path>] [--ascii] [--binary] [--debug] [--help] <file> <uuid> ... [--]\n"
	             "    Arguments:\n"
	             "      <file> <uuid> ...            Any number of input files or UUIDs\n"
	             "    Optional arguments:\n"
	             "      --source <path>              Operate on resource file source structure given by <path>\n"
	             "      --binary                     Write binary files\n"
	             "      --ascii                      Write ASCII files (default)\n"
	             "      --debug                      Enable debug output\n"
	             "      --help                       Display this help message\n"
	             "      --                           Stop processing command line arguments"
	         ));
	log_set_suppress(0, saved_level);
}